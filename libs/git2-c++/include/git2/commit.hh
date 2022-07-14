// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <git2/commit.h>
#include <chrono>
#include <git2/object.hh>
#include <git2/tree.hh>

#include <string>

namespace git {
	GIT_PTR_FREE(git_commit);

	// using sys_seconds = std::chrono::time_point<std::chrono::system_clock,
	// std::chrono::seconds>;
	using sys_seconds = std::chrono::sys_seconds;

	struct commit : basic_treeish<git_commit, GIT_OBJECT_COMMIT> {
		// GCOV_EXCL_START - ctor seems to be inlined away
		using basic_treeish<git_commit, GIT_OBJECT_COMMIT>::basic_treeish;
		// GCOV_EXCL_STOP

		struct signature {
			std::string_view name{};
			std::string_view email{};
			sys_seconds when{};
		};

		static commit lookup(repository_handle repo,
		                     std::string_view id,
		                     std::error_code& ec) noexcept;
		static commit lookup(repository_handle repo,
		                     git_oid const& id,
		                     std::error_code& ec) noexcept;
		git::tree tree(std::error_code& ec) const noexcept;
		sys_seconds commit_time_utc() const noexcept;
		signature committer() const noexcept;
		signature author() const noexcept;
		char const* message_raw() const noexcept;
	};
}  // namespace git
