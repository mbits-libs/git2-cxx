// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include "git2-c++/bytes.hh"
#include "git2-c++/object.hh"
#include "git2/blob.h"

namespace git {
	GIT_PTR_FREE(git_blob);

	struct blob : object_ptr<git_blob, GIT_OBJECT_BLOB> {
		using object_ptr<git_blob, GIT_OBJECT_BLOB>::object_ptr;

		static blob lookup(repository_handle repo,
		                   std::string_view id) noexcept;
		static blob lookup(repository_handle repo, git_oid const& id) noexcept;
		bytes raw() const noexcept;
		git_buf filtered(char const* as_path) const noexcept;
	};
}  // namespace git
