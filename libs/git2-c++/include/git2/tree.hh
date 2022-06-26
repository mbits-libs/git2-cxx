// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <git2/tree.h>
#include <git2/blob.hh>
#include <git2/object.hh>

#include <string>

namespace git {
	GIT_PTR_FREE(git_tree);
	GIT_PTR_FREE(git_tree_entry);

	struct tree;
	struct commit;

	template <class GitObject, git_object_t ObjectType>
	struct basic_treeish : object_ptr<GitObject, ObjectType> {
		using object_ptr<GitObject, ObjectType>::object_ptr;

		blob blob_bypath(const char* path) const noexcept {
			return bypath<blob>(path);
		}
		inline tree tree_bypath(const char* path) const noexcept;

	protected:
		template <typename Result>
		Result bypath(const char* path) const noexcept {
			auto* obj = this->get_object();
			return git::create_object<Result>(git_object_lookup_bypath, obj,
			                                  path, Result::OBJECT_TYPE);
		}

	private:
	};

	template <class Holder>
	struct basic_tree_entry : Holder {
		using Holder::Holder;

		git_oid const& oid() const noexcept {
			return *git_tree_entry_id(this->get());
		}

		std::string strid() const noexcept {
			try {
				auto out = std::string(size_t{GIT_OID_HEXSZ}, ' ');
				git_oid_fmt(out.data(), git_tree_entry_id(this->get()));
				return out;
			} catch (std::bad_alloc&) {
				return {};
			}
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
		                   std::string_view id) noexcept;
		static tree lookup(repository_handle repo, git_oid const& id) noexcept;

		size_t count() const noexcept;
		tree_entry_handle entry_byindex(size_t) const noexcept;
		tree_entry entry_bypath(const char* path) const noexcept;
	};

	template <class GitObject, git_object_t ObjectType>
	inline tree basic_treeish<GitObject, ObjectType>::tree_bypath(
	    const char* path) const noexcept {
		return this->bypath<tree>(path);
	}
}  // namespace git
