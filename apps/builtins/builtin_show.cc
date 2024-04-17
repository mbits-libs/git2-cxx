// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#define NOMINMAX

#include <fmt/format.h>
#include <args/actions.hpp>
#include <cov/app/cvg_info.hh>
#include <cov/app/line_printer.hh>
#include <cov/app/path.hh>
#include <cov/app/show.hh>
#include <cov/format.hh>
#include <cov/git2/blob.hh>
#include <cov/git2/repository.hh>
#include <cov/module.hh>
#include <hilite/hilite.hh>
#include <hilite/lighter.hh>
#include <hilite/none.hh>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <unordered_set>

#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#else
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>
#define _isatty(FD) isatty(FD)
#define _fileno(OBJ) fileno(OBJ)
#endif

namespace cov::app::builtin::show {
	namespace {
		// GCOV_EXCL_START -- TODO: Add pty to test_driver
		size_t terminal_width(FILE* out) noexcept {
#ifdef _WIN32
			CONSOLE_SCREEN_BUFFER_INFO buffer = {};
			auto handle =
			    reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(out)));
			if (handle == INVALID_HANDLE_VALUE ||
			    !GetConsoleScreenBufferInfo(handle, &buffer))
				return 0;
			return buffer.dwSize.X < 0 ? 0u
			                           : static_cast<size_t>(buffer.dwSize.X);
#else
			winsize w;
			while (ioctl(fileno(out), TIOCGWINSZ, &w) == -1) {
				if (errno != EINTR) return 0;
			}
			return w.ws_col;
#endif
		}
		// GCOV_EXCL_STOP
	}  // namespace

	std::vector<file_stats> report_diff(app::show::parser::response const& info,
	                                    std::error_code& ec) {
		if (info.range.single) {
			std::error_code ec1{}, ec2{};
			auto const build = info.repo.lookup<cov::build>(info.range.to, ec1);
			auto files = info.repo.lookup<cov::files>(info.range.to, ec2);
			if (!ec1 && build) {
				ec2.clear();
				files =
				    info.repo.lookup<cov::files>(build->file_list_id(), ec2);
			}
			if (!ec2 && files) {
				std::vector<file_stats> result{};
				result.reserve(files->entries().size());
				for (auto const& entry : files->entries()) {
					auto const path_view = entry->path();
					auto path = std::string{path_view.data(), path_view.size()};
					result.push_back({
					    .filename = std::move(path),
					    .current = entry->stats(),
					    .previous = entry->stats(),
					    .diff_kind = file_diff::normal,
					    .current_functions = entry->function_coverage(),
					    .previous_functions = entry->function_coverage(),
					});
				}
				return result;
			}
		}

		auto const newer = info.repo.lookup<cov::report>(info.range.to, ec);
		if (ec) return {};

		if (info.range.single) {
			return info.repo.diff_with_parent(newer, ec);
		}

		auto const older = info.repo.lookup<cov::report>(info.range.from, ec);
		if (ec) return {};

		return info.repo.diff_betwen_reports(newer, older, ec);
	}

	ref_ptr<cov::files> get_files(git::oid_view id,
	                              cov::repository const& repo) {
		std::error_code ec{};
		auto generic = repo.lookup<cov::object>(id, ec);
		if (!generic || ec) return {};

		if (auto report = as_a<cov::report>(generic); report) {
			auto files = repo.lookup<cov::files>(report->file_list_id(), ec);
			if (ec) files.reset();
			return files;
		}

		if (auto build = as_a<cov::build>(generic); build) {
			auto files = repo.lookup<cov::files>(build->file_list_id(), ec);
			if (ec) files.reset();
			return files;
		}

		if (auto files = as_a<cov::files>(generic); files) {
			return files;
		}  // GCOV_EXCL_LINE[WIN32]

		// The only objects coming here are those above...
		[[unlikely]];  // GCOV_EXCL_LINE
		return {};     // GCOV_EXCL_LINE
	}

	int handle(std::string_view tool, args::arglist args) {
		using namespace str;
		using placeholder::color;

		app::show::parser p{{tool, args},
		                    {platform::locale_dir(), ::lngs::system_locales()}};
		auto info = p.parse();
		std::error_code ec{};

		auto mods = cov::modules::from_report(info.range.to, info.repo, ec);
		if (ec) {
			if (ec == git::errc::notfound) {
				ec = {};
			} else if (ec == cov::errc::wrong_object_type) {
				ec.clear();
				mods = cov::modules::make_modules("/"s, {});
			} else {                  // GCOV_EXCL_LINE
				p.error(ec, p.tr());  // GCOV_EXCL_LINE
			}                         // GCOV_EXCL_LINE
		}

		auto view = projection::report_filter{
		    mods.get(), p.module.value_or(std::string{}), info.path};
		auto entries = view.project(report_diff(info, ec), &info.repo);
		if (ec) p.error(ec, p.tr());

		app::show::environment env{
		    .colorizer = p.colorizer(),
		    .marks = placeholder::environment::rating_from(info.repo)};

		auto const is_standalone =
		    entries.size() == 1 &&
		    entries.front().type == projection::entry_type::standalone_file;

		auto const is_root = !is_standalone && view.module.filter.empty() &&
		                     view.fname.prefix.empty();

		auto const print_build_props = [&repo = info.repo, &range = info.range,
		                                &p = p]() {
			using namespace std::chrono;
			auto const now = floor<seconds>(system_clock::now());

			auto const facade =
			    placeholder::object_facade::present_oid(range.to, repo);
			if (!facade) return;
#define BUILDS "%{?prop[%C(yellow)build%Creset %mZ%n%]}"
			auto const format = facade->name() == "report"sv
			                        ? ("%{B[" BUILDS "%]}"sv)
			                        : BUILDS ""sv;
			auto const build_format = formatter::from(format);
			fmt::print("{}", build_format.format(facade.get(),
			                                     {.now = now,
			                                      .hash_length = 9,
			                                      .names = {},
			                                      .colorize = p.colorizer(),
			                                      .decorate = true,
			                                      .prop_names = false}));
		};

		if (is_root) {
			if (!info.range.single && info.range.from == info.range.to) {
				info.range.single = true;
				info.range.from = {};
			}

			p.show.print(info.repo, info.range, 1);
		} else if (is_standalone) {
			fmt::print("{}file {}{}\n", env.color_for(color::yellow),
			           entries.front().name.expanded,
			           env.color_for(color::reset));

			auto files = get_files(info.range.to, info.repo);
			if (files) {
				auto const entry =
				    files->by_path(entries.front().name.expanded);
				if (entry) {
					auto const facade =
					    placeholder::object_facade::present_file(entry,
					                                             &info.repo);
					p.show.selected_format = known_format::raw;
					p.show.print(info.repo, *facade, true);
				}
			}

			print_build_props();
			fmt::print("\n");
		} else {
			if (!view.module.filter.empty()) {
				fmt::print("{}module {}{}\n", env.color_for(color::yellow),
				           view.module.filter, env.color_for(color::reset));
			}
			if (!view.fname.prefix.empty()) {
				fmt::print("{}directory {}{}\n", env.color_for(color::yellow),
				           view.fname.prefix, env.color_for(color::reset));
			}

			print_build_props();
			fmt::print("\n");
		}

		env.print_table(entries);

		if (!is_standalone) return 0;

		auto const files = get_files(info.range.to, info.repo);
		if (!files) return 1;

		auto const* file_entry = files->by_path(entries.front().name.expanded);
		if (!file_entry || file_entry->contents().is_zero()) return 1;

		cvg_info cvg{};
		bool with_functions{true};

		auto const display_width = cov::platform::is_terminal(stdout)
		                               ? terminal_width(stdout)
		                               : std::numeric_limits<size_t>::max();

		if (!file_entry->line_coverage().is_zero()) {
			auto const file_cvg = info.repo.lookup<cov::line_coverage>(
			    file_entry->line_coverage(), ec);
			if (file_cvg && !ec)
				cvg = cvg_info::from_coverage(file_cvg->coverage());
		}

		if (with_functions && !file_entry->function_coverage().is_zero()) {
			auto const file_cvg = info.repo.lookup<cov::function_coverage>(
			    file_entry->function_coverage(), ec);
			if (file_cvg && !ec) cvg.add_functions(*file_cvg);
		}

		cvg.find_chunks();

		auto const data = file_entry->get_contents(info.repo, ec);
		if (ec) p.error(ec, p.tr());
		cvg.load_syntax(
		    std::string_view{reinterpret_cast<char const*>(data.data()),
		                     data.size()},
		    entries.front().name.display);

		auto clr = p.show.color_type;
		if (clr == use_feature::automatic) {
			clr = cov::platform::is_terminal(stdout) ? use_feature::yes
			                                         : use_feature::no;
		}

		auto const widths = cvg.column_widths();
		auto fn = cvg.funcs();

		bool first = true;
		for (auto const& [start, stop] : cvg.chunks) {
			if (first)
				first = false;
			else
				fmt::print("\n");
			fmt::print("\n");

			bool ran_out_of_lines{false};
			for (auto line_no = start; line_no <= stop; ++line_no) {
				if (!cvg.has_line(line_no)) {
					ran_out_of_lines = true;
					break;
				}
				fn.at(line_no, [=, &widths](auto const& function) {
					auto const aliases = cvg_info::soft_alias(function);
					for (auto const& alias : aliases) {
						fmt::print("{}\n",
						           cvg.to_string(
						               alias, widths, clr == use_feature::yes,
						               display_width, function.count));
					}
				});

				fmt::print("{}\n", cvg.to_string(line_no, widths,
				                                 clr == use_feature::yes));
			}
			if (ran_out_of_lines) break;
		}

		fmt::print("\n");
		return 0;
	}
}  // namespace cov::app::builtin::show
