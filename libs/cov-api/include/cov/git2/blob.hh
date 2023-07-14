// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <git2/blob.h>
#include <cov/git2/bytes.hh>
#include <cov/git2/object.hh>

namespace git {
	GIT_PTR_FREE(git_blob);

	struct blob : object_ptr<git_blob, GIT_OBJECT_BLOB> {
		// GCOV_EXCL_START - ctor seems to be inlined away
		using object_ptr<git_blob, GIT_OBJECT_BLOB>::object_ptr;
		// GCOV_EXCL_STOP

		static blob lookup(repository_handle repo,
		                   std::string_view id,
		                   std::error_code& ec) noexcept;
		static blob lookup(repository_handle repo,
		                   git_oid const& id,
		                   std::error_code& ec) noexcept;
		bytes raw() const noexcept;
		git_buf filtered(char const* as_path,
		                 std::error_code& ec) const noexcept;
	};
}  // namespace git
