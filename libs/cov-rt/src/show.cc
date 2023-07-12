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

	std::string context::color_for(placeholder::color clr,
	                               io::v1::coverage_stats const* stats) const {
		if (!colorizer) return {};
		if (stats) clr = formatter::apply_mark(clr, *stats, marks);
		return colorizer(clr, app);
	}

	void context::add(data_table& table,
	                  char type,
	                  entry_stats const& stats,
	                  std::string_view label,
	                  row_type row) const {
		using placeholder::color;

		auto change = stats.diff();
		table.emplace(
		    {
		        {type},
		        {label.data(), label.size()},
		        apply_mark(val(change.coverage.current, "%"sv),
		                   change.stats.current),
		        val_sign(change.coverage.diff, color::faint_green, "%"sv),
		        val(change.stats.current.lines.visited),
		        val_sign(change.stats.diff.lines.visited),
		        val(change.stats.current.lines.relevant),
		        val_sign(change.stats.diff.lines.relevant, color::faint_normal),
		        // GCOV_EXCL_START[GCC]
		        val(change.stats.current.lines.relevant -
		            change.stats.current.lines.visited),
		        val_sign(change.stats.diff.lines.relevant -
		                     change.stats.diff.lines.visited,
		                 color::faint_red),
		        // GCOV_EXCL_STOP
		        val(change.stats.current.lines_total),
		        val_sign(change.stats.diff.lines_total, color::faint_normal),
		    },
		    row == row_type::data);
	}

	void context::print_table(std::vector<entry> const& entries) const {
		entry_stats total{};
		data_table table{.header = {
		                     {"NAME"s, '<'},  // GCOV_EXCL_LINE[GCC]
		                     "COVERAGE"s,
		                     "COVERED"s,
		                     "RELEVANT"s,
		                     "MISSING"s,
		                     "TOTAL"s,
		                 }};
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

			add(table, entry_flag, entry.stats, entry.name.display);
			total.extend(entry.stats);
		}

		if (entries.size() > 1)
			add(table, '>', total, "TOTAL"sv, row_type::footer);

		table.print();
	}
}  // namespace cov::app::show
