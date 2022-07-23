// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <git2/transaction.h>
#include <git2/ptr.hh>

namespace git {
	GIT_PTR_FREE(git_transaction);

	struct transaction : ptr<git_transaction> {
		using ptr<git_transaction>::ptr;
		std::error_code commit() const noexcept;
	};
}  // namespace git
