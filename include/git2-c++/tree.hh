#pragma once
#include "git2/tree.h"
#include "git2-c++/object.hh"
#include "git2-c++/blob.hh"

#include <string>

namespace git {
	GIT_PTR_FREE(git_tree);
	GIT_PTR_FREE(git_tree_entry);

	struct tree;
	struct commit;

	template <class GitObject, git_object_t ObjectType>
	struct basic_treeish : object_ptr<GitObject, ObjectType> {
		using object_ptr<GitObject, ObjectType>::object_ptr;

		blob blob_bypath(const char* path) const noexcept { return bypath<blob>(path); }
		inline tree tree_bypath(const char* path) const noexcept;
	protected:
		template <typename Result>
		Result bypath(const char* path) const noexcept {
			auto* obj = this->get_object();
			return git::create_object<Result>(
				git_object_lookup_bypath,
				obj, path, Result::OBJECT_TYPE
				);
		}
	private:
	};

	struct tree_entry : ptr<git_tree_entry> {
		using ptr<git_tree_entry>::ptr;

		git_oid const& oid() const noexcept;
		std::string strid() const noexcept;
	};

	struct tree : basic_treeish<git_tree, GIT_OBJECT_TREE> {
		using basic_treeish<git_tree, GIT_OBJECT_TREE>::basic_treeish;
		using basic_treeish<git_tree, GIT_OBJECT_TREE>::operator bool;

		static tree lookup(repository_handle repo, std::string_view id) noexcept;
		tree_entry entry_bypath(const char* path) const noexcept;
	};

	template <class GitObject, git_object_t ObjectType>
	inline tree basic_treeish<GitObject, ObjectType>::tree_bypath(const char* path) const noexcept {
		return this->bypath<tree>(path);
	}
}