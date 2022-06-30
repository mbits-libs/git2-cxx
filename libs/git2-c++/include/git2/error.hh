// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <system_error>

namespace git {
	enum class errc : int {
		error = -1,
		notfound = -3,
		exists = -4,
		ambiguous = -5,
		bufs = -6,
		user = -7,
		barerepo = -8,
		unbornbranch = -9,
		unmerged = -10,
		nonfastforward = -11,
		invalidspec = -12,
		conflict = -13,
		locked = -14,
		modified = -15,
		auth = -16,
		certificate = -17,
		applied = -18,
		peel = -19,
		eof = -20,
		invalid = -21,
		uncommitted = -22,
		directory = -23,
		mergeconflict = -24,
		passthrough = -30,
		iterover = -31,
		retry = -32,
		mismatch = -33,
		indexdirty = -34,
		applyfail = -35,
	};

	std::error_category const& category();

	inline std::error_code make_error_code(int git_error) {
		return std::error_code(git_error, git::category());
	}
	inline std::error_code make_error_code(errc git_error) {
		return std::error_code(static_cast<int>(git_error), git::category());
	}
	inline std::error_condition make_error_condition(errc git_error) {
		return std::error_condition(static_cast<int>(git_error),
		                            git::category());
	}
}  // namespace git

namespace std {
	template <>
	struct is_error_condition_enum<git::errc> : public std::true_type {};
}  // namespace std
