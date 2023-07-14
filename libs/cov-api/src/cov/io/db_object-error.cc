// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/io/db_object.hh>

namespace cov::io {
	namespace {
		class git_category : public std::error_category {
		public:
			const char* name() const noexcept override {
				return "cov::io::category";
			}

			std::string message(int value) const override {
				switch (static_cast<errc>(value)) {
					case errc::bad_syntax:
						return "Unrecognizable syntax";
					case errc::unknown_magic:
						return "Magic value not registered";
					case errc::unsupported_version:
						return "Version not supported";
				}
				return "cov::io::errc{" + std::to_string(value) + "}";
			}

			bool equivalent(int err, const std::error_condition& cond)
			    const noexcept override {
				return cond == make_error_condition(static_cast<errc>(err));
			}
		};
	}  // namespace

	std::error_category const& category() {
		static git_category cat;
		return cat;
	}

}  // namespace cov::io
