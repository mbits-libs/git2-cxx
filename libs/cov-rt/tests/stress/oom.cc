// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/app/dirs.hh>
#include <cov/app/root_command.hh>
#include <cov/app/tools.hh>
#include <git2/global.hh>
#include "../path-utils.hh"
#include "new.hh"

namespace cov::app::testing {
	using namespace cov::testing;

	class oom : public ::testing::Test {
		git::init globals{};

	public:
		void run_setup(std::vector<path_info> const& steps) {
			std::error_code ec{};
			path_info::op(steps, ec);
			ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
			                 << ec.category().name() << ')';
		}
	};

	TEST_F(oom, tools_resolve) {
		run_setup(make_setup(
		    touch("a-config"sv,
		          "[alias]\n"
		          "tool = very long replacement for this tool. and now, 100 "
		          "characters to push past the limit: 123456789 123456789 "
		          "123456789 123456789 123456789 123456789 123456789 123456789 "
		          "123456789 123456789"
		          ""sv)));

		auto cfg = git::config::create();
		cfg.add_file_ondisk(setup::test_dir() / "a-config"sv);
		tools handler{std::move(cfg), {}};

		str::strings<covlng> str{};
		str.setup_path_manager({});

		std::string actual;

		OOM_LIMIT(100)
		handler.handle("tool"sv, actual, {}, {}, str);
		OOM_END
	}

	TEST_F(oom, tools_list) {
		run_setup(
		    make_setup(touch("a-config"sv,
		                     "[alias]\n"
		                     "a-tool-with-very-long-name-A-123456789-123456789-"
		                     "123456789-123456789-123456789-123456789-"
		                     "123456789-123456789-123456789 = nevermind\n"
		                     "a-tool-with-very-long-name-B-123456789-123456789-"
		                     "123456789-123456789-123456789-123456789-"
		                     "123456789-123456789-123456789 = nevermind\n"
		                     "a-tool-with-very-long-name-C-123456789-123456789-"
		                     "123456789-123456789-123456789-123456789-"
		                     "123456789-123456789-123456789 = nevermind\n"
		                     "a-tool-with-very-long-name-D-123456789-123456789-"
		                     "123456789-123456789-123456789-123456789-"
		                     "123456789-123456789-123456789 = nevermind\n"
		                     "a-tool-with-very-long-name-E-123456789-123456789-"
		                     "123456789-123456789-123456789-123456789-"
		                     "123456789-123456789-123456789 = nevermind\n"
		                     ""sv)));

		auto cfg = git::config::create();
		cfg.add_file_ondisk(setup::test_dir() / "a-config"sv);
		tools handler{std::move(cfg), {}};

		std::string actual;

		OOM_LIMIT(100)
		handler.list_tools("alias"sv, {});
		OOM_END
	}

	TEST_F(oom, tools_cautiously_open_config) {
		path root_dir =
		    "/lets/just/let/loose/and/create/a/name/long/enough/to/trigger/oom"sv;
		// it's ought to land somewhere inside discover_repository
		OOM_LIMIT(5 * 1024)
		tools::cautiously_open_config(root_dir);
		OOM_END
	}

	TEST_F(oom, cov_setup_parser) {
		auto arg0 = "test"s;
		char* argv[] = {
		    arg0.data(),
		    nullptr,
		};

		OOM_BEGIN(100)
		root::parser{args::from_main(2, argv), {}, {}};
		OOM_END
		printf("not exited...\n");
	}

	TEST_F(oom, cov_list_commands) {
		auto arg0 = "test"s;
		auto arg1 = "--list-cmds=builtins"s;
		char* argv[] = {
		    arg0.data(),
		    arg1.data(),
		    nullptr,
		};

		static constexpr builtin_tool mock[] = {
		    {"very-long-name-of-a-subcommand-A"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     "-123456789abcdefghijklmnopqrstuvwxyz"
		     ""sv,
		     nullptr},
		};

		auto parser = root::parser{args::from_main(2, argv), mock, {}};

		OOM_BEGIN(2048)
		parser.parse();
		OOM_END
		printf("not exited...\n");
	}

	TEST_F(oom, cov_help) {
		auto arg0 = "test"s;
		auto arg1 = "--help"s;
		char* argv[] = {
		    arg0.data(),
		    arg1.data(),
		    nullptr,
		};

		auto parser = root::parser{args::from_main(2, argv), {}, {}};

		OOM_BEGIN(384)
		parser.parse();
		OOM_END
		printf("not exited...\n");
	}
}  // namespace cov::app::testing
