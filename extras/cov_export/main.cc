// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/core/report_stats.hh>
#include <cov/format.hh>
#include <cov/io/file.hh>
#include <cov/module.hh>
#include <cxx-filt/parser.hh>
#include <native/path.hh>
#include <set>
#include <web/link_service.hh>
#include "parser.hh"
#include "stage.hh"

using namespace std::literals;

namespace cov::app::report_export {
	std::vector<file_stats> report_diff(parser::response const& info,
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

	struct counted_actions {
		size_t count;
		size_t index{1};
		size_t prev_line_length{0};

		[[noreturn]] void error(parser& p, std::error_code const& ec) {
			if (cov::platform::is_terminal(stdout)) fmt::print("\n");
			p.error(ec, p.tr());
		}

		void on_action(std::string_view action) {
			if (cov::platform::is_terminal(stdout)) {
				fmt::print("\r\033[2K[{}/{}] {}", index, count, action);
				std::fflush(stdout);
			} else {
				fmt::print("[{}/{}] {}\n", index, count, action);
			}
			++index;
		}
	};

	void html_report(web::stage stage, parser& p) {
		std::error_code ec{};

		stage.initialize(ec);
		if (ec) p.error(ec, p.tr());

		auto const pages = stage.list_pages_in_report();
		counted_actions logger{.count = pages.size()};

		for (auto const& item : pages) {
			logger.on_action(get_u8path(item.filename));
			auto state = stage.next_page(item, ec);
			if (ec) logger.error(p, ec);

			auto ctx = state.create_context(stage, ec);
			if (ec) logger.error(p, ec);

			auto page_text = stage.tmplt.render(state.template_name, ctx);

			auto out = io::fopen(state.full_path, "wb");
			if (!out) {
				auto const error = errno;
				ec = std::make_error_code(error ? static_cast<std::errc>(error)
				                                : std::errc::permission_denied);
				logger.error(p, ec);
			}

			out.store(page_text.c_str(), page_text.size());
		}
	}

	cxx_filt::Replacements load_replacements() {
		static constexpr std::pair<std::string_view, std::string_view>
		    raw_replacements[] = {
		        {"std::__cxx11::basic_string<$1, $2, $3>"sv,
		         "std::basic_string<$1, $2, $3>"sv},
		        {"std::basic_string<$1, $2, std::allocator<$1>>"sv,
		         "std::basic_string<$1, $2>"sv},
		        {"std::basic_string<$1, std::char_traits<$1>>"sv,
		         "std::basic_string<$1>"sv},
		        {"std::basic_string<char>"sv, "std::string"sv},
		        {"std::basic_string<wchar_t>"sv, "std::wstring"sv},
		        {"std::basic_string<char8_t>"sv, "std::u8string"sv},
		        {"std::basic_string<char16_t>"sv, "std::u8string"sv},
		        {"std::basic_string<char32_t>"sv, "std::u32string"sv},

		        {"std::basic_string_view<$1, std::char_traits<$1>>"sv,
		         "std::basic_string_view<$1>"sv},
		        {"std::basic_string_view<char>"sv, "std::string_view"sv},
		        {"std::basic_string_view<wchar_t>"sv, "std::wstring_view"sv},
		        {"std::basic_string_view<char8_t>"sv, "std::u8string_view"sv},
		        {"std::basic_string_view<char16_t>"sv, "std::u16string_view"sv},
		        {"std::basic_string_view<char32_t>"sv, "std::u32string_view"sv},

		        {"std::vector<$1, std::allocator<$1>>"sv, "std::vector<$1>"sv},
		        {"std::span<$1, 18446744073709551615ul>"sv, "std::span<$1>"sv},

		        {"std::map<$1, $2, $3, std::allocator<std::pair<$1 const, $2>>>"sv,
		         "std::map<$1, $2, $3>"sv},
		        {"std::map<$1, $2, std::less<$1>>"sv, "std::map<$1, $2>"sv},
		    };
		cxx_filt::Replacements result{};
		result.reserve(std::size(raw_replacements));

		for (auto const& [from, to] : raw_replacements) {
			auto stmt_from = cxx_filt::Parser::statement_from(from);
			auto stmt_to = cxx_filt::Parser::statement_from(to);
			if (stmt_from.items.size() != 1) {
				fmt::print(stderr,
				           "Expected to get exactly one expression from "
				           "'{}', got {}\n",
				           from, stmt_from.items.size());
				std::exit(1);
			}
			if (stmt_to.items.size() != 1) {
				fmt::print(stderr,
				           "Expected to get exactly one expression from "
				           "'{}', got {}\n",
				           to, stmt_to.items.size());
				std::exit(1);
			}
			result.push_back(
			    {std::move(stmt_from.items[0]), std::move(stmt_to.items[0])});
		}
		return result;
	}

	int handle(args::args_view const& args) {
		parser p{args,
		         {platform::core_extensions::locale_dir(),
		          ::lngs::system_locales()}};
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

		if (info.only_json) {
			p.error("--json is not implemented yet");
		}

		auto diff = report_diff(info, ec);
		if (ec) p.error(ec, p.tr());

		struct mod {
			std::vector<std::string> prefixes, files;
		};

		std::map<std::string, mod> module_mapping{};

		for (auto const& mod : mods->entries()) {
			module_mapping[mod.name].prefixes = mod.prefixes;
		}

		for (auto const& item : diff) {
			size_t length{};
			std::string module_name{};
			for (auto const& entry : mods->entries()) {
				for (std::string_view prefix : entry.prefixes) {
					if (!prefix.empty() && prefix.back() == '/')
						prefix = prefix.substr(0, prefix.length() - 1);
					if (item.filename.starts_with(prefix) &&
					    (item.filename.length() == prefix.length() ||
					     item.filename[prefix.length()] == '/') &&
					    length < prefix.length()) {
						length = prefix.length();
						module_name = entry.name;
					}
				}
			}

			module_mapping[module_name].files.push_back(item.filename);
		}

		html_report({.marks = placeholder::environment::rating_from(info.repo),
		             .mods = std::move(mods),
		             .diff = std::move(diff),
		             .out_dir = info.path,
		             .repo = info.repo,
		             .ref = info.range.to,
		             .base = info.range.from,
		             .replacements = load_replacements()},
		            p);

		return 0;
	}
}  // namespace cov::app::report_export

int tool(args::args_view const& args) {
	return cov::app::report_export::handle(args);
}
