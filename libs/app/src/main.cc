// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

#include <args/parser.hpp>
#include <git2/global.hh>

extern int tool(::args::args_view const&);

#ifdef _WIN32
#include <tchar.h>

std::string ut8_str(wchar_t const* arg) {
	if (!arg) return {};

	auto size =
	    WideCharToMultiByte(CP_UTF8, 0, arg, -1, nullptr, 0, nullptr, nullptr);
	std::unique_ptr<char[]> out{new char[size + 1]};
	WideCharToMultiByte(CP_UTF8, 0, arg, -1, out.get(), size + 1, nullptr,
	                    nullptr);
	return out.get();
}

std::vector<std::string> wide_char_to_utf8(int argc, wchar_t* argv[]) {
	std::vector<std::string> result{};
	result.resize(argc);
	std::transform(argv, argv + argc, result.begin(), ut8_str);
	return result;
}

int _tmain(int argc, wchar_t* argv[]) {
	auto utf8 = wide_char_to_utf8(argc, argv);
	std::vector<char*> args{};
	args.resize(utf8.size() + 1);
	std::transform(utf8.begin(), utf8.end(), args.begin(),
	               [](auto& s) { return s.data(); });
	args[argc] = nullptr;

	SetConsoleOutputCP(CP_UTF8);

	git::init memory_suite{};
	return tool(::args::from_main(static_cast<int>(args.size() - 1), args.data()));
}
#else
int main(int argc, char* argv[]) {
	git::init memory_suite{};
	return tool(args::from_main(argc, argv));
}
#endif
