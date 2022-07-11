// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <git2/tree.hh>

namespace git {
	tree tree::lookup(repository_handle repo,
	                  std::string_view id,
	                  std::error_code& ec) noexcept {
		return repo.lookup<tree>(id, ec);
	}

	tree tree::lookup(repository_handle repo,
	                  git_oid const& id,
	                  std::error_code& ec) noexcept {
		return repo.lookup<tree>(id, ec);
	}

	size_t tree::count() const noexcept { return git_tree_entrycount(get()); }

	tree_entry_handle tree::entry_byindex(size_t index) const noexcept {
		return tree_entry_handle{git_tree_entry_byindex(get(), index)};
	}

	tree_entry tree::entry_bypath(const char* path,
	                              std::error_code& ec) const noexcept {
		return git::create_handle<tree_entry>(ec, git_tree_entry_bypath, get(),
		                                      path);
	}

	diff tree::diff_to(tree const& other,
	                   repository_handle repo,
	                   git_diff_options const* opts,
	                   std::error_code& ec) const noexcept {
		return git::create_handle<diff>(ec, git_diff_tree_to_tree, repo.get(),
		                                get(), other.get(), opts);
	}

}  // namespace git
