// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <git2/tree.h>
#include <cov/git2/blob.hh>
#include <cov/git2/diff.hh>
#include <cov/git2/object.hh>

#include <string>

namespace git {
	GIT_PTR_FREE(git_tree);
	GIT_PTR_FREE(git_tree_entry);
	GIT_PTR_FREE(git_treebuilder);

	struct tree;
	struct commit;

	template <class GitObject, git_object_t ObjectType>
	struct basic_treeish : object_ptr<GitObject, ObjectType> {
		using object_ptr<GitObject, ObjectType>::object_ptr;

		blob blob_bypath(const char* path, std::error_code& ec) const noexcept {
			return bypath<blob>(path, ec);
		}
		inline tree tree_bypath(const char* path,
		                        std::error_code& ec) const noexcept;

	protected:
		template <typename Result>
		Result bypath(const char* path, std::error_code& ec) const noexcept {
			auto* obj = this->get_object();
			return git::create_object<Result>(ec, git_object_lookup_bypath, obj,
			                                  path, Result::OBJECT_TYPE);
		}

	private:
	};

	template <class Holder>
	struct basic_tree_entry : Holder {
		using Holder::Holder;  // GCOV_EXCL_LINE - ctor seems to be inlined away

		git::oid_view oid() const noexcept {
			return *git_tree_entry_id(this->get());
		}

		std::string strid() const noexcept {
			try {
				auto out = std::string(size_t{GIT_OID_HEXSZ}, ' ');
				git_oid_fmt(out.data(), git_tree_entry_id(this->get()));
				return out;
			} catch (std::bad_alloc&) {
				return {};
			}  // GCOV_EXCL_LINE[WIN32]
		}

		std::string_view name() const noexcept {
			return git_tree_entry_name(this->get());
		}

	protected:
		auto get() const { return Holder::get(); }
	};

	using tree_entry_handle = basic_tree_entry<handle<git_tree_entry const>>;
	using tree_entry = basic_tree_entry<ptr<git_tree_entry>>;

	struct tree : basic_treeish<git_tree, GIT_OBJECT_TREE> {
		using basic_treeish<git_tree, GIT_OBJECT_TREE>::basic_treeish;
		using basic_treeish<git_tree, GIT_OBJECT_TREE>::operator bool;

		static tree lookup(repository_handle repo,
		                   std::string_view id,
		                   std::error_code& ec) noexcept;
		static tree lookup(repository_handle repo,
		                   git::oid_view id,
		                   std::error_code& ec) noexcept;

		size_t count() const noexcept;
		tree_entry_handle entry_byindex(size_t) const noexcept;
		tree_entry entry_bypath(const char* path,
		                        std::error_code& ec) const noexcept;

		diff diff_to(tree const& new_tree,
		             repository_handle repo,
		             std::error_code& ec) const noexcept {
			return diff_to(new_tree, repo, nullptr, ec);
		}

		diff diff_to(tree const& new_tree,
		             repository_handle repo,
		             git_diff_options const* opts,
		             std::error_code& ec) const noexcept;

		friend struct treebuilder;
	};

	template <class GitObject, git_object_t ObjectType>
	inline tree basic_treeish<GitObject, ObjectType>::tree_bypath(
	    const char* path,
	    std::error_code& ec) const noexcept {
		return this->bypath<tree>(path, ec);
	}

	struct treebuilder : ptr<git_treebuilder> {
		using ptr<git_treebuilder>::ptr;

		static treebuilder create(std::error_code& ec,
		                          repository_handle repo,
		                          tree const& tree = {}) {
			return create_handle<treebuilder>(ec, git_treebuilder_new,
			                                  repo.get(), tree.get());
		}

		std::error_code insert(char const* filename,
		                       git::oid_view id,
		                       git_filemode_t filemode) {
			return as_error(git_treebuilder_insert(nullptr, get(), filename,
			                                       id.ref, filemode));
		}

		std::error_code write(git::oid& id) {
			return as_error(git_treebuilder_write(&id.id, get()));
		}
	};
}  // namespace git
