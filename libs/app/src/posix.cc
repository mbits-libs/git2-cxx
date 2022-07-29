// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <sys/wait.h>
#include <unistd.h>
#include <args/parser.hpp>
#include <cstdlib>
#include <filesystem>

#ifdef RUNNING_GCOV
extern "C" {
#include <gcov.h>
}
#endif

namespace cov::app::platform {
	std::filesystem::path exec_path() {
		using namespace std::literals;
		std::error_code ec;
		static constexpr std::string_view self_links[] = {
		    "/proc/self/exe"sv,
		    "/proc/curproc/file"sv,
		    "/proc/curproc/exe"sv,
		    "/proc/self/path/a.out"sv,
		};
		for (auto path : self_links) {
			auto link = std::filesystem::read_symlink(path, ec);
			if (!ec) return link;
		}
		[[unlikely]];  // GCOV_EXCL_LINE[POSIX]
		return {};     // GCOV_EXCL_LINE[POSIX]
	}

	std::string con_to_u8(std::error_code const& ec) { return ec.message(); }
}  // namespace cov::app::platform
