// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/branch.hh>
#include <cov/report.hh>
#include <cov/repository.hh>
#include <cov/tag.hh>
#include <git2/blob.hh>
#include <git2/commit.hh>
#include <git2/error.hh>
#include <git2/repository.hh>
#include "path-utils.hh"

namespace cov {
	namespace {
		class blob_impl : public counted_impl<blob> {
		public:
			blob_impl(git::blob&& ref, cov::origin orig)
			    : ref_{std::move(ref)}, origin_{orig} {}

			git::blob const& peek() const noexcept { return ref_; }
			git::blob peel_and_unlink() noexcept { return std::move(ref_); }
			cov::origin origin() const noexcept { return origin_; }

		private:
			git::blob ref_{};
			cov::origin origin_{};
		};

		git::config open_config(std::filesystem::path const& sysroot,
		                        std::filesystem::path const& common,
		                        std::error_code& ec) {
			auto result = git::config::open_default(sysroot, names::dot_config,
			                                        "cov"sv, ec);
			if (!ec) ec = result.add_local_config(common);
			if (ec) result = nullptr;
			return result;
		}  // GCOV_EXCL_LINE[GCC] -- oom.open_config fires inside here...

		git::repository open_companion_git(std::filesystem::path const& common,
		                                   git::config const& cfg,
		                                   std::error_code& ec) {
			auto gitdir = cfg.get_path(names::core_gitdir);
			if (!gitdir) {
				ec = make_error_code(git::errc::notfound);
				return {};
			}
			return git::repository::open(common / *gitdir, ec);
		}
	}  // namespace

	ref_ptr<blob> blob::wrap(git::blob&& ref, cov::origin orig) {
		return make_ref<blob_impl>(std::move(ref), orig);
	}

	repository::git_repo::git_repo() = default;

	void repository::git_repo::open(std::filesystem::path const& common,
	                                git::config const& cfg,
	                                std::error_code& ec) {
		if (!ec) odb_ = git::odb::open(common / names::objects_dir, ec);
		if (!ec) local_ = git::repository::wrap(odb_, ec);
		if (!ec) git_ = open_companion_git(common, cfg, ec);
		if (ec) {
			odb_ = nullptr;
			local_ = nullptr;
			git_ = nullptr;
		}
	}

	ref_ptr<blob> repository::git_repo::lookup(git_oid const& id,
	                                           std::error_code& ec) const {
		if (auto blob = git_.lookup<git::blob>(id, ec))
			return cov::blob::wrap(std::move(blob), origin::git);
		if (auto blob = local_.lookup<git::blob>(id, ec))
			return cov::blob::wrap(std::move(blob), origin::cov);
		return {};
	}

	bool repository::git_repo::write(git_oid& out, git::bytes const& bytes) {
		return !odb_.write(&out, bytes, GIT_OBJECT_BLOB);
	}

	repository::repository() = default;
	repository::~repository() = default;

	repository::repository(std::filesystem::path const& sysroot,
	                       std::filesystem::path const& common,
	                       std::error_code& ec)
	    : commondir_{common}, cfg_{open_config(sysroot, common, ec)} {
		if (!common.empty()) {
			if (!ec) refs_ = references::make_refs(common);
			if (!ec) db_ = loose_backend_create(common / names::coverage_dir);
			if (!ec) git_.open(common, cfg_, ec);
		}
	}

	ref_ptr<object> repository::dwim(std::string_view name) const {
		auto ref = refs_->dwim(name);
		if (ref) {
			if (ref->references_branch()) return branch::from(std::move(ref));
			if (ref->references_tag()) return tag::from(std::move(ref));
		}
		return ref;
	}

	ref_ptr<reference> repository::create_reference(
	    std::string_view name,
	    git_oid const& target) const {
		return refs_->create(name, target);
	}

	ref_ptr<object> repository::lookup_object(git_oid const& id,
	                                          std::error_code& ec) const {
		if (auto report = db_->lookup_object(id)) return report;
		if (auto blob = git_.lookup(id, ec)) return blob;

		return {};
	}

	bool repository::write(git_oid& out, ref_ptr<object> const& obj) {
		if (is_a<blob>(obj)) return false;
		return db_->write(out, obj);
	}

	std::map<std::string, commit_file_diff> repository::diff_betwen_commits(
	    git_oid const& new_commit,
	    git_oid const& old_commit,
	    std::error_code& ec,
	    git_diff_find_options const* opts) const {
		auto const newer = git::commit::lookup(git_.repo(), new_commit, ec);
		if (ec) return {};
		auto const new_tree = newer.tree(ec);
		if (ec) return {};

		auto const older = git::commit::lookup(git_.repo(), old_commit, ec);
		if (ec) return {};
		auto const old_tree = older.tree(ec);
		if (ec) return {};

		auto const diff = old_tree.diff_to(new_tree, git_.repo(), ec);
		if (ec) return {};

		ec = diff.find_similar(opts);
		if (ec) return {};

		std::map<std::string, commit_file_diff> result{};
		for (auto const& delta : diff.deltas()) {
			if (delta.status != GIT_DELTA_RENAMED &&
			    delta.status != GIT_DELTA_COPIED) {
				continue;
			}
			auto const kind = delta.status == GIT_DELTA_RENAMED
			                      ? file_diff::renamed
			                      : file_diff::copied;
			result[delta.new_file.path] = {.previous_name = delta.old_file.path,
			                               .diff_kind = kind};
		}
		return result;
	}

#define PATH_AND_STATS .filename = std::move(path), .current = entry->stats()
#define RENAMED_PREV(UNUSED) \
	.previous_name = it->second.previous_name, .diff_kind = it->second.diff_kind
#define NO_PREV(KIND) .previous_name = {}, .diff_kind = file_diff::KIND

#define FIND_IN_POOL(KEY, GET_PREV)         \
	auto prev = pool.find(KEY);             \
	if (prev != pool.end()) {               \
		prev->second.second = true;         \
		result.push_back({                  \
		    PATH_AND_STATS,                 \
		    .previous = prev->second.first, \
		    GET_PREV(normal),               \
		});                                 \
		continue;                           \
	}                                       \
                                            \
	result.push_back({                      \
	    PATH_AND_STATS,                     \
	    .previous = {},                     \
	    GET_PREV(added),                    \
	});

	static bool path_sort_less(file_stats const& left,
	                           file_stats const& right) {
		if (auto cmp = left.filename <=> right.filename; cmp != 0)
			return cmp < 0;
		// GCOV_EXCL_START
		[[unlikely]];
		if (auto cmp = left.previous_name <=> right.previous_name; cmp != 0)
			return cmp < 0;
		// equivalent:
		return false;
		// GCOV_EXCL_STOP
	}

	std::vector<file_stats> repository::diff_betwen_reports(
	    ref_ptr<report> const& newer,
	    ref_ptr<report> const& older,
	    std::error_code& ec,
	    git_diff_find_options const* opts) const {
		auto const renames = [&, this, opts] {
			std::error_code ignore{};
			return this->diff_betwen_commits(newer->commit(), older->commit(),
			                                 ignore, opts);
		}();

		auto const new_files =
		    lookup<cov::report_files>(newer->file_list(), ec);
		if (ec) return {};
		auto const old_files =
		    lookup<cov::report_files>(older->file_list(), ec);
		if (ec) return {};

		std::map<std::string, std::pair<io::v1::coverage_stats, bool>> pool{};

		for (auto const& entry : old_files->entries()) {
			auto const path = entry->path();
			pool[{path.data(), path.size()}] = {entry->stats(), false};
		}

		std::vector<file_stats> result{};
		for (auto const& entry : new_files->entries()) {
			auto const path_view = entry->path();
			auto path = std::string{path_view.data(), path_view.size()};
			auto it = renames.find(path);
			if (it != renames.end()) {
				// this is the name and kind to use for pool lookup...
				FIND_IN_POOL(it->second.previous_name, RENAMED_PREV);
				// GCOV_EXCL_START
				// It's highly unlikely to have both a rename and old name
				// missing from the pool
				[[unlikely]];
				continue;
			}  // GCOV_EXCL_STOP

			FIND_IN_POOL(path, NO_PREV);
		}

		for (auto const& [path, info] : pool) {
			auto const& [stats, used] = info;
			if (used) continue;

			result.push_back({
			    .filename = path,
			    .current = {},
			    .previous = stats,
			    NO_PREV(deleted),
			});
		}

		std::stable_sort(result.begin(), result.end(), path_sort_less);
		return result;
	}

	std::vector<file_stats> repository::diff_with_parent(
	    ref_ptr<report> const& current,
	    std::error_code& ec,
	    git_diff_find_options const* opts,
	    file_diff::initial initial_policy) const {
		auto const& parent_oid = current->parent_report();
		ref_ptr<report> parent{};
		if (!git_oid_is_zero(&parent_oid)) {
			std::error_code ignore{};
			parent = lookup<report>(parent_oid, ignore);
		}

		if (parent) return diff_betwen_reports(current, parent, ec, opts);

		if (initial_policy == file_diff::initial_with_self)
			return diff_betwen_reports(current, current, ec);

		auto const files = lookup<cov::report_files>(current->file_list(), ec);
		if (ec) return {};

		std::vector<file_stats> result{};
		for (auto const& entry : files->entries()) {
			auto const path = entry->path();

			result.push_back({
			    .filename = {path.data(), path.size()},
			    .current = entry->stats(),
			    .previous = {},
			    NO_PREV(added),
			});
		}

		std::stable_sort(result.begin(), result.end(), path_sort_less);
		return result;
	}

#undef PATH_AND_STATS
#undef RENAMED_PREV
#undef NO_PREV
#undef FIND_IN_POOL
}  // namespace cov
