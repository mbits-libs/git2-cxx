// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/app/show.hh>
#include <cov/tabular.hh>

namespace cov::app::show {
	using namespace projection;

	enum class show_column { name, other };

	using vis_callback = bool (*)(with);
	bool has(with flags, with test) { return (flags & test) == test; }
	bool always(with) { return true; };
	bool with_branches(with flags) { return has(flags, with::branches); };
	bool with_funcs(with flags) { return has(flags, with::functions); };
	bool with_branches_missing(with flags) {
		return has(flags, with::branches | with::branches_missing);
	};
	bool with_funcs_missing(with flags) {
		return has(flags, with::functions | with::functions_missing);
	};
	bool with_lines_missing(with flags) {
		return has(flags, with::lines_missing);
	};

	using cell_row = std::vector<std::string>;
	using data_append = void (*)(environment const&,
	                             cell_row&,
	                             file_diff const&);

	struct column {
		struct label_t {
			std::string_view text;
			char align;
			constexpr label_t() = delete;
			constexpr label_t(std::string_view text, char align = '^')
			    : text{text}, align{align} {}
		} label;
		data_append append;
		show_column id{show_column::other};
		vis_callback vis{always};

		void header(std::vector<table_column>& hdr, with flags) const {
			if (!vis(flags)) return;
			hdr.push_back(
			    {{label.text.data(), label.text.size()}, label.align});
		}

		void data(environment const& env,
		          std::vector<std::string>& cells,
		          file_diff const& change,
		          with flags) const {
			if (!vis(flags)) return;
			append(env, cells, change);
		}
	};

	static constexpr std::array columns = {
	    column{{"Name"sv, '<'}, nullptr, show_column::name},
	    // GCOV_EXCL_START for now
	    column{"% Branch"sv,
	           [](environment const& env,
	              cell_row& cells,
	              file_diff const& change) {
		           env.percentage(
		               cells, change,
		               [](auto const& stats) { return stats.branches; },
		               placeholder::color::bold_rating_branches);
	           },
	           show_column::other, with_branches},
	    column{"Missing"sv,
	           [](environment const& env,
	              cell_row& cells,
	              file_diff const& change) {
		           env.dimmed_count(
		               cells, change,
		               [](auto const& stats) {
			               return stats.branches.relevant -
			                      stats.branches.visited;
		               },
		               placeholder::color::faint_red);
	           },
	           show_column::other, with_branches_missing},
	    // GCOV_EXCL_END
	    column{"% Funcs"sv,
	           [](environment const& env,
	              cell_row& cells,
	              file_diff const& change) {
		           env.percentage(
		               cells, change,
		               [](auto const& stats) { return stats.functions; },
		               placeholder::color::bold_rating_functions);
	           },
	           show_column::other, with_funcs},
	    column{"Missing"sv,
	           [](environment const& env,
	              cell_row& cells,
	              file_diff const& change) {
		           env.dimmed_count(
		               cells, change,
		               [](auto const& stats) {
			               return stats.functions.relevant -
			                      stats.functions.visited;
		               },
		               placeholder::color::faint_red);
	           },
	           show_column::other, with_funcs_missing},
	    column{"% Lines"sv,
	           [](environment const& env,
	              cell_row& cells,
	              file_diff const& change) {
		           env.percentage(
		               cells, change,
		               [](auto const& stats) { return stats.lines; },
		               placeholder::color::bold_rating_lines);
	           }},
	    column{"Visited"sv,
	           [](environment const& env,
	              cell_row& cells,
	              file_diff const& change) {
		           env.count(cells, change, [](auto const& stats) {
			           return stats.lines.visited;
		           });
	           }},
	    column{"Relevant"sv,
	           [](environment const& env,
	              cell_row& cells,
	              file_diff const& change) {
		           env.count(
		               cells, change,
		               [](auto const& stats) { return stats.lines.relevant; },
		               placeholder::color::faint_normal);
	           }},
	    column{"Missing"sv,
	           [](environment const& env,
	              cell_row& cells,
	              file_diff const& change) {
		           env.dimmed_count(
		               cells, change,
		               [](auto const& stats) {
			               return stats.lines.relevant - stats.lines.visited;
		               },
		               placeholder::color::faint_red);
	           },
	           show_column::other, with_lines_missing},
	    column{"Line count"sv,
	           [](environment const& env,
	              cell_row& cells,
	              file_diff const& change) {
		           env.count(
		               cells, change,
		               [](auto const& stats) { return stats.lines_total; },
		               placeholder::color::faint_normal);
	           }},
	};

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

	std::string environment::color_for(placeholder::color clr,
	                                   io::v1::stats const* stats) const {
		if (!colorizer) return {};
		if (stats) clr = formatter::apply_mark(clr, *stats, marks);
		return colorizer(clr, app);
	}

	void environment::add(data_table& table,
	                      char type,
	                      entry_stats const& stats,
	                      std::string_view label,
	                      with flags,
	                      row_type row) const {
		using placeholder::color;

		auto change = stats.diff();
		std::vector<std::string> cells{};
		cells.reserve(columns.size() * 2);
		for (auto const& col : columns) {
			if (col.id == show_column::name) {
				cells.push_back({type});
				cells.push_back({label.data(), label.size()});
				continue;
			}
			col.data(*this, cells, change, flags);
		}

		table.emplace(std::move(cells), row == row_type::data);
	}

	void environment::print_table(std::vector<entry> const& entries) const {
		entry_stats total{};

		with flags{};

		for (auto const& entry : entries) {
			total.extend(entry.stats);

			if (entry.stats.current.lines.relevant !=
			    entry.stats.current.lines.visited)
				flags |= with::lines_missing;

			if (entry.stats.current.functions.relevant !=
			    entry.stats.current.functions.visited)
				flags |= with::functions_missing;

			if (entry.stats.current.branches.relevant !=
			    entry.stats.current.branches.visited)
				flags |= with::branches_missing;  // GCOV_EXCL_LINE for now
		}

		if (total.current.branches.relevant > 0) flags |= with::branches;
		if (total.current.functions.relevant > 0) flags |= with::functions;

		std::vector<table_column> header{};
		header.reserve(columns.size());
		for (auto const& col : columns) {
			col.header(header, flags);
		}

		data_table table{.header = std::move(header)};

		for (auto const& entry : entries) {
			auto entry_flag = '?';
			switch (entry.type) {
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

			add(table, entry_flag, entry.stats, entry.name.display, flags);
		}

		if (entries.size() > 1) {
			add(table, '>', total, "TOTAL"sv, flags, row_type::footer);
		}

		table.print();
	}
}  // namespace cov::app::show
