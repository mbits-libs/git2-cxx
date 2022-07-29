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
	namespace {
		void append(std::wstring& ws, wchar_t next) { ws.push_back(next); }

		template <size_t Length>
		void append(std::wstring& ws, wchar_t const (&next)[Length]) {
			ws.insert(ws.end(), next, next + Length - 1);
		}

		void append(std::wstring& ws, std::wstring_view next) {
			ws.append(next);
		}

		void append(std::wstring& ws, std::string_view next) {
			ws.reserve(ws.size() + next.size());
			std::copy(next.begin(), next.end(), std::back_inserter(ws));
		}

		template <typename Char>
		void append_arg(std::wstring& ws, std::basic_string_view<Char> next) {
			static constexpr auto npos = std::string_view::npos;

			auto const esc = [=] {
				if constexpr (std::same_as<char, Char>)
					return next.find_first_of("\" ");
				else
					return next.find_first_of(L"\" ");
			}();

			if (esc == npos) {
				append(ws, next);
				return;
			}

			append(ws, '"');
			bool in_slashes = false;
			size_t slashes = 0;
			for (auto c : next) {
				switch (c) {
					case '\\':
						if (!in_slashes) slashes = 0;
						in_slashes = true;
						++slashes;
						break;
					case '"':
						// CommandLineToArgvW: (2n) + 1 backslashes followed by
						// a quotation mark produce n backslashes and ", but
						// does not toggle the "in quotes" mode
						if (in_slashes) {
							for (size_t i = 0; i < slashes; ++i)
								append(ws, '\\');
						}
						append(ws, '\\');
						in_slashes = false;
						break;
					default:
						in_slashes = false;
						break;
				}
				append(ws, c);
			}

			// CommandLineToArgvW: 2n backslashes followed by a quotation mark
			// produce n backslashes and no ", instead toggle the "in quotes"
			// mode
			if (in_slashes) {
				for (size_t i = 0; i < slashes; ++i)
					append(ws, '\\');
			}
			append(ws, '"');
		}

		template <typename Char>
		void append_arg(std::wstring& ws, std::basic_string<Char> const& next) {
			append_arg(ws, std::basic_string_view<Char>{next});
		}

		size_t arg_length(std::string_view arg) {
			static constexpr auto npos = std::string_view::npos;

			auto length = arg.length();
			auto quot = arg.find('"');
			while (quot != npos) {
				auto slashes = quot;
				while (slashes && arg[slashes - 1] == '\\')
					--slashes;
				// CommandLineToArgvW: (2n) + 1 backslashes followed by a
				// quotation mark produce n backslashes and ", but does not
				// toggle the "in quotes" mode
				length += 1 + slashes;
				quot = arg.find('"', quot + 1);
			}
			auto const escaped = length != arg.length();
			auto const space = !escaped ? arg.find(' ') : npos;
			if (escaped || space != npos) {
				length += 2;
				auto slashes = arg.length();
				while (slashes && arg[slashes - 1] == '\\')
					--slashes;
				// CommandLineToArgvW: 2n backslashes followed by a quotation
				// mark produce n backslashes and no ", instead toggle the "in
				// quotes" mode
				length += slashes;
			}

			return length;
		}

		std::wstring from_utf8(std::string_view arg) {
			if (arg.empty()) return {};

			auto const length = static_cast<int>(arg.size());

			auto size =
			    MultiByteToWideChar(CP_UTF8, 0, arg.data(), length, nullptr, 0);
			std::unique_ptr<wchar_t[]> out{new wchar_t[size + 1]};
			MultiByteToWideChar(CP_UTF8, 0, arg.data(), length, out.get(),
			                    size + 1);
			out[size] = 0;
			return {out.get(), static_cast<size_t>(size)};
		}

	}  // namespace

	int run_tool(std::filesystem::path const& tooldir,
	             std::string_view tool,
	             args::arglist args) {
		auto const program = L"cov-" + from_utf8(tool);
		auto const program_file = program + L".exe";

		auto prog_path = tooldir / program_file;
		prog_path.make_preferred();

		auto length = arg_length(tool) + 8;
		for (unsigned index = 0; index < args.size(); ++index)
			length += 1 + arg_length(args[index]);

		std::wstring arg_string;
		arg_string.reserve(length + 1);

		append_arg(arg_string, program);
		for (unsigned index = 0; index < args.size(); ++index) {
			append(arg_string, ' ');
			append_arg(arg_string, from_utf8(args[index]));
		}

		append(arg_string, '\0');

		STARTUPINFOW si;
		PROCESS_INFORMATION pi;

		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		if (!CreateProcessW(prog_path.c_str(), arg_string.data(), nullptr,
		                    nullptr, 0, 0, nullptr, nullptr, &si, &pi)) {
			auto const error = GetLastError();
			if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
				return -ENOENT;

			// GCOV_EXCL_START[WIN32]
			[[unlikely]];
			return 128;
			// GCOV_EXCL_STOP[WIN32]
		}

		DWORD retrun_code{};
		WaitForSingleObject(pi.hProcess, INFINITE);
		if (!GetExitCodeProcess(pi.hProcess, &retrun_code)) {
			// GCOV_EXCL_START[WIN32]
			[[unlikely]];
			return 128;
			// GCOV_EXCL_STOP[WIN32]
		}

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		return static_cast<int>(retrun_code);
	}
}  // namespace cov::app::platform
