// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <args/actions.hpp>
#include <cov/app/args.hh>
#include <cov/app/checkout.hh>
#include <cov/app/rt_path.hh>
namespace cov::app::builtin::checkout {
	int handle(std::string_view tool, args::arglist args) {
		using namespace str;
		app::checkout::parser p{
		    {tool, args}, {platform::locale_dir(), ::lngs::system_locales()}};
		p.parse();
		auto const repo = p.open_here();
		auto const ec = p.run(repo);
		if (ec) p.error(ec, p.tr());

		return 0;
	}
}  // namespace cov::app::builtin::checkout
