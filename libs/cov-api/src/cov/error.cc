// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/error.hh>
#include <string>

namespace cov {
	namespace {
		class cov_category : public std::error_category {
		public:
			const char* name() const noexcept override {
				return "cov::category";
			}

			std::string message(int value) const override {
				using enum errc;
				switch (static_cast<errc>(value)) {
					case error:
						return "Generic error";
					case current_branch:
						return "Current branch cannot be deleted";
					case wrong_object_type:
						return "Object cannot be cast to required type";
					case uninitialized_worktree:
						return "This directory is a Git worktree";
					case not_a_worktree:
						return "This directory is expected to be a Git "
						       "worktree, but is not";
					case not_a_branch:
						return "Reference does not points to a branch";
				}
				return "cov::errc{" + std::to_string(value) + "}";
			}

			bool equivalent(int err, const std::error_condition& cond)
			    const noexcept override {
				return cond ==
				       cov::make_error_condition(static_cast<errc>(err));
			}
		};
	}  // namespace

	std::error_category const& category() {
		static cov_category cat;
		return cat;
	}
}  // namespace cov
