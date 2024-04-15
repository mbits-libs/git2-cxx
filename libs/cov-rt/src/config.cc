// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/app/config.hh>
#include <cov/app/dirs.hh>
#include <cov/app/path.hh>
#include <cov/app/tools.hh>
#include <cov/repository.hh>

namespace cov::app::config {
	using namespace std::literals;

	namespace {
		template <scope which>
		using scope_constant = std::integral_constant<scope, which>;

		struct local_scope : scope_constant<scope::local> {};
		struct global_scope : scope_constant<scope::global> {};
		struct system_scope : scope_constant<scope::system> {};

		using namespace std::filesystem;
		using namespace app::config;

		inline parser::string name_of(command cmd) {
			using enum command;
			switch (cmd) {
				case set_or_get:
					return str::args::lng::NAME_META;
				case add:
					return "--add"sv;
				case get:
					return "--get"sv;
				case get_all:
					return "--get-all"sv;
				case unset:
					return "--unset"sv;
				case unset_all:
					return "--unset-all"sv;
				case list:
					return "--list"sv;
				case unspecified:  // GCOV_EXCL_LINE
					[[unlikely]];  // GCOV_EXCL_LINE
					break;         // GCOV_EXCL_LINE
			}
			[[unlikely]];  // GCOV_EXCL_LINE
			return {};     // GCOV_EXCL_LINE
		}

		[[noreturn]] void show_help(::args::parser& p) {
			using namespace std::literals;
			using str::args::lng;

			auto prn = args::printer{stdout};
			auto& _ = base_parser<errlng, cfglng>::tr(p);

			auto const _s = [&](auto arg) { return to_string(_(arg)); };

			auto file = _(cfglng::SCOPE_META);
			auto name = _(lng::NAME_META);
			auto value = _(lng::VALUE_META);

			prn.format_paragraph(_s(lng::USAGE), 7);
			for (auto synopsis : {
			         " cov config [-h] [{0}] {1} [{2}]"sv,
			         " cov config [-h] [{0}] --add {1} {2}"sv,
			         " cov config [-h] [{0}] --get {1}"sv,
			         " cov config [-h] [{0}] --get-all {1}"sv,
			         " cov config [-h] [{0}] --unset {1}"sv,
			         " cov config [-h] [{0}] --unset-all {1}"sv,
			         " cov config [-h] [{0}] -l | --list"sv,
			     }) {
				prn.format_paragraph(
				    fmt::format(fmt::runtime(synopsis), file, name, value), 7);
			}

			static constexpr parser::args_description scopes[] = {
			    {cfglng::NO_SCOPE_META, cfglng::NO_SCOPE_DESCRIPTION},
			    {"--local"sv, cfglng::SCOPE_LOCAL},
			    {"--global"sv, cfglng::SCOPE_GLOBAL},
			    {"--system"sv, cfglng::SCOPE_SYSTEM},
			    {"-f, --file"sv, cfglng::SCOPE_FILE, lng::FILE_META},
			};

			static constexpr parser::args_description positionals[] = {
			    {lng::NAME_META, cfglng::NAME_VALUE, opt{lng::VALUE_META}},
			};

			static constexpr parser::args_description optionals[] = {
			    {"-h, --help"sv, lng::HELP_DESCRIPTION},
			    {"--add"sv, cfglng::ADD, lng::NAME_META, lng::VALUE_META},
			    {"--get"sv, cfglng::GET, lng::NAME_META},
			    {"--get-all"sv, cfglng::GET_ALL, lng::NAME_META},
			    {"--unset"sv, cfglng::UNSET, lng::NAME_META},
			    {"--unset-all"sv, cfglng::UNSET_ALL, lng::NAME_META},
			    {"-l, --list"sv, cfglng::LIST_ENTRIES},
			};

			args::fmt_list args(3);
			parser::args_description::group(_, cfglng::SCOPE_TITLE, scopes,
			                                args[0]);
			parser::args_description::group(_, lng::POSITIONALS, positionals,
			                                args[1]);
			parser::args_description::group(_, lng::OPTIONALS, optionals,
			                                args[2]);

			prn.format_list(args);
			std::exit(0);
		}  // GCOV_EXCL_LINE
	}      // namespace

	parser::parser(::args::args_view const& arguments,
	               str::translator_open_info const& langs)
	    : base_parser<errlng, cfglng>{langs, arguments} {
		using namespace str;
		using enum command;

		parser_.usage(
		    fmt::format("[-h] [{0}] [{1} [{2}] | --unset {1} | -l | --list]"sv,
		                tr_(cfglng::SCOPE_META), tr_(args::lng::NAME_META),
		                tr_(args::lng::VALUE_META)));
		parser_.provide_help(false);
		parser_.custom(show_help, "h", "help").opt();
		parser_.set<local_scope>(which, "local").opt();
		parser_.set<global_scope>(which, "global").opt();
		parser_.set<system_scope>(which, "system").opt();
		parser_
		    .custom(
		        [&](std::string const& val) {
			        file = make_u8path(val);
			        which = scope::file;
		        },
		        "f", "file")
		    .opt();

		parser_.custom(set_command<add>(), "add").opt();
		parser_.custom(set_command<get>(), "get").opt();
		parser_.custom(set_command<get_all>(), "get-all").opt();
		parser_.custom(set_command<unset>(), "unset").opt();
		parser_.custom(set_command<unset_all>(), "unset-all").opt();
		parser_.custom([this] { handle_list(); }, "l", "list").opt();
		parser_.custom([this](std::string const& val) { handle_arg(val); })
		    .opt();
	}

	void parser::handle_command(command arg, std::string const& val) {
		if (cmd != command::unspecified) {
			str_visitor visitor{tr_};
			parser_.error(tr_.format(str::args::lng::ARGUMENT_MSG,
			                         visitor.visit(name_of(cmd)),
			                         tr_.format(str::args::lng::EXCLUSIVE,
			                                    visitor.visit(name_of(arg)))));
		}
		cmd = arg;
		args.push_back(val);
	}

	void parser::handle_list() {
		if (cmd != command::unspecified) {
			str_visitor visitor{tr_};
			parser_.error(tr_.format(
			    str::args::lng::ARGUMENT_MSG, visitor.visit(name_of(cmd)),
			    tr_.format(str::args::lng::EXCLUSIVE,
			               visitor.visit(name_of(command::list)))));
		}
		cmd = command::list;
	}

	void parser::handle_arg(std::string const& val) {
		if (cmd == command::unspecified) {
			cmd = command::set_or_get;
			args.push_back(val);
			return;
		}

		if ((cmd == command::set_or_get || cmd == command::add) &&
		    args.size() < 2) {
			args.push_back(val);
			return;
		}

		str_visitor visitor{tr_};
		parser_.error(tr_.format(str::args::lng::ARGUMENT_MSG,
		                         visitor.visit(name_of(cmd)),
		                         tr_(cfglng::TOO_MANY_ARGUMENTS)));
	}

	void parser::parse() {
		using namespace str;
		using enum command;

		parser_.parse();

		if (cmd == unspecified) show_help(parser_);

		switch (cmd) {
			case set_or_get:
				if (args.size() < 2) break;
				[[fallthrough]];
			case add:
			case unset:
			case unset_all:
				if (which == scope::unspecified) which = scope::local;
				how = mode::write;
				break;
			default:
				break;
		}

		if (cmd == add && args.size() < 2) {
			error(tr_.format(args::lng::ARGUMENT_MSG, "--add"sv,
			                 tr_(cfglng::ADD_VALUE_MISSING)));
		}

		path current_directory;

		if (which == scope::local || which == scope::unspecified) {
			std::error_code ec{};
			current_directory = current_path(ec);
			// GCOV_EXCL_START
			if (ec) {
				[[unlikely]];
				error(tr_.format(args::lng::ARGUMENT_MSG, "-C"sv,
				                 platform::con_to_u8(ec)));
			}
			// GCOV_EXCL_STOP
		}

		if (which == scope::local) {
			std::error_code ec{};
			auto const commondir =
			    cov::repository::discover(current_directory, ec);
			if (ec)
				error(tr_.format(args::lng::CANNOT_FIND_COV,
				                 get_u8path(current_directory)));
			file = commondir / "config"sv;
			which = scope::file;
		} else if (which == scope::unspecified) {
			file = current_directory;
		}
	}

	git::config open_config(scope which,
	                        std::filesystem::path const& sysroot,
	                        std::filesystem::path const& path,
	                        mode how,
	                        std::error_code& ec) {
		if (which == scope::unspecified)
			return tools::cautiously_open_config(sysroot, path);
		git::config result{};
		switch (which) {
			// GCOV_EXCL_START
			case scope::unspecified:
				[[unlikely]];
				break;
			case scope::local:
				[[unlikely]];
				[[fallthrough]];
			// GCOV_EXCL_STOP
			case scope::file:
				result = git::config::create();
				ec = result.add_file_ondisk(path);
				break;
			case scope::global:
				result = git::config::open_global(".covconfig"sv, "cov"sv,
				                                  how == mode::write, ec);
				break;
			case scope::system:
				result = git::config::open_system(sysroot, "cov"sv, ec);
				break;
		}
		if (ec) result = nullptr;
		return result;
	}
}  // namespace cov::app::config
