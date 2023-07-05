// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/io/report_files.hh>
#include <cov/io/strings.hh>
#include <cov/io/types.hh>
#include <cov/repository.hh>
#include <git2/blob.hh>

namespace cov::io::handlers {
	namespace {
		struct report_entry_impl : report_entry {
			report_entry_impl(std::string_view path,
			                  io::v1::coverage_stats const& stats,
			                  git_oid const& contents,
			                  git_oid const& line_coverage)
			    : path_{path.data(), path.size()}
			    , stats_{stats}
			    , contents_{contents}
			    , line_coverage_{line_coverage} {}

			~report_entry_impl() = default;
			std::string_view path() const noexcept override { return path_; }
			io::v1::coverage_stats const& stats() const noexcept override {
				return stats_;
			}
			git_oid const& contents() const noexcept override {
				return contents_;
			}
			git_oid const& line_coverage() const noexcept override {
				return line_coverage_;
			}
			std::vector<std::byte> get_contents(
			    repository const& repo,
			    std::error_code& ec) const noexcept {
				auto const obj = repo.lookup<cov::blob>(contents_, ec);
				if (!obj || ec) return {};
				auto blob = obj->peek().filtered(path_.c_str(), ec);
				if (ec) return {};
				auto const data = git::bytes{blob};
				auto result = std::vector<std::byte>{data.begin(), data.end()};
				git_buf_dispose(&blob);
				return result;
			}

		private:
			std::string path_{};
			io::v1::coverage_stats stats_{};
			git_oid contents_{};
			git_oid line_coverage_{};
		};

		struct impl : counted_impl<cov::report_files> {
			explicit impl(std::vector<std::unique_ptr<report_entry>>&& files)
			    : files_{std::move(files)} {}

			std::vector<std::unique_ptr<report_entry>> const& entries()
			    const noexcept override {
				return files_;
			}

			report_entry const* by_path(std::string_view path) const noexcept {
				for (auto const& entry : files_) {
					if (entry->path() == path) return entry.get();
				}
				return nullptr;
			}

		private:
			std::vector<std::unique_ptr<report_entry>> files_{};
		};

		constexpr uint32_t uint_32(size_t value) {
			return static_cast<uint32_t>(value &
			                             std::numeric_limits<uint32_t>::max());
		}
		static_assert(sizeof(1ull) > 4);
		static_assert(uint_32(std::numeric_limits<size_t>::max()) ==
		              0xFFFF'FFFF);
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
			return {};

		report_files_builder builder{};
		std::vector<std::byte> buffer{};

		for (uint32_t index = 0; index < header.entries_count; ++index) {
			if (!in.load(buffer, entry_size)) return {};
			auto const& entry =
			    *reinterpret_cast<v1::report_entry const*>(buffer.data());

			if (!strings.is_valid(entry.path)) return {};

			builder.add(strings.at(entry.path), entry.stats, entry.contents,
			            entry.line_coverage);
		}

		ec.clear();
		return builder.extract();
	}

#if defined(__GNUC__)
// The warning is legit, since as_a<> can return nullptr, if there is no
// cov::report_files in type tree branch, but this should be called from within
// db_object::store, which is guarded by report_files::recognized
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
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
			}

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
			auto const path32 = uint_32(path);
			if (path != path32 || path32 > stg.size()) return false;

			v1::report_entry entry = {
			    .path = path32,
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

	ref_ptr<report_files> report_files::create(
	    std::vector<std::unique_ptr<report_entry>>&& entries) {
		return make_ref<io::handlers::impl>(std::move(entries));
	}

	report_files_builder& report_files_builder::add(
	    std::unique_ptr<report_entry>&& entry) {
		auto const filename = entry->path();
		entries_[filename] = std::move(entry);
		return *this;
	}

	report_files_builder& report_files_builder::add(
	    std::string_view path,
	    io::v1::coverage_stats const& stats,
	    git_oid const& contents,
	    git_oid const& line_coverage) {
		return add(std::make_unique<io::handlers::report_entry_impl>(
		    path, stats, contents, line_coverage));
	}

	bool report_files_builder::remove(std::string_view path) {
		auto it = entries_.find(path);
		if (it == entries_.end()) return false;
		entries_.erase(it);
		return true;
	}

	ref_ptr<report_files> report_files_builder::extract() {
		return report_files::create(release());
	}

	std::vector<std::unique_ptr<report_entry>> report_files_builder::release() {
		std::vector<std::unique_ptr<report_entry>> entries{};
		entries.reserve(entries_.size());
		std::transform(std::move_iterator(entries_.begin()),
		               std::move_iterator(entries_.end()),
		               std::back_inserter(entries),
		               [](auto&& pair) { return std::move(pair.second); });
		entries_.clear();
		return entries;
	}  // GCOV_EXCL_LINE[GCC]

}  // namespace cov
