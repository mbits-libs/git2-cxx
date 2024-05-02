// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include "platform.hh"
#include <fmt/format.h>
#include <cov/app/dirs.hh>

using namespace std::literals;

namespace cov::app::platform {
	namespace {
		template <size_t DotDot>
		struct parent_path {
			static std::filesystem::path get(
			    std::filesystem::path const& dirname) {
				return parent_path<DotDot - 1>::get(dirname).parent_path();
			}
		};
		template <>
		struct parent_path<0> {
			static std::filesystem::path get(
			    std::filesystem::path const& dirname) {
				return dirname;
			}
		};

		template <size_t DotDot>
		std::filesystem::path const& sys_root_impl() {
			static auto const root = [] {
				auto const dir = exec_path().parent_path();
				return parent_path<DotDot>::get(dir);
			}();

			return root;
		}
	}  // namespace

	std::filesystem::path locale_dir_provider::locale_dir_inside(
	    std::filesystem::path const& sys_root) {
		return sys_root / directory_info::share / "locale"sv;
	}  // GCOV_EXCL_LINE

	std::filesystem::path const& filters::sys_root() {
		// share/cov-XY/filters
		return sys_root_impl<3>();
	}

	std::filesystem::path const& core_extensions::sys_root() {
		// libexec/cov
		return sys_root_impl<2>();
	}

	std::vector<char8_t> read_input() {
		std::vector<char8_t> result{};
		char8_t buffer[8192];

		while (auto read = std::fread(buffer, 1, sizeof(buffer), stdin)) {
			result.insert(result.end(), buffer, buffer + read);
		}

		return result;
	}  // GCOV_EXCL_LINE
};     // namespace cov::app::platform
