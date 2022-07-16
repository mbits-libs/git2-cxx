// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/app/root_command.hh>
#include <cov/app/tr.hh>
#include <cov/version.hh>

#include <fmt/format.h>
/*#include <cov/app/dirs.hh>
#include <filesystem>
#include <git2/global.hh>*/

using namespace std::literals;

namespace cov::app::root {
	namespace {
		static constexpr help_command common_commands[] = {
		    {"init"sv, _("creates a new cov repo")},
		    {"config"sv, _("shows and/or sets various settings")},
		    {"module"sv, _("defines file groups")},
		    {"add"sv, _("appends a report to repo")},
		    {"remove"sv, _("removes a particual report from repo")},
		    {"log"sv, _("prints a list of reports")},
		    {"show"sv, _("shows a specific report")},
		    {"serve"sv, _("starts a local webserver with reports")},
		};

		static constexpr help_group known_commands[] = {
		    {_("common commands"), common_commands},
		};

		std::string as_str(std::string_view view) {
			return {view.data(), view.size()};
		}

		void copy_commands(args::chunk& chunk, help_group const& group) {
			chunk.title.assign(group.name);
			chunk.items.reserve(group.commands.size());
			for (auto [name, description] : group.commands)
				chunk.items.push_back({as_str(name), as_str(description)});
		}

		[[noreturn]] void show_help(args::parser& p) {
			auto commands = p.printer_arguments();
			commands.reserve(commands.size() + std::size(known_commands));
			for (auto const& group : known_commands) {
				commands.emplace_back();
				copy_commands(commands.back(), group);
			}

			p.short_help();
			args::printer{stdout}.format_list(commands);
			std::exit(0);
		}  // GCOV_EXCL_LINE[WIN32] -- covered in oom.cov_help

		[[noreturn]] void show_version() {
			fmt::print(_("cov version {}\n"), cov::version::ui);
			std::exit(0);
		}  // GCOV_EXCL_LINE[WIN32]

		void change_dir(args::parser& p, std::filesystem::path const& dirname) {
			std::error_code ec;
			std::filesystem::current_path(dirname, ec);
			if (ec) p.error(dirname.string() + ": " + ec.message());
		}

		[[noreturn]] void list_commands(
		    std::span<builtin_tool const> const& builtins,
		    std::string_view groups) {
			auto commands =
			    setup_tools(builtins).list_tools(groups, tools::get_sysroot());
			for (auto const& cmd : commands)
				std::puts(cmd.c_str());
			std::exit(0);
		}  // GCOV_EXCL_LINE -- covered in oom.cov_list_commands

	}  // namespace

	parser::parser(args::args_view const& arguments,
	               std::span<builtin_tool const> const& builtins)
	    : parser_{setup_parser(arguments)}, builtins_{builtins} {}

	args::parser parser::setup_parser(args::args_view const& arguments) {
		auto const wrap_list_commands = [this](std::string_view groups) {
			return list_commands(this->builtins_, groups);
		};

		args::parser parser{{}, arguments, &null_tr_};

		parser.usage(_("[-h] [-C <path>] <command> [<args>]"));

		parser.provide_help(false);
		parser.custom(show_help, "h", "help")
		    .help(_("show this help message and exit"))
		    .opt();

		parser.custom(show_version, "v", "version")
		    .help(_("shows version information and exits"))
		    .opt();

		parser.custom(change_dir, "C")
		    .meta(_("<path>"))
		    .help(_("runs as if cov was started in <path> instead of the "
		            "current "
		            "working directory"))
		    .opt();

		parser.custom(wrap_list_commands, "list-cmds")
		    .meta(_("<spec>[,<spec>,...]"))
		    .help(_("lists known commands from requested groups"))
		    .opt();

		return parser;
	}

	args::args_view parser::parse() {
		auto const unparsed = parser_.parse(args::parser::allow_subcommands);
		if (unparsed.empty()) show_help(parser_);

		auto const tool = unparsed[0];
		if (!tool.empty() && tool.front() == '-')
			parser_.error(parser_.tr()(args::lng::unrecognized, tool));

		return {tool, unparsed.shift()};
	}

	void parser::noent(std::string_view tool) const {
		args::fmt_list commands{};
		commands.reserve(std::size(known_commands));
		for (auto const& group : known_commands) {
			commands.emplace_back();
			copy_commands(commands.back(), group);
		}

		parser_.short_help(stderr);

		auto prn = args::printer{stderr};
		auto msg = fmt::format(_("\"{}\" is not a cov command"), tool);
		prn.format_paragraph(fmt::format("{}: {}", parser_.program(), msg), 0);
		prn.format_list(commands);
	}

	tools setup_tools(std::span<builtin_tool const> const& builtins) {
		std::error_code ec{};
		auto cwd = std::filesystem::current_path(ec);
		if (ec) cwd.clear();
		auto cfg = tools::cautiously_open_config(cwd);
		return {std::move(cfg), builtins};
	}
}  // namespace cov::app::root
