// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/git2/blob.hh>
#include <cov/io/files.hh>
#include <cov/io/strings.hh>
#include <cov/io/types.hh>
#include <cov/repository.hh>

namespace cov::io::handlers {
	namespace {
		struct entry_impl : cov::files::entry {
			entry_impl(std::string_view path,
			           io::v1::coverage_stats const& stats,
			           git::oid_view contents,
			           git::oid_view line_coverage,
			           git::oid_view function_coverage,
			           git::oid_view branch_coverage)
			    : path_{path.data(), path.size()}
			    , stats_{stats}
			    , contents_{contents.oid()}
			    , line_coverage_{line_coverage.oid()}
			    , function_coverage_{function_coverage.oid()}
			    , branch_coverage_{branch_coverage.oid()} {}

			~entry_impl() = default;
			std::string_view path() const noexcept override { return path_; }
			io::v1::coverage_stats const& stats() const noexcept override {
				return stats_;
			}
			git::oid const& contents() const noexcept override {
				return contents_;
			}
			git::oid const& line_coverage() const noexcept override {
				return line_coverage_;
			}
			git::oid const& function_coverage() const noexcept override {
				return function_coverage_;
			}
			git::oid const& branch_coverage() const noexcept override {
				return branch_coverage_;
			}
			std::vector<std::byte> get_contents(
			    repository const& repo,
			    std::error_code& ec) const noexcept override {
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
			git::oid contents_{};
			git::oid line_coverage_{};
			git::oid function_coverage_{};
			git::oid branch_coverage_{};
		};

		struct impl : counted_impl<cov::files> {
			explicit impl(
			    std::vector<std::unique_ptr<cov::files::entry>>&& files)
			    : files_{std::move(files)} {}

			std::span<std::unique_ptr<cov::files::entry> const> entries()
			    const noexcept override {
				return files_;
			}

			cov::files::entry const* by_path(
			    std::string_view path) const noexcept override {
				for (auto const& entry : files_) {
					if (entry->path() == path) return entry.get();
				}
				return nullptr;
			}

		private:
			std::vector<std::unique_ptr<cov::files::entry>> files_{};
		};

		constexpr uint32_t uint_32(size_t value) {
			return static_cast<uint32_t>(value &
			                             std::numeric_limits<uint32_t>::max());
		}
		static_assert(sizeof(1ull) > 4);
		static_assert(uint_32(std::numeric_limits<size_t>::max()) ==
		              0xFFFF'FFFF);
	}  // namespace

	ref_ptr<counted> files::load(uint32_t,
	                             uint32_t,
	                             git::oid_view,
	                             read_stream& in,
	                             std::error_code& ec) const {
		ec = make_error_code(errc::bad_syntax);
		v1::files header{};
		if (!in.load(header)) {
			return {};
		}
		auto const entry_size = header.entries.size * sizeof(uint32_t);
		if (entry_size < sizeof(v1::files::basic) ||
		    header.strings.offset < sizeof(v1::files) / sizeof(uint32_t) ||
		    header.entries.offset < header.strings.offset) {
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

		cov::files::builder builder{};
		std::vector<std::byte> buffer{};

		static const git_oid zero_id{};

		for (uint32_t index = 0; index < header.entries.count; ++index) {
			if (!in.load(buffer, entry_size)) {
				return {};
			}
			auto const& entry_v0 =
			    *reinterpret_cast<v1::files::basic const*>(buffer.data());

			if (!strings.is_valid(entry_v0.path)) {
				return {};
			}

			if (entry_size < sizeof(v1::files::ext)) {
				builder.add(strings.at(entry_v0.path),
				            {entry_v0.lines_total, entry_v0.lines.summary,
				             io::v1::stats::init(), io::v1::stats::init()},
				            entry_v0.contents, entry_v0.lines.details, zero_id,
				            zero_id);
			} else {
				auto const& entry_v1 =
				    *reinterpret_cast<v1::files::ext const*>(buffer.data());
				builder.add(
				    strings.at(entry_v1.path),
				    {entry_v1.lines_total, entry_v1.lines.summary,
				     entry_v1.functions.summary, entry_v1.branches.summary},
				    entry_v1.contents, entry_v1.lines.details,
				    entry_v1.functions.details, entry_v1.branches.details);
			}
		}

		ec.clear();
		return builder.extract();
	}

#if defined(__GNUC__)
// The warning is legit, since as_a<> can return nullptr, if there is no
// cov::files in type tree branch, but this should be called from within
// db_object::store, which is guarded by files::recognized
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#endif
	template <typename EntryType>
	struct init_entry;
	template <>
	struct init_entry<v1::files::basic> {
		static v1::files::basic get(cov::files::entry const& in, str path32) {
			auto const& stats = in.stats();
			return {
			    .path = path32,
			    .contents = in.contents().id,
			    .lines_total = stats.lines_total,
			    .lines = {.summary = stats.lines,
			              .details = in.line_coverage().id},
			};
		}
	};
	template <>
	struct init_entry<v1::files::ext> {
		static v1::files::ext get(cov::files::entry const& in, str path32) {
			auto const& stats = in.stats();
			return {
			    .path = path32,
			    .contents = in.contents().id,
			    .lines_total = stats.lines_total,
			    .lines = {.summary = stats.lines,
			              .details = in.line_coverage().id},
			    .functions = {.summary = stats.functions,
			                  .details = in.function_coverage().id},
			    .branches = {.summary = stats.branches,
			                 .details = in.branch_coverage().id},
			};
		}
	};

	template <typename EntryType>
	bool store_entry(cov::files::entry const& in,
	                 str path32,
	                 write_stream& out) {
		auto const entry = init_entry<EntryType>::get(in, path32);
		return out.store(entry);
	}

	bool files::store(ref_ptr<counted> const& value, write_stream& out) const {
		auto const obj =
		    as_a<cov::files>(static_cast<object const*>(value.get()));
		auto entries = obj->entries();

		auto only_lines = true;
		for (auto const& entry_ptr : entries) {
			auto& entry = *entry_ptr;
			if (!entry.function_coverage().is_zero() ||
			    !entry.branch_coverage().is_zero()) {
				only_lines = false;
				break;
			}
		}

		auto stg = [&] {
			strings_builder strings{};
			for (auto const& entry_ptr : entries) {
				auto& entry = *entry_ptr;
				strings.insert(entry.path());
			}

			return strings.build();
		}();

		v1::files hdr{
		    .strings = stg.after<v1::files>(),
		    .entries = only_lines
		                   ? stg.align_array<v1::files, v1::files::basic>(
		                         entries.size())
		                   : stg.align_array<v1::files, v1::files::ext>(
		                         entries.size()),
		};

		if (!out.store(hdr)) {
			return false;
		}
		if (!out.store({stg.data(), stg.size()})) {
			return false;
		}

		for (auto const& entry_ptr : entries) {
			auto& in = *entry_ptr;

			auto const path = stg.locate_or(in.path(), stg.size() + 1);
			auto const path32 = uint_32(path);
			if (path != path32 || path32 > stg.size()) {
				return false;
			}
			auto const str32 = static_cast<str>(path32);

			if (only_lines) {
				if (!store_entry<v1::files::basic>(in, str32, out)) {
					return false;
				}
			} else {
				if (!store_entry<v1::files::ext>(in, str32, out)) {
					return false;
				}
			}
		}

		return true;
	}
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
}  // namespace cov::io::handlers

namespace cov {
	files::entry::~entry() {}

	ref_ptr<files> files::create(
	    std::vector<std::unique_ptr<files::entry>>&& entries) {
		return make_ref<io::handlers::impl>(std::move(entries));
	}

	files::builder& files::builder::add(std::unique_ptr<entry>&& entry) {
		auto const filename = entry->path();
		entries_[filename] = std::move(entry);
		return *this;
	}

	files::builder& files::builder::add(std::string_view path,
	                                    io::v1::coverage_stats const& stats,
	                                    git::oid_view contents,
	                                    git::oid_view line_coverage,
	                                    git::oid_view function_coverage,
	                                    git::oid_view branch_coverage) {
		return add(std::make_unique<io::handlers::entry_impl>(
		    path, stats, contents, line_coverage, function_coverage,
		    branch_coverage));
	}

	bool files::builder::remove(std::string_view path) {
		auto it = entries_.find(path);
		if (it == entries_.end()) return false;
		entries_.erase(it);
		return true;
	}

	ref_ptr<files> files::builder::extract() {
		return files::create(release());
	}

	std::vector<std::unique_ptr<files::entry>> files::builder::release() {
		std::vector<std::unique_ptr<files::entry>> entries{};
		entries.reserve(entries_.size());
		std::transform(std::move_iterator(entries_.begin()),
		               std::move_iterator(entries_.end()),
		               std::back_inserter(entries),
		               [](auto&& pair) { return std::move(pair.second); });
		entries_.clear();
		return entries;
	}  // GCOV_EXCL_LINE[GCC]
}  // namespace cov
