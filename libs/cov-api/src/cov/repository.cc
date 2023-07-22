// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/branch.hh>
#include <cov/git2/blob.hh>
#include <cov/git2/commit.hh>
#include <cov/git2/error.hh>
#include <cov/git2/repository.hh>
#include <cov/report.hh>
#include <cov/repository.hh>
#include <cov/tag.hh>
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

	ref_ptr<blob> repository::git_repo::lookup(git::oid_view id,
	                                           std::error_code& ec) const {
		if (auto blob = git_.lookup<git::blob>(id, ec))
			return cov::blob::wrap(std::move(blob), origin::git);
		if (auto blob = local_.lookup<git::blob>(id, ec))
			return cov::blob::wrap(std::move(blob), origin::cov);
		return {};
	}

	bool repository::git_repo::write(git::oid& out, git::bytes const& bytes) {
		return !odb_.write(out, bytes, GIT_OBJECT_BLOB);
	}

	repository::repository() = default;
	repository::repository(repository&&) = default;
	repository& repository::operator=(repository&&) = default;
	repository::~repository() = default;

	repository::repository(std::filesystem::path const& sysroot,
	                       std::filesystem::path const& common,
	                       std::error_code& ec)
	    : commondir_{common}, cfg_{open_config(sysroot, common, ec)} {
		if (!common.empty()) {
			if (!ec) refs_ = references::make_refs(common);
			if (!ec) db_ = backend::loose_backend(common / names::coverage_dir);
			if (!ec) git_.open(common, cfg_, ec);
		}
	}

	current_head_type repository::current_head() const {
		auto const HEAD = dwim(names::HEAD);
		auto const head_ref = as_a<cov::reference>(HEAD);

		if (!head_ref) return {};

		if (head_ref->direct_target())
			return {.tip = *head_ref->direct_target(), .ref = head_ref};

		auto const peeled = head_ref->peel_target();
		auto const name = peeled->shorthand();

		if (peeled->references_branch() && peeled->direct_target()) {
			return {.branch = {name.data(), name.size()},
			        .tip = *peeled->direct_target(),
			        .ref = peeled};
		}
		if (peeled->references_branch())
			return {.branch = {name.data(), name.size()}, .ref = peeled};

		if (peeled->direct_target())
			return {.tip = *peeled->direct_target(), .ref = peeled};

		return {.ref = peeled};
	}  // GCOV_EXCL_LINE[WIN32]

	bool repository::update_current_head(git::oid_view ref,
	                                     current_head_type const& known) const {
		if (known != current_head()) return false;
		std::string name{};
		if (known.branch.empty()) {
			name.assign(names::HEAD);
		} else {
			name.reserve(names::heads_dir_prefix.size() + known.branch.size());
			name.append(names::heads_dir_prefix);
			name.append(known.branch);
		}
		refs_->create(name, *ref.ref);
		return true;
	}

	ref_ptr<object> repository::dwim(std::string_view name) const {
		auto ref = refs_->dwim(name);
		if (ref) {
			if (ref->references_branch()) return branch::from(std::move(ref));
			if (ref->references_tag()) return tag::from(std::move(ref));
		}
		return ref;
	}

	ref_ptr<object> repository::find_partial(std::string_view partial) const {
		git_oid oid{};
		auto const length = (std::min)(partial.size(), size_t{GIT_OID_HEXSZ});
		if (git_oid_fromstrn(&oid, partial.data(), length)) return {};
		return find_partial(oid, length);
	}

	ref_ptr<object> repository::find_partial(git::oid_view in,
	                                         size_t character_count) const {
		return db_->lookup<object>(in, character_count);
	}

	ref_ptr<object> repository::lookup_object(git::oid_view id,
	                                          std::error_code& ec) const {
		if (auto report = db_->lookup_object(id)) return report;
		if (auto blob = git_.lookup(id, ec)) return blob;

		return {};
	}

	bool repository::write(git::oid& out, ref_ptr<object> const& obj) {
		if (is_a<blob>(obj)) return false;
		return db_->write(out, obj);
	}

	std::map<std::string, commit_file_diff> repository::diff_betwen_commits(
	    git::oid_view new_commit,
	    git::oid_view old_commit,
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
	}  // GCOV_EXCL_STOP

	struct file_pool {
		struct file_info {
			io::v1::coverage_stats stats;
			git::oid functions;
			bool used;
		};
		std::map<std::string, file_info> items{};

		static file_pool from(
		    std::span<std::unique_ptr<cov::files::entry> const> const&
		        old_entries) {
			file_pool result{};

			for (auto const& entry : old_entries) {
				auto const path = entry->path();
				result.items[{path.data(), path.size()}] = {
				    entry->stats(), entry->function_coverage(), false};
			}

			return result;
		}  // GCOV_EXCL_LINE[GCC]

		void apply(std::span<std::unique_ptr<cov::files::entry> const> const&
		               new_entries,
		           std::map<std::string, cov::commit_file_diff> const& renames,
		           std::vector<file_stats>& result) {
			for (auto const& entry : new_entries) {
				auto const path_view = entry->path();
				auto path = std::string{path_view.data(), path_view.size()};
				auto it = renames.find(path);
				if (it != renames.end()) {
					// this is the name and kind to use for pool lookup...
					result.push_back(match(
					    it->second.previous_name, std::move(path), entry,
					    [it](cov::file_stats& next, cov::file_diff::kind) {
						    next.previous_name = it->second.previous_name;
						    next.diff_kind = it->second.diff_kind;
					    }));
					continue;
				}

				{
					auto copy = path;
					result.push_back(
					    match(copy, std::move(path), entry,
					          [](file_stats& info, file_diff::kind kind) {
						          info.diff_kind = kind;
					          }));
				}
			}
		}

		void get_deleted(std::vector<file_stats>& result) {
			for (auto const& [path, info] : items) {
				auto const& [stats, prev_oid, used] = info;
				if (used) continue;

				result.push_back({
				    .filename = path,
				    .current = {},
				    .previous = stats,
				    .previous_name = {},
				    .diff_kind = file_diff::deleted,
				});
			}
		}

		template <typename Callback>
		file_stats match(std::string const& key,
		                 std::string&& path,
		                 std::unique_ptr<cov::files::entry> const& entry,
		                 Callback&& get_prev) {
			file_stats result{
			    .filename = std::move(path),
			    .current = entry->stats(),
			    .previous = {},
			    .current_functions = entry->function_coverage(),
			    .previous_functions = {},
			};

			auto kind = file_diff::added;

			auto prev = items.find(key);
			if (prev != items.end()) {
				prev->second.used = true;
				result.previous = prev->second.stats;
				result.previous_functions = prev->second.functions;
				kind = file_diff::normal;
			}
			get_prev(result, kind);
			return result;
		}  // GCOV_EXCL_LINE[GCC]
	};

	std::vector<file_stats> repository::diff_betwen_reports(
	    ref_ptr<report> const& newer,
	    ref_ptr<report> const& older,
	    std::error_code& ec,
	    git_diff_find_options const* opts) const {
		auto const renames = [&, this, opts] {  // GCOV_EXCL_LINE[GCC]
			std::error_code ignore{};
			return this->diff_betwen_commits(newer->commit_id(),
			                                 older->commit_id(), ignore, opts);
		}();

		auto const new_files = lookup<cov::files>(newer->file_list_id(), ec);
		if (ec) return {};
		auto const old_files = lookup<cov::files>(older->file_list_id(), ec);
		if (ec) return {};

		std::vector<file_stats> result{};

		auto pool = file_pool::from(old_files->entries());
		pool.apply(new_files->entries(), renames, result);
		pool.get_deleted(result);

		std::stable_sort(result.begin(), result.end(), path_sort_less);
		return result;
	}

	std::vector<file_stats> repository::diff_with_parent(
	    ref_ptr<report> const& current,
	    std::error_code& ec,
	    git_diff_find_options const* opts,
	    file_diff::initial initial_policy) const {
		auto const& parent_oid = current->parent_id();
		ref_ptr<report> parent{};
		if (!parent_oid.is_zero()) {
			std::error_code ignore{};
			parent = lookup<report>(parent_oid, ignore);
		}

		if (parent) return diff_betwen_reports(current, parent, ec, opts);

		if (initial_policy == file_diff::initial_with_self)
			return diff_betwen_reports(current, current, ec);

		auto const files = lookup<cov::files>(current->file_list_id(), ec);
		if (ec) return {};

		std::vector<file_stats> result{};
		for (auto const& entry : files->entries()) {
			auto const path = entry->path();

			result.push_back({
			    .filename = {path.data(), path.size()},
			    .current = entry->stats(),
			    .previous = {},
			    .previous_name = {},
			    .diff_kind = file_diff::added,
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
