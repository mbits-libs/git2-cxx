// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <args/parser.hpp>
#include <cov/app/dirs.hh>
#include <cov/app/root_command.hh>
#include <cov/app/tools.hh>
#include <cov/app/tr.hh>
#include <cov/builtins.hh>
#include <cov/version.hh>
#include <filesystem>
#include <git2/global.hh>

using namespace std::literals;

namespace cov::app {
	namespace {
		int cov(args::args_view const& arguments) {
			root::parser parser{arguments, builtin::tools};
			auto [tool, args] = parser.parse();

			std::string aliased_tool{};
			auto const ret =
			    root::setup_tools(builtin::tools)
			        .handle(tool, aliased_tool, args, tools::get_sysroot());
			if (ret == -ENOENT) {
				parser.noent(aliased_tool.empty() ? tool : aliased_tool);
				return 1;
			}
			if (ret < 0) return -ret;
			return ret;
		}
	}  // namespace
}  // namespace cov::app

int main(int argc, char* argv[]) {
	using namespace std::filesystem;

	git::init memory_suite{};

	return cov::app::cov(args::from_main(argc, argv));
}
