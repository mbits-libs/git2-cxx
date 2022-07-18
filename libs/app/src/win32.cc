// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#define NOMINMAX

#include <Windows.h>
#include <errno.h>
#include <args/parser.hpp>
#include <filesystem>
#include <string_view>
#include <vector>
#include "fmt/format.h"

namespace cov::app::platform {
	std::filesystem::path exec_path() {
		wchar_t modpath[2048];
		GetModuleFileNameW(nullptr, modpath,
		                   sizeof(modpath) / sizeof(modpath[0]));
		return modpath;
	}

	std::string con_to_u8(std::error_code const& ec) {
		auto const msg = ec.message();
		auto const CP_OEM = CP_ACP;

		auto const oem_length = static_cast<int>(msg.size());
		auto const wide_size =
		    MultiByteToWideChar(CP_OEM, 0, msg.data(), oem_length, nullptr, 0);
		std::unique_ptr<wchar_t[]> wide{new wchar_t[wide_size]};
		MultiByteToWideChar(CP_OEM, 0, msg.data(), oem_length, wide.get(),
		                    wide_size);
		auto u8_size = WideCharToMultiByte(CP_UTF8, 0, wide.get(), wide_size,
		                                   nullptr, 0, nullptr, nullptr);
		std::unique_ptr<char[]> u8{new char[u8_size + 1]};
		WideCharToMultiByte(CP_UTF8, 0, wide.get(), wide_size, u8.get(),
		                    u8_size + 1, nullptr, nullptr);
		u8[u8_size] = 0;
		return u8.get();
	}
}  // namespace cov::app::platform
