// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <git2/commit.h>
#include <git2/object.hh>
#include <git2/tree.hh>

#include <string>

namespace git {
	GIT_PTR_FREE(git_commit);

	struct commit : basic_treeish<git_commit, GIT_OBJECT_COMMIT> {
		using basic_treeish<git_commit, GIT_OBJECT_COMMIT>::basic_treeish;

		static commit lookup(repository_handle repo,
		                     std::string_view id) noexcept;
		static commit lookup(repository_handle repo,
		                     git_oid const& id) noexcept;
		git::tree tree() const noexcept;
		uint64_t commit_time_utc() const noexcept;
	};
}  // namespace git
