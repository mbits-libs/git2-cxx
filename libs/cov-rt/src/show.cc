// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/app/show.hh>
#include <cov/tabular.hh>

namespace cov::app::show {
	using namespace projection;

	parser::parser(::args::args_view const& arguments,
	               str::translator_open_info const& langs)
	    : base_parser<covlng, loglng, errlng, showlng>{langs, arguments} {
		using namespace str;

		parser_
		    .arg(rev)  // GCOV_EXCL_LINE[GCC]
		    .meta(tr_(covlng::REPORT_META))
		    .help(tr_(showlng::REPORT_DESCRIPTION));
		parser_.arg(module, "m", "module")
		    .meta(tr_(showlng::MODULE_META))
		    .help(tr_(showlng::MODULE_DESCRIPTION));
		show.add_args(parser_, tr_, false);
	}

	parser::response parser::parse() {
		using namespace str;

		parser_.parse();

		response result{};

		result.repo = open_here();

		if (!rev) rev = "HEAD"s;
		auto pos = rev->find(':');
		auto revision = pos == std::string::npos
		                    ? std::string_view{*rev}
		                    : std::string_view{*rev}.substr(0, pos);
		if (pos != std::string::npos) {
			result.path = rev->substr(pos + 1);
		}

		auto ec = revs::parse(result.repo, revision, result.range);
		if (ec) error(ec, tr_);

		return result;
	}  // GCOV_EXCL_LINE[GCC]

	parser::colorizer_t parser::colorizer() const noexcept {
		auto clr = show.color_type;
		if (clr == use_feature::automatic) {
			clr = cov::platform::is_terminal(stdout) ? use_feature::yes
			                                         : use_feature::no;
		}

		return clr == use_feature::yes
		           ? &formatter::shell_colorize
		           : +[](placeholder::color, void*) { return std::string{}; };
	}

	std::string environment::color_for(placeholder::color clr) const {
		if (!colorizer) return {};
		return colorizer(clr, app);
	}

	std::string format(environment const& env,
	                   std::string const& display,
	                   placeholder::color color) {
		if (display.empty() || color == placeholder::color::normal)
			return display;
		return fmt::format("{}{}{}", env.color_for(color), display,
		                   env.color_for(placeholder::color::reset));
	}

	std::vector<std::string> environment::add_table_row(
	    add_table_row_options const& options) const {
		std::string display{};
		auto const pos = options.label.rfind('/');
		if (pos == std::string_view::npos) {
			display.assign(options.label);
		} else {
			auto const prefix = options.label.substr(0, pos + 1);
			auto const name = options.label.substr(pos + 1);
			display = fmt::format(
			    "{}{}{}{}", color_for(placeholder::color::faint_normal), prefix,
			    color_for(placeholder::color::reset), name);
		}

		std::vector<std::string> result{};
		result.reserve((options.columns.size() + 1) * 2);

		result.push_back({options.entry_flag});
		result.push_back(std::move(display));

		static constinit auto const dummy_column =
		    core::column_info::from<core::col_title::lines_total,
		                            core::col_priority::supplemental,
		                            core::col_data::counter>();
		auto it = options.columns.begin();
		for (auto const& cell : options.cells) {
			auto const& column =
			    it != options.columns.end() ? *it++ : dummy_column;

			if (cell.value.empty() && cell.change.empty()) {
				result.push_back({});
				result.push_back({});
				continue;
			}

			using enum core::value_category;
			using enum core::col_priority;
			auto color = column.priority == high
			                 ? cell.category == passing
			                       ? placeholder::color::bold_green
			                   : cell.category == incomplete
			                       ? placeholder::color::bold_yellow
			                       : placeholder::color::bold_red
			                 : placeholder::color::normal;
			auto change_color =
			    cell.change.empty() ||
			            column.priority == core::col_priority::supplemental
			        ? placeholder::color::faint_normal
			    : cell.change_is_negative ? placeholder::color::faint_red
			                              : placeholder::color::faint_green;

			result.push_back(format(*this, cell.value, color));
			result.push_back(format(*this, cell.change, change_color));
		}

		return result;
	}

	void environment::print_table(std::vector<entry> const& entries) const {
		auto const [total, flags] = core::stats::calc_stats(entries);
		auto const projection =
		    core::stats::project(marks, entries, total, flags);

		data_table table{};

		table.header.reserve(projection.columns.size() + 1);
		table.header.push_back({"Name"s, '<'});

		for (auto const& col : projection.columns) {
			auto display = ""sv;
			switch (col.title) {
				// GCOV_EXCL_START TODO: remove when branch is supported
				case core::col_title::branches_covered:
					display = "% Branches"sv;
					break;
				case core::col_title::branches_relevant:
					display = "Relevant"sv;
					break;
				case core::col_title::branches_missing:
					display = "Missing"sv;
					break;
				// GCOV_EXCL_STOP
				case core::col_title::functions_covered:
					display = "% Funcs"sv;
					break;
				case core::col_title::functions_relevant:
					display = "Relevant"sv;
					break;
				case core::col_title::functions_missing:
					display = "Missing"sv;
					break;
				case core::col_title::lines_covered:
					display = "% Lines"sv;
					break;
				case core::col_title::lines_missing:
					display = "Missing"sv;
					break;
				case core::col_title::lines_visited:
					display = "Visited"sv;
					break;
				case core::col_title::lines_relevant:
					display = "Relevant"sv;
					break;
				case core::col_title::lines_total:
					display = "Line count"sv;
					break;
			}
			table.header.push_back(std::string{display.data(), display.size()});
		}

		table.rows.reserve(projection.rows.size());
		for (auto const& row : projection.rows) {
			auto entry_flag = '?';
			switch (row.type) {
				case entry_type::module:
					entry_flag = 'M';
					break;
				case entry_type::directory:
					entry_flag = 'D';
					break;
				case entry_type::standalone_file:
				case entry_type::file:
					entry_flag = ' ';
					break;
			}

			table.emplace(add_table_row({.label = row.name.display,
			                             .entry_flag = entry_flag,
			                             .cells = row.data,
			                             .columns = projection.columns}));
		}

		if (!projection.footer.empty()) {
			table.emplace(add_table_row({
			                  .label = "TOTAL"sv,
			                  .entry_flag = '>',
			                  .cells = projection.footer,
			                  .columns = projection.columns,
			              }),
			              false);
		}

		table.print();
	}
}  // namespace cov::app::show
