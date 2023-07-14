// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/git2/commit.hh>

#include <chrono>

namespace git {
	commit commit::lookup(repository_handle repo,
	                      std::string_view id,
	                      std::error_code& ec) noexcept {
		return repo.lookup<commit>(id, ec);
	}

	commit commit::lookup(repository_handle repo,
	                      oid_view id,
	                      std::error_code& ec) noexcept {
		return repo.lookup<commit>(id, ec);
	}

	git::tree commit::tree(std::error_code& ec) const noexcept {
		return peel<git::tree>(ec);
	}

	sys_seconds commit::commit_time_utc() const noexcept {
		return sys_seconds{std::chrono::seconds{git_commit_time(get())}};
	}

	commit::signature commit::committer() const noexcept {
		auto action = git_commit_committer(get());
		return {
		    .name = action->name,
		    .email = action->email,
		    .when = sys_seconds{std::chrono::seconds{action->when.time}},
		};
	}

	commit::signature commit::author() const noexcept {
		auto action = git_commit_author(get());
		return {
		    .name = action->name,
		    .email = action->email,
		    .when = sys_seconds{std::chrono::seconds{action->when.time}},
		};
	}

	char const* commit::message_raw() const noexcept {
		return git_commit_message_raw(get());
	}
}  // namespace git
