// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <cov/app/root_command.hh>
#include <cov/app/tr.hh>
#include <cov/version.hh>

using namespace std::literals;

namespace cov::app::root {
	namespace {
		struct help_command {
			std::string_view name;
			covlng description;
		};

		struct help_group {
			covlng name;
			std::span<help_command const> commands;
		};

		static constexpr help_command common_commands[] = {
		    {"init"sv, covlng::HELP_DESCRIPTION_INIT},
		    {"config"sv, covlng::HELP_DESCRIPTION_CONFIG},
		    {"module"sv, covlng::HELP_DESCRIPTION_MODULE},
		    {"add"sv, covlng::HELP_DESCRIPTION_ADD},
		    {"remove"sv, covlng::HELP_DESCRIPTION_REMOVE},
		    {"log"sv, covlng::HELP_DESCRIPTION_LOG},
		    {"show"sv, covlng::HELP_DESCRIPTION_SHOW},
		    {"serve"sv, covlng::HELP_DESCRIPTION_SERVE},
		};

		static constexpr help_group known_commands[] = {
		    {covlng::HELP_GROUP_COMMON, common_commands},
		};

		std::string as_str(std::string_view view) {
			return {view.data(), view.size()};
		}

		void copy_commands(args::chunk& chunk,
		                   help_group const& group,
		                   CovStrings const& tr) {
			chunk.title.assign(tr(group.name));
			chunk.items.reserve(group.commands.size());
			for (auto [name, description] : group.commands)
				chunk.items.push_back({as_str(name), as_str(tr(description))});
		}

		[[noreturn]] void show_help(args::parser& p) {
			auto commands = p.printer_arguments();
			commands.reserve(commands.size() + std::size(known_commands));
			for (auto const& group : known_commands) {
				commands.emplace_back();
				copy_commands(commands.back(), group, parser::tr(p));
			}

			p.short_help();
			args::printer{stdout}.format_list(commands);
			std::exit(0);
		}  // GCOV_EXCL_LINE[WIN32] -- covered in oom.cov_help

		[[noreturn]] void show_version(args::parser& p) {
			//
			fmt::print(fmt::runtime(parser::tr(p)(covlng::VERSION_IS)),
			           cov::version::ui);
			std::fputc('\n', stdout);
			std::exit(0);
		}  // GCOV_EXCL_LINE[WIN32]

#ifdef __cpp_lib_char8_t
		template <typename CharTo, typename Source>
		inline std::basic_string_view<CharTo> conv(Source const& view) {
			return {reinterpret_cast<CharTo const*>(view.data()),
			        view.length()};
		}

		inline std::filesystem::path make_path(std::string_view utf8) {
			return conv<char8_t>(utf8);
		}
#else
		inline std::filesystem::path make_path(std::string_view utf8) {
			return std::filesystem::u8path(utf8);
		}
#endif

		void change_dir(args::parser& p, std::string const& dirname_str) {
			std::error_code ec;
			std::filesystem::current_path(make_path(dirname_str), ec);
			if (ec) p.error(dirname_str + ": " + ec.message());
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
	               std::span<builtin_tool const> const& builtins,
	               std::filesystem::path const& locale_dir,
	               std::span<std::string const> const& langs)
	    : parser_{setup_parser(arguments, locale_dir, langs)}
	    , builtins_{builtins} {}

	::args::parser parser::setup_parser(
	    ::args::args_view const& arguments,
	    std::filesystem::path const& locale_dir,
	    std::span<std::string const> const& langs) {
		using args = str::args::lng;

		auto const wrap_list_commands = [this](std::string_view groups) {
			return list_commands(this->builtins_, groups);
		};

		tr_.setup_path_manager(locale_dir);
		tr_.open_first_of(langs);

		::args::parser parser{{}, arguments, tr_.args()};

		parser.usage(tr_(covlng::USAGE));

		parser.provide_help(false);
		parser.custom(show_help, "h", "help")
		    .help(tr_(args::HELP_DESCRIPTION))
		    .opt();

		parser.custom(show_version, "v", "version")
		    .help(tr_(covlng::VERSION_DESCRIPTION))
		    .opt();

		parser.custom(change_dir, "C")
		    .meta(tr_(args::DIR_META))
		    .help(tr_(covlng::CWD_DESCRIPTION))
		    .opt();

		parser.custom(wrap_list_commands, "list-cmds")
		    .meta(tr_(covlng::SPECS_MULTI_META))
		    .help(tr_(covlng::LIST_CMDS_DESCRIPTION))
		    .opt();

		return parser;
	}  // GCOV_EXCL_LINE[GCC]

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
			copy_commands(commands.back(), group, tr_);
		}

		parser_.short_help(stderr);

		auto prn = args::printer{stderr};
		auto msg = fmt::format(
		    fmt::runtime(tr_(covlng::ERROR_TOOL_NOT_RECOGNIZED)), tool);
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
