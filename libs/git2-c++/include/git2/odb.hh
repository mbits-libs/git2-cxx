// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <git2/odb.h>
#include <git2/bytes.hh>
#include <git2/ptr.hh>

#include <filesystem>
#include <string>

namespace git {
	GIT_PTR_FREE(git_odb);

	struct odb : ptr<git_odb> {
		using ptr<git_odb>::ptr;

		static odb create();
		static odb open(std::filesystem::path const&);
		static void hash(git_oid*, bytes const&, git_object_t) noexcept;
		bool exists(git_oid const&) const noexcept;
		bool write(git_oid*, bytes const&, git_object_t) const noexcept;

	private:
		friend struct repository;
	};
}  // namespace git
