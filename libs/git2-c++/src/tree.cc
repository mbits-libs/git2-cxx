// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <git2/tree.hh>

namespace git {
	tree tree::lookup(repository_handle repo, std::string_view id) noexcept {
		return repo.lookup<tree>(id);
	}

	tree tree::lookup(repository_handle repo, git_oid const& id) noexcept {
		return repo.lookup<tree>(id);
	}

	size_t tree::count() const noexcept { return git_tree_entrycount(get()); }

	tree_entry_handle tree::entry_byindex(size_t index) const noexcept {
		return tree_entry_handle{git_tree_entry_byindex(get(), index)};
	}

	tree_entry tree::entry_bypath(const char* path) const noexcept {
		return git::create_handle<tree_entry>(git_tree_entry_bypath, get(),
		                                      path);
	}
}  // namespace git
