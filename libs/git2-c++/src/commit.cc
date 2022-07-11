// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <git2/commit.hh>

#include <chrono>

namespace git {
	commit commit::lookup(repository_handle repo,
	                      std::string_view id,
	                      std::error_code& ec) noexcept {
		return repo.lookup<commit>(id, ec);
	}

	commit commit::lookup(repository_handle repo,
	                      git_oid const& id,
	                      std::error_code& ec) noexcept {
		return repo.lookup<commit>(id, ec);
	}

	git::tree commit::tree(std::error_code& ec) const noexcept {
		return peel<git::tree>(ec);
	}

	sys_seconds commit::commit_time_utc() const noexcept {
		return sys_seconds{std::chrono::seconds{git_commit_time(get())}};
	}
}  // namespace git
