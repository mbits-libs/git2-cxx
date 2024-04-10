// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <system_error>

namespace cov {
	enum class errc : int {
		error = -1,
		current_branch = -2,
		wrong_object_type = -3,
		uninitialized_worktree = -4,
		not_a_worktree = -5,
		not_a_branch = -6,
	};

	std::error_category const& category();

	inline std::error_code make_error_code(errc git_error) {
		return std::error_code(static_cast<int>(git_error), cov::category());
	}
	inline std::error_condition make_error_condition(errc git_error) {
		return std::error_condition(static_cast<int>(git_error),
		                            cov::category());
	}
}  // namespace cov

namespace std {
	template <>
	struct is_error_condition_enum<cov::errc> : public std::true_type {};
}  // namespace std
