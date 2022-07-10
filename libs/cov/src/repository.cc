// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/branch.hh>
#include <cov/repository.hh>
#include <cov/tag.hh>
#include <git2/blob.hh>
#include <git2/error.hh>
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

		git::config open_config(std::filesystem::path const& common,
		                        std::error_code& ec) {
			auto result =
			    git::config::open_default(names::dot_config, "cov"sv, ec);
			if (!ec) ec = result.add_local_config(common);
			if (ec) result = nullptr;
			return result;
		}
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
	                                           std::error_code& ec) {
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

	repository::repository(std::filesystem::path const& common,
	                       std::error_code& ec)
	    : commondir_{common}, cfg_{open_config(common, ec)} {
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

	ref_ptr<object> repository::lookup_object(git_oid const& id,
	                                          std::error_code& ec) {
		if (auto report = db_->lookup_object(id)) return report;
		if (auto blob = git_.lookup(id, ec)) return blob;

		return {};
	}

	bool repository::write(git_oid& out, ref_ptr<object> const& obj) {
		if (is_a<blob>(obj)) return false;
		return db_->write(out, obj);
	}

}  // namespace cov
