// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/git2/blob.hh>
#include <cov/io/function_coverage.hh>
#include <cov/io/strings.hh>
#include <cov/io/types.hh>
#include <cov/repository.hh>

namespace cov::io::handlers {
	namespace {
		inline std::string S(std::string_view view) {
			return {view.data(), view.size()};
		}

		struct entry_impl : cov::function_coverage::entry {
			entry_impl(std::string_view name,
			           std::string_view demangled_name,
			           uint32_t count,
			           io::v1::text_pos const& start,
			           io::v1::text_pos const& end)
			    : name_{S(name)}
			    , demangled_name_{S(demangled_name)}
			    , count_{count}
			    , start_{start}
			    , end_{end} {}

			~entry_impl() = default;

			std::string_view name() const noexcept override { return name_; };
			std::string_view demangled_name() const noexcept override {
				return demangled_name_;
			};
			uint32_t count() const noexcept override { return count_; };
			io::v1::text_pos const& start() const noexcept override {
				return start_;
			};
			io::v1::text_pos const& end() const noexcept override {
				return end_;
			};

		private:
			std::string name_{};
			std::string demangled_name_{};
			uint32_t count_{};
			io::v1::text_pos start_{};
			io::v1::text_pos end_{};
		};

		struct impl : counted_impl<cov::function_coverage> {
			explicit impl(std::vector<std::unique_ptr<entry>>&& files)
			    : files_{std::move(files)} {}

			std::span<std::unique_ptr<entry> const> entries()
			    const noexcept override {
				return files_;
			}

		private:
			std::vector<std::unique_ptr<entry>> files_{};
		};

		constexpr uint32_t uint_32(size_t value) {
			return static_cast<uint32_t>(value &
			                             std::numeric_limits<uint32_t>::max());
		}
		static_assert(sizeof(1ull) > 4);
		static_assert(uint_32(std::numeric_limits<size_t>::max()) ==
		              0xFFFF'FFFF);
	}  // namespace

	ref_ptr<counted> function_coverage::load(uint32_t,
	                                         uint32_t,
	                                         git::oid_view,
	                                         read_stream& in,
	                                         std::error_code& ec) const {
		ec = make_error_code(errc::bad_syntax);
		v1::function_coverage header{};
		if (!in.load(header)) {
			return {};
		}
		auto const entry_size = header.entries.size * sizeof(uint32_t);
		if (entry_size < sizeof(v1::function_coverage::entry) ||
		    header.strings.offset <
		        sizeof(v1::function_coverage) / sizeof(uint32_t) ||
		    header.entries.offset <
		        (header.strings.offset + header.strings.size)) {
			return {};
		}

		if (!in.skip((header.strings.offset * sizeof(uint32_t)) -
		             sizeof(header))) {
			return {};
		}
		strings_view strings{};
		if (!strings.load_from(in, header.strings)) {
			return {};
		}

		if (!in.skip((header.entries.offset -
		              (header.strings.offset + header.strings.size)) *
		             sizeof(uint32_t))) {
			return {};
		}

		cov::function_coverage::builder builder{};
		std::vector<std::byte> buffer{};

		for (uint32_t index = 0; index < header.entries.count; ++index) {
			if (!in.load(buffer, entry_size)) {
				return {};
			}
			auto const& entry =
			    *reinterpret_cast<v1::function_coverage::entry const*>(
			        buffer.data());

			if (!strings.is_valid(entry.name) ||
			    !strings.is_valid(entry.demangled_name)) {
				return {};
			}

			builder.add(strings.at(entry.name),
			            strings.at(entry.demangled_name), entry.count,
			            entry.start, entry.end);
		}

		ec.clear();
		return builder.extract();
	}

	bool function_coverage::store(ref_ptr<counted> const& value,
	                              write_stream& out) const {
		auto const obj = as_a<cov::function_coverage>(
		    static_cast<object const*>(value.get()));
		auto entries = obj->entries();

		auto stg = [&] {
			strings_builder strings{};
			for (auto const& entry_ptr : entries) {
				auto& entry = *entry_ptr;
				strings.insert(entry.name());
				strings.insert(entry.demangled_name());
			}

			return strings.build();
		}();

		auto const locate = [&, size = stg.size()](std::string_view value) {
			auto const offset = stg.locate_or(value, size + 1);
			auto const offset32 = uint_32(offset);
			if (offset != offset32) throw false;
			return static_cast<io::str>(offset32);
		};

		v1::function_coverage hdr{
		    .strings = stg.after<v1::function_coverage>(),
		    .entries =
		        stg.align_array<v1::function_coverage,
		                        v1::function_coverage::entry>(entries.size()),
		};

		if (!out.store(hdr)) {
			return false;
		}
		if (!out.store({stg.data(), stg.size()})) {
			return false;
		}

		for (auto const& entry_ptr : entries) {
			auto& in = *entry_ptr;

			if (!out.store(v1::function_coverage::entry{
			        .name = locate(in.name()),
			        .demangled_name = locate(in.demangled_name()),
			        .count = in.count(),
			        .start = in.start(),
			        .end = in.end(),
			    }))
				return false;
		}

		return true;
	}
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
}  // namespace cov::io::handlers

namespace cov {
	function_coverage::entry::~entry() {}

	std::vector<function_coverage::function> function_coverage::merge_aliases()
	    const {
		std::vector<function> aliases{};
		aliases.reserve(entries().size());

		for (auto const& entry : entries()) {
			auto name = entry->demangled_name();
			if (name.empty()) name = entry->name();
			aliases.push_back({
			    .label{.start = entry->start(), .end = entry->end()},
			    .count = entry->count(),
			});
			aliases.back().label.name.assign(name);
		}

		std::stable_sort(aliases.begin(), aliases.end());
		auto dst = aliases.begin();
		auto end = aliases.end();
		if (dst == end) return aliases;
		for (auto src = std::next(dst); src != end;) {
			if (src->label != dst->label) {
				++dst;
				++src;
				continue;
			}
			dst->count += src->count;
			src = aliases.erase(src);
			end = aliases.end();
		}

		aliases.shrink_to_fit();
		return aliases;
	}

	ref_ptr<function_coverage> function_coverage::create(
	    std::vector<std::unique_ptr<function_coverage::entry>>&& entries) {
		return make_ref<io::handlers::impl>(std::move(entries));
	}

	function_coverage::builder& function_coverage::builder::add(
	    std::unique_ptr<entry>&& entry) {
		entries_.push_back(std::move(entry));
		return *this;
	}

	function_coverage::builder& function_coverage::builder::add(
	    std::string_view name,
	    std::string_view demangled_name,
	    uint32_t count,
	    io::v1::text_pos const& start,
	    io::v1::text_pos const& end) {
		return add(std::make_unique<io::handlers::entry_impl>(
		    name, demangled_name, count, start, end));
	}

	ref_ptr<function_coverage> function_coverage::builder::extract() {
		return function_coverage::create(release());
	}

	std::vector<std::unique_ptr<function_coverage::entry>>
	function_coverage::builder::release() {
		auto entries = std::move(entries_);
		entries_.clear();
		return entries;
	}  // GCOV_EXCL_LINE[GCC]
}  // namespace cov
