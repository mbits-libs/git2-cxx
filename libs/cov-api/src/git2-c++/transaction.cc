// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/git2/transaction.hh>

namespace git {
	std::error_code transaction::commit() const noexcept {
		return as_error(git_transaction_commit(get()));
	}
}  // namespace git
