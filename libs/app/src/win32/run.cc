// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#define NOMINMAX

#include <Windows.h>
#include <errno.h>
#include <args/parser.hpp>
#include <cov/app/run.hh>
#include <cov/io/file.hh>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <thread>
#include <vector>
#include "../path_env.hh"
#include "fmt/format.h"

namespace cov::app::platform {
	namespace {
		using namespace std::literals;

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

		template <typename Char>
		size_t arg_length(std::basic_string_view<Char> arg) {
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

		template <typename Char>
		size_t arg_length(std::basic_string<Char> const& arg) {
			return arg_length(std::basic_string_view<Char>{arg});
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

		// GCOV_EXCL_START[WIN32]
		// Currently, used only in debug fmt::prints
		std::string to_utf8(std::wstring_view arg) {
			if (arg.empty()) return {};

			auto const length = static_cast<int>(arg.size());

			auto size = WideCharToMultiByte(CP_UTF8, 0, arg.data(), length,
			                                nullptr, 0, nullptr, nullptr);
			std::unique_ptr<char[]> out{new char[size + 1]};
			WideCharToMultiByte(CP_UTF8, 0, arg.data(), length, out.get(),
			                    size + 1, nullptr, nullptr);
			out[size] = 0;
			return {out.get(), static_cast<size_t>(size)};
		}
		// GCOV_EXCL_STOP

		struct win32_pipe {
			HANDLE read{nullptr};
			HANDLE write{nullptr};

			~win32_pipe() {
				close_side(read);
				close_side(write);
			}

			void close_read() { close_side(read); }
			void close_write() { close_side(write); }

			bool open(SECURITY_ATTRIBUTES& attrs, bool as_input = false) {
				return CreatePipe(&read, &write, &attrs, 0) &&
				       SetHandleInformation(as_input ? write : read,
				                            HANDLE_FLAG_INHERIT, 0);
			}

			static constexpr DWORD BUFSIZE = 16384u;

			std::thread async_write(std::vector<std::byte> const& src) {
				return std::thread(
				    [](win32_pipe* self, std::vector<std::byte> const& bytes) {
					    auto ptr = bytes.data();
					    auto size = bytes.size();
					    DWORD written{};

					    auto const handle = self->write;
					    while (size) {
						    auto chunk = size;
						    if (chunk > BUFSIZE) chunk = BUFSIZE;
						    if (!WriteFile(handle, ptr,
						                   static_cast<DWORD>(chunk), &written,
						                   nullptr)) {
							    // GCOV_EXCL_START
							    [[unlikely]];
							    break;
							    // GCOV_EXCL_STOP
						    }  // GCOV_EXCL_LINE
						    ptr += written;
						    size -= static_cast<size_t>(written);
					    }

					    self->close_write();
				    },
				    this, std::ref(src));
			}

			std::thread async_read(std::vector<std::byte>& dst) {
				return std::thread(
				    [](HANDLE handle, std::vector<std::byte>& bytes) {
					    static constexpr auto CR = std::byte{'\r'};

					    DWORD read;
					    std::vector<std::byte> buffer(BUFSIZE);

					    for (;;) {
						    if (!ReadFile(handle, buffer.data(), BUFSIZE, &read,
						                  nullptr) ||
						        read == 0)
							    break;

						    auto first = buffer.data();
						    auto last = first + read;
						    bytes.reserve(bytes.size() + read);

						    while (first != last) {
							    auto start = first;
							    while (first != last && *first != CR)
								    ++first;

							    bytes.insert(bytes.end(), start, first);

							    if (first != last) ++first;
						    }
					    }
				    },
				    read, std::ref(dst));
			}

		private:
			void close_side(HANDLE& handle) {
				if (handle != nullptr) CloseHandle(handle);
				handle = nullptr;
			}
		};

		struct win32_pipes {
			win32_pipe input{};
			win32_pipe output{};
			win32_pipe error{};

			bool open(bool with_input, bool with_output) {
				SECURITY_ATTRIBUTES saAttr{
				    .nLength = sizeof(SECURITY_ATTRIBUTES),
				    .lpSecurityDescriptor = nullptr,
				    .bInheritHandle = TRUE,
				};
				if (with_input) {
					if (!input.open(saAttr, true)) {
						// GCOV_EXCL_START
						[[unlikely]];
						return false;
						// GCOV_EXCL_STOP
					}  // GCOV_EXCL_LINE
				}
				if (with_output) {
					if (!output.open(saAttr) || !error.open(saAttr)) {
						// GCOV_EXCL_START
						[[unlikely]];
						return false;
						// GCOV_EXCL_STOP
					}  // GCOV_EXCL_LINE
				}
				return true;
			}

			void io(std::vector<std::byte> const* input_data,
			        captured_output* output_data) {
				input.close_read();
				output.close_write();
				error.close_write();

				{
					HANDLE handles[] = {output.read, error.read};
					WaitForMultipleObjects(2, handles, FALSE, INFINITE);
				}

				std::vector<std::thread> threads{};
				threads.reserve((input_data ? 1u : 0u) +
				                (output_data ? 2u : 0u));
				if (input_data) {
					threads.push_back(input.async_write(*input_data));
				}
				if (output_data) {
					threads.push_back(output.async_read(output_data->output));
					threads.push_back(error.async_read(output_data->error));
				}

				for (auto& thread : threads) {
					thread.join();
				}
			}
		};

		struct scripted_file {
			std::filesystem::path program_file;
			std::string script_engine;
			bool access{true};

			std::wstring command_line(std::wstring const& program,
			                          args::arglist args) const {
				auto length = script_engine.empty() ? arg_length(program)
				                                    : arg_length(script_engine);
				if (!script_engine.empty())
					length += 1 + arg_length(program_file.native());
				for (unsigned index = 0; index < args.size(); ++index)
					length += 1 + arg_length(args[index]);

				std::wstring arg_string;
				arg_string.reserve(length + 1);

				if (script_engine.empty()) {
					append_arg(arg_string, program);
				} else {
					append_arg(arg_string, script_engine);
					append(arg_string, ' ');
					append_arg(arg_string, program_file.native());
				}

				for (unsigned index = 0; index < args.size(); ++index) {
					append(arg_string, ' ');
					append_arg(arg_string, from_utf8(args[index]));
				}

				append(arg_string,
				       ' ');  // because we need .data() in CreateProcess...
				return arg_string;
			}

			LPCWSTR command_file() const {
				return script_engine.empty() ? program_file.c_str() : nullptr;
			}
		};
		std::wstring env(wchar_t const* name) {
			wchar_t* env{};
			size_t length{};
			auto err = _wdupenv_s(&env, &length, name);
			std::wstring result{};
			if (!err && env && length) result.assign(env, length - 1);
			return result;
		}
		std::wstring filename(std::wstring_view program,
		                      std::wstring_view ext) {
			std::wstring result{};
			result.reserve(program.length() + ext.length());
			result.append(program);
			result.append(ext);
			return result;
		}

		bool file_exists(std::filesystem::path const& path) {
			std::error_code ec{};
			auto status = std::filesystem::status(path, ec);
			return !ec && std::filesystem::is_regular_file(status);
		}

		std::filesystem::path where(std::filesystem::path const& bin,
		                            wchar_t const* environment_variable,
		                            std::wstring const& program) {
			auto ext_str = env(L"PATHEXT");
			auto path_ext = split(std::wstring{}, ext_str);

			if (program.find_first_of(L"\\/"sv) != std::string::npos) {
				return program;
			}

			auto path_str = env(environment_variable);
			auto dirs = split(bin.native(), path_str);

			for (auto const& dir : dirs) {
				for (auto const ext : path_ext) {
					auto path =
					    std::filesystem::path{dir} / filename(program, ext);
					if (file_exists(path)) {
						return std::filesystem::canonical(path);
					}
				}
			}

			return {};
		}

		std::filesystem::path extensionless_where(
		    std::filesystem::path const& bin,
		    wchar_t const* environment_variable,
		    std::wstring const& program) {
			auto path_str = env(environment_variable);
			auto dirs = split(bin.native(), path_str);

			for (auto const& dir : dirs) {
				auto path = std::filesystem::path{dir} / program;
				if (file_exists(path)) {
					return std::filesystem::canonical(path);
				}
			}

			return {};
		}

		scripted_file locate_file(std::filesystem::path const& bin,
		                          wchar_t const* environment_variable,
		                          std::wstring const& program) {
			// .exe -> extensionless python
			scripted_file result{};
			result.program_file = where(bin, environment_variable, program);
			if (result.program_file.empty()) {
				result.program_file =
				    extensionless_where(bin, environment_variable, program);
				auto in = io::fopen(result.program_file);
				if (!in) {
					result.program_file.clear();
				} else {
					auto first_line = in.read_line();
					if (first_line.starts_with("#!"sv) &&
					    first_line.find("python"sv) != std::string::npos) {
						result.script_engine.assign("python"sv);
					} else {
						result.program_file.clear();
						result.access = false;
					}
				}
			}
			return result;
		}

		captured_output execute(std::filesystem::path const& bin,
		                        wchar_t const* environment_variable,
		                        std::wstring const& program,
		                        args::arglist args,
		                        std::vector<std::byte> const* input,
		                        std::filesystem::path const* cwd,
		                        bool capture) {
			captured_output result{};

			auto const path = locate_file(bin, environment_variable, program);
			if (path.program_file.empty()) {
				result.return_code = !path.access ? -EACCES : -ENOENT;
				return result;
			}

			win32_pipes pipes{};
			if (!pipes.open(input != nullptr, capture)) {
				// GCOV_EXCL_START
				[[unlikely]];
				result.return_code = 128;
				return result;
				// GCOV_EXCL_STOP
			}  // GCOV_EXCL_LINE

			STARTUPINFOW si;
			PROCESS_INFORMATION pi;

			ZeroMemory(&si, sizeof(si));
			si.cb = sizeof(si);
			if (input != nullptr || capture) {
				si.hStdInput = pipes.input.read;
				si.hStdOutput = pipes.output.write;
				si.hStdError = pipes.error.write;
				si.dwFlags |= STARTF_USESTDHANDLES;
			}
			ZeroMemory(&pi, sizeof(pi));

			if (!CreateProcessW(path.command_file(),
			                    path.command_line(program, args).data(),
			                    nullptr, nullptr, input != nullptr || capture,
			                    0, nullptr, cwd ? cwd->c_str() : nullptr, &si,
			                    &pi)) {
				// GCOV_EXCL_START[WIN32]
				[[unlikely]];
				auto const error = GetLastError();
				if (error == ERROR_FILE_NOT_FOUND ||
				    error == ERROR_PATH_NOT_FOUND) {
					result.return_code = -ENOENT;
					return result;
				}

				result.return_code = 128;
				return result;
				// GCOV_EXCL_STOP[WIN32]
			}  // GCOV_EXCL_LINE

			pipes.io(input, &result);

			DWORD return_code{};
			WaitForSingleObject(pi.hProcess, INFINITE);
			if (!GetExitCodeProcess(pi.hProcess, &return_code)) {
				// GCOV_EXCL_START[WIN32]
				[[unlikely]];
				result.return_code = 128;
				return result;
				// GCOV_EXCL_STOP[WIN32]
			}  // GCOV_EXCL_LINE

			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);

			result.return_code = static_cast<int>(return_code);
			return result;
		}
	}  // namespace

	int run_tool(std::filesystem::path const& tooldir,
	             std::string_view tool,
	             args::arglist args) {
		return execute(tooldir, L"COV_PATH", L"cov-" + from_utf8(tool), args,
		               nullptr, nullptr, false)
		    .return_code;
	}

	captured_output run_filter(std::filesystem::path const& filter_dir,
	                           std::filesystem::path const& cwd,
	                           std::string_view filter,
	                           args::arglist args,
	                           std::vector<std::byte> const& input) {
		return execute(filter_dir, L"COV_FILTER_PATH", from_utf8(filter), args,
		               &input, &cwd, true);
	}

	std::optional<std::filesystem::path> find_program(
	    std::span<std::string const> names,
	    std::filesystem::path const& hint) {
		for (auto const& name : names) {
			auto candidate = where(hint, L"PATH", from_utf8(name));
			if (!candidate.empty()) return candidate;
		}
		return std::nullopt;
	}

	captured_output run(std::filesystem::path const& exec,
	                    args::arglist args,
	                    std::filesystem::path const& cwd,
	                    std::vector<std::byte> const& input) {
		return execute({}, L"PATH", exec.native(), args, &input, &cwd, true);
	}

	captured_output run(std::filesystem::path const& exec,
	                    args::arglist args,
	                    std::vector<std::byte> const& input) {
		return execute({}, L"PATH", exec.native(), args, &input, nullptr, true);
	}

	int call(std::filesystem::path const& exec,
	         args::arglist args,
	         std::filesystem::path const& cwd,
	         std::vector<std::byte> const& input) {
		auto input_ptr = input.empty() ? nullptr : &input;
		return execute({}, L"PATH", exec.native(), args, input_ptr, &cwd, false)
		    .return_code;
	}

	int call(std::filesystem::path const& exec,
	         args::arglist args,
	         std::vector<std::byte> const& input) {
		auto input_ptr = input.empty() ? nullptr : &input;
		return execute({}, L"PATH", exec.native(), args, input_ptr, nullptr,
		               false)
		    .return_code;
	}
}  // namespace cov::app::platform
