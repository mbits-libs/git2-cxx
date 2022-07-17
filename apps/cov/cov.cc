// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <args/parser.hpp>
#include <cov/app/root_command.hh>
#include <cov/app/tools.hh>
#include <cov/builtins.hh>

int tool(args::args_view const& arguments) {
	using namespace cov::app;

	auto const langs = ::lngs::system_locales();
	root::parser parser{arguments, builtin::tools, tools::get_locale_dir(),
	                    langs};
	auto [tool, args] = parser.parse();

	std::string aliased_tool{};
	auto const ret = root::setup_tools(builtin::tools)
	                     .handle(tool, aliased_tool, args, tools::get_sysroot(),
	                             parser.tr());
	if (ret == -ENOENT) {
		parser.noent(aliased_tool.empty() ? tool : aliased_tool);
		return 1;
	}
	if (ret < 0) return -ret;
	return ret;
}
