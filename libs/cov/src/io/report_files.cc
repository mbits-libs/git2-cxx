// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/io/report_files.hh>
#include <cov/io/strings.hh>
#include <cov/io/types.hh>

namespace cov::io::handlers {
	namespace {
		struct report_entry_impl : report_entry {
			report_entry_impl(report_entry_builder&& data)
			    : data_{std::move(data)} {}
			~report_entry_impl() = default;
			bool is_dirty() const noexcept override { return data_.is_dirty; }
			bool is_modified() const noexcept override {
				return data_.is_modified;
			}
			std::string_view path() const noexcept override {
				return data_.path;
			}
			io::v1::coverage_stats const& stats() const noexcept override {
				return data_.stats;
			}
			git_oid const& contents() const noexcept override {
				return data_.contents;
			}
			git_oid const& line_coverage() const noexcept override {
				return data_.line_coverage;
			}

		private:
			report_entry_builder data_;
		};

		struct impl : counted_impl<cov::report_files> {
			explicit impl(std::vector<std::unique_ptr<report_entry>>&& files)
			    : files_{std::move(files)} {}

			std::vector<std::unique_ptr<report_entry>> const& entries()
			    const noexcept override {
				return files_;
			}

		private:
			std::vector<std::unique_ptr<report_entry>> files_{};
		};

		constexpr uint32_t uint_30(size_t value) {
			return static_cast<uint32_t>(value & ((1u << 30) - 1u));
		}
		static_assert(uint_30(std::numeric_limits<size_t>::max()) ==
		              0x3FFF'FFFF);
	}  // namespace

	ref_ptr<counted> report_files::load(uint32_t,
	                                    uint32_t,
	                                    git_oid const&,
	                                    read_stream& in,
	                                    std::error_code& ec) const {
		ec = make_error_code(errc::bad_syntax);
		v1::report_files header{};
		if (!in.load(header)) return {};
		auto const entry_size = header.entry_size * sizeof(uint32_t);
		if (entry_size < sizeof(v1::report_entry) ||
		    header.entries_offset < header.strings_offset)
			return {};

		if (!in.skip(header.strings_offset * sizeof(uint32_t))) return {};
		strings_view strings{};
		if (!in.load(strings.data, header.strings_size * sizeof(uint32_t)))
			return {};
		strings.resync();

		if (!in.skip((header.entries_offset -
		              (header.strings_offset + header.strings_size)) *
		             sizeof(uint32_t)))
			return {};  // GCOV_EXCL_LINE[WIN32]

		std::vector<std::unique_ptr<report_entry>> result(header.entries_count);
		std::vector<std::byte> buffer{};

		for (auto& file : result) {
			if (!in.load(buffer, entry_size)) return {};
			auto const& entry =
			    *reinterpret_cast<v1::report_entry const*>(buffer.data());

			if (!strings.is_valid(entry.path)) return {};

			report_entry_builder builder{};
			builder.set_dirty(entry.is_dirty)
			    .set_modifed(entry.is_modified)
			    .set_path(strings.at(entry.path))
			    .set_stats(entry.stats)
			    .set_contents(entry.contents)
			    .set_line_coverage(entry.line_coverage);
			file = std::move(builder).create();
		}

		ec.clear();
		return report_files_create(std::move(result));
	}

#if defined(__GNUC__)
// The warning is legit, since as_a<> can return nullptr, if there is no
// cov::report_files in type tree branch, but this should be called from within
// db_object::store, which is guarded by report_files::recognized
//
// Conversion is turned off due to "out_entry.path = path30", which is mitigated
// through uint_30()
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#pragma GCC diagnostic ignored "-Wconversion"
#endif
	bool report_files::store(ref_ptr<counted> const& value,
	                         write_stream& out) const {
		auto const obj =
		    as_a<cov::report_files>(static_cast<object const*>(value.get()));
		auto const& entries = obj->entries();

		auto stg = [&] {
			strings_builder strings{};
			for (auto const& entry_ptr : entries) {
				auto& entry = *entry_ptr;
				strings.insert(entry.path());
			};

			return strings.build();
		}();

		v1::report_files hdr{
		    .strings_offset = 0u,
		    .strings_size =
		        static_cast<uint32_t>(stg.size() / sizeof(uint32_t)),
		    .entries_offset =
		        static_cast<uint32_t>(stg.size() / sizeof(uint32_t)),
		    .entries_count = static_cast<uint32_t>(entries.size()),
		    .entry_size = sizeof(v1::report_entry) / sizeof(uint32_t),
		};

		if (!out.store(hdr)) return false;
		if (!out.store({stg.data(), stg.size()})) return false;

		for (auto const& entry_ptr : entries) {
			auto& in = *entry_ptr;

			auto const path = stg.locate_or(in.path(), stg.size() + 1);
			auto const path30 = uint_30(path);
			if (path != path30 || path30 > stg.size()) return false;

			v1::report_entry entry = {
			    .path = path30,
			    .is_dirty = in.is_dirty() ? 1u : 0u,
			    .is_modified = in.is_modified() ? 1u : 0u,
			    .stats = in.stats(),
			    .contents = in.contents(),
			    .line_coverage = in.line_coverage(),
			};

			if (!out.store(entry)) return false;
		}

		return true;
	}
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
}  // namespace cov::io::handlers

namespace cov {
	report_entry::~report_entry() {}

	std::unique_ptr<report_entry> report_entry_builder::create() && {
		return std::make_unique<io::handlers::report_entry_impl>(
		    std::move(*this));
	}

	ref_ptr<report_files> report_files_create(
	    std::vector<std::unique_ptr<report_entry>>&& entries) {
		return make_ref<io::handlers::impl>(std::move(entries));
	}
}  // namespace cov
