// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <cov/app/branches.hh>
#include <cov/app/path.hh>
#include <cov/format.hh>
#include <cov/repository.hh>
#include <cov/revparse.hh>

namespace fs = std::filesystem;
using namespace std::literals;

#ifdef WIN32
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
namespace cov::app::platform {
	bool glob_matches(fs::path const& match, fs::path const name) {
		return !!PathMatchSpecW(name.c_str(), match.c_str());
	}
}  // namespace cov::app::platform
#else
#include <fnmatch.h>
int fnmatch(const char* pattern, const char* string, int flags);
namespace cov::app::platform {
	bool glob_matches(fs::path const& match, fs::path const name) {
		return !fnmatch(match.c_str(), name.c_str(), FNM_PATHNAME);
	}
}  // namespace cov::app::platform
#endif

namespace cov::app {
	namespace {
		static constexpr auto heads = "refs/heads"sv;
		static constexpr auto tags = "refs/tags"sv;

		inline branches_base_parser::string name_of(command cmd) {
			using enum command;
			switch (cmd) {
				case create:
					return str::args::lng::NAME_META;
				case list:
					return "--list"sv;
				case remove:
					return "--delete"sv;
				case current:
					return "--show-current"sv;
				case unspecified:  // GCOV_EXCL_LINE
					[[unlikely]];  // GCOV_EXCL_LINE
					break;         // GCOV_EXCL_LINE
			}
			[[unlikely]];  // GCOV_EXCL_LINE
			return {};     // GCOV_EXCL_LINE
		}

		std::string name_for(std::string_view name, target tgt) {
			return fmt::format("{}/{}"sv, tgt == target::tag ? tags : heads,
			                   name);
		}

		struct lister {
			using color = placeholder::color;

			std::string_view prefix{};
			bool index{};
			std::string_view current{};
			std::string (*colorize)(color, void* app) = {};

			void list_refs(cov::repository const& repo,
			               std::span<std::string const> const& names);

		private:
			bool matching(std::vector<std::filesystem::path> const& matches,
			              std::string_view name);
			void print_selected(std::string_view name);
		};

		void lister::list_refs(cov::repository const& repo,
		                       std::span<std::string const> const& names) {
			std::vector<fs::path> matches{};
			matches.reserve(names.size());
			std::transform(names.begin(), names.end(),
			               std::back_inserter(matches), make_u8path);

			if (index && current.empty() && matching(matches, "HEAD"sv)) {
				print_selected("(detached HEAD)"sv);
			}

			auto items = repo.refs()->iterator(prefix);
			std::vector<std::string> matching_names{};
			for (auto const ref : *items) {
				auto const name = ref->shorthand();
				if (!matching(matches, name)) continue;
				matching_names.emplace_back(name.data(), name.size());
			}

			std::sort(matching_names.begin(), matching_names.end());
			if (index) {
				for (auto const& name : matching_names) {
					if (current == name)
						print_selected(name);
					else
						fmt::print("  {}\n"sv, name);
				}
			} else {
				for (auto const& name : matching_names)
					fmt::print("{}\n"sv, name);
			}
		}

		bool lister::matching(std::vector<fs::path> const& matches,
		                      std::string_view name) {
			if (matches.empty()) return true;

			auto const matched = make_u8path(name);
			for (auto const& match : matches) {
				if (platform::glob_matches(match, matched)) {
					return true;
				}
			}
			return false;
		}

		void lister::print_selected(std::string_view name) {
			if (!colorize) {
				fmt::print("* {}\n"sv, name);
				return;
			}
			fmt::print("* {}{}{}\n"sv, colorize(color::green, nullptr), name,
			           colorize(color::reset, nullptr));
		}
	}  // namespace

	void params::setup(::args::parser& parser, branches_base_parser& cli) {
		parser.custom(add_name()).opt();
		parser.custom(set_command<command::remove>(cli), "d", "delete").opt();
		parser.custom(set_command<command::list>(cli), "l", "list").opt();
		if (tgt == target::branch) {
			parser.custom(set_command<command::current>(cli), "show-current")
			    .opt();
		}
		parser.set<std::true_type>(force, "f", "force").opt();
		if (tgt == target::branch) {
			parser.set<std::true_type>(quiet, "q", "quiet").opt();
			parser.arg(color, "color").opt();
		}
	}

	void params::set_command(branches_base_parser& cli, command arg) {
		if (cmd != command::unspecified) {
			auto const& tr = cli.tr();
			str_visitor visitor{tr};
			cli.error(tr.format(str::args::lng::ARGUMENT_MSG,
			                    visitor.visit(name_of(cmd)),
			                    tr.format(str::args::lng::EXCLUSIVE,
			                              visitor.visit(name_of(arg)))));
		}
		cmd = arg;
	}

	void params::validate_using(branches_base_parser& cli) {
		if (cmd == command::unspecified) cmd = command::list;

		auto const& tr = cli.tr();

		if (force && cmd != command::create) {
			str_visitor visitor{tr};
			cli.error(tr.format(str::args::lng::ARGUMENT_MSG, "--force",
			                    tr.format(str::args::lng::EXCLUSIVE,
			                              visitor.visit(name_of(cmd)))));
		}

		std::optional<modcnt> error{};
		intmax_t count = 1;

		if (cmd == command::create && names.size() > 2) {
			// there is no way there could be zero names and still a command...
			error = modcnt::ERROR_OPTS_NEEDS_AT_MOST;
			count = 2;
		} else if (cmd == command::remove && names.empty()) {
			error = modcnt::ERROR_OPTS_NEEDS_AT_LEAST;
		}

		if (error) {
			str_visitor visitor{tr};
			cli.error(fmt::format(fmt::runtime(tr(*error, count)),
			                      visitor.visit(name_of(cmd)), count));
		}
	}

	std::error_code params::run(cov::repository const& repo) const {
		static const auto OK = std::error_code{};
		switch (cmd) {
			case command::list:
				return list(repo), OK;
			case command::create:
				return create(repo);
			case command::remove:
				return remove(repo);
			case command::current:
				return show_current(repo), OK;
			case command::unspecified:  // GCOV_EXCL_LINE
				[[unlikely]];           // GCOV_EXCL_LINE
				break;                  // GCOV_EXCL_LINE
		}
		[[unlikely]];  // GCOV_EXCL_LINE
		return OK;     // GCOV_EXCL_LINE
	}

	void params::list(cov::repository const& repo) const {
		if (tgt == target::tag) {
			lister{.prefix{tags}}.list_refs(repo, names);
			return;
		}

		auto const HEAD = repo.current_head();
		auto clr = color;
		if (clr == use_feature::automatic) {
			clr = cov::platform::is_terminal(stdout) ? use_feature::yes
			                                         : use_feature::no;
		}
		auto colorize =
		    clr == use_feature::yes ? formatter::shell_colorize : nullptr;

		lister{.prefix = heads,  // GCOV_EXCL_LINE
		       .index = true,
		       .current = HEAD.branch,
		       .colorize = colorize}
		    .list_refs(repo, names);
	}

	std::error_code params::create(cov::repository const& repo) const {
		git::oid oid{};
		auto result = revs::parse_single(
		    repo, names.size() < 2 ? "HEAD"sv : names[1], oid);
		if (result) return result;
		auto ref = reference::direct(references::prefix_info(""), oid);

		repo.refs()->copy_ref(ref, names.front(), tgt == target::branch, force,
		                      result);
		return result;
	}

	std::error_code params::remove(cov::repository const& repo) const {
		auto const HEAD = repo.current_head();
		auto curr = repo.refs()->lookup(name_for(names.front(), tgt));
		if (!curr) return git::make_error_code(git::errc::notfound);

		if (tgt == target::branch && HEAD.branch == curr->shorthand())
			return cov::make_error_code(cov::errc::current_branch);

		return repo.refs()->remove_ref(curr);
	}

	void params::show_current(cov::repository const& repo) const {
		auto const HEAD = repo.current_head();
		if (!HEAD.branch.empty()) fmt::print("{}\n"sv, HEAD.branch);
	}
}  // namespace cov::app
