// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <args/parser.hpp>
#include <cov/app/path.hh>
#include <filesystem>

int tool(args::args_view const& arguments) {
	fmt::print("{}: {}\n", arguments.progname,
	           cov::app::get_u8path(std::filesystem::current_path()));
	return 0;
}
