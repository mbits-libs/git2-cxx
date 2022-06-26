// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <git2/commit.hh>

#include <chrono>

namespace git {
	commit commit::lookup(repository_handle repo,
	                      std::string_view id) noexcept {
		return repo.lookup<commit>(id);
	}

	commit commit::lookup(repository_handle repo, git_oid const& id) noexcept {
		return repo.lookup<commit>(id);
	}

	git::tree commit::tree() const noexcept { return peel<git::tree>(); }

	uint64_t commit::commit_time_utc() const noexcept {
		auto const time = git_commit_time(get());
		if (time < 0)
			return 0;
		else
			return static_cast<uint64_t>(time);
	}
}  // namespace git
