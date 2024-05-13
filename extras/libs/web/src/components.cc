// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/core/cvg_info.hh>
#include <cov/hash/md5.hh>
#include <cxx-filt/parser.hh>
#include <hilite/hilite.hh>
#include <hilite/lighter.hh>
#include <hilite/none.hh>
#include <native/path.hh>
#include <web/components.hh>
#include "../../../cov-api/src/cov/format/internal_environment.hh"

using namespace std::literals;

namespace cov::app::web {
	namespace {
		enum class icon_type {
			none,
			repository,
			module,
			directory,
			file,
			summary,
		};

		inline auto isspace(char c) {
			return std::isspace(static_cast<unsigned char>(c));
		}

		inline std::string_view lstrip(std::string_view view) {
			while (!view.empty() && isspace(view.front()))
				view = view.substr(1);
			return view;
		}

		inline std::string_view rstrip(std::string_view view) {
			while (!view.empty() && isspace(view.back()))
				view = view.substr(0, view.length() - 1);

			return view;
		}

		inline std::string_view strip(std::string_view view) {
			return rstrip(lstrip(view));
		}

		inline std::optional<std::string_view> prefixed(std::string_view prefix,
		                                                std::string_view line) {
			if (!line.starts_with(prefix)) return std::nullopt;
			return strip(line.substr(prefix.length()));
		}

		std::string_view subject_from(std::string_view message) {
			return strip(message.substr(0, message.find('\n')));
		}

		std::string_view body_from(std::string_view message) {
			auto const line = message.find('\n');
			if (line == std::string_view::npos) return {};
			return strip(message.substr(line + 1));
		}

		std::string str(std::string_view s) { return {s.data(), s.size()}; }

		static mstch::map mstch_person(placeholder::git_person const& person) {
			return {{"name", str(person.name)},
			        {"email", str(person.email)},
			        {"hash", hash::md5::once(
			                     {person.email.data(), person.email.length()})
			                     .str()}};
		}

		std::string formatted(placeholder::git_person const& person,
		                      placeholder::internal_environment& env,
		                      placeholder::person fld) {
			std::string result{};
			auto out = std::back_inserter(result);
			person.format(out, env, fld);
			return result;
		}

		std::string date_relative(placeholder::git_person const& person,
		                          placeholder::internal_environment& env) {
			return formatted(person, env, placeholder::person::date_relative);
		}

		std::string date_YmdHM(placeholder::git_person const& person,
		                       placeholder::internal_environment& env) {
			auto result =
			    formatted(person, env, placeholder::person::date_iso_strict);
			auto const pos = result.find('T');
			if (pos != std::string::npos) {
				result[pos] = ' ';
				auto max_length = std::min(pos + 6, result.length());
				result.erase(std::next(result.begin(),
				                       static_cast<std::ptrdiff_t>(max_length)),
				             result.end());
			}
			return result;
		}

		using namespace ::lighter;

		inline size_t length_of(highlighted_line const& items) {
			if (items.empty()) return 0u;
			return items.back().furthest_end();
		}

		static std::string_view hl_color(unsigned tok) {
			using namespace std::literals;
#define TOKEN_NAME(x) \
	case hl::x:       \
		return #x##sv;
			switch (static_cast<hl::token>(tok)) { HILITE_TOKENS(TOKEN_NAME) }
			return {};
#undef TOKEN_NAME
		}

		namespace id {
			static constexpr auto SPAN_OPEN = "<span class=\"hl-"sv;
			static constexpr auto SPAN_CHILDREN = "\">"sv;
			static constexpr auto SPAN_CLOSE = "</span>"sv;
			static constexpr auto AMP = "&amp;"sv;
			static constexpr auto LT = "&lt;"sv;
			static constexpr auto GT = "&gt;"sv;
			static constexpr auto QUOT = "&quot;"sv;
		}  // namespace id

#define HTML_X(X) \
	X('&', AMP)   \
	X('<', LT)    \
	X('>', GT)    \
	X('"', QUOT)

		struct paint {
			std::string_view line{};
			std::map<unsigned, std::string> const& dict;
			std::string_view text_from(text_span const& text) const noexcept {
				return line.substr(text.begin, text.end - text.begin);
			}

			std::string_view kind(unsigned key) const noexcept {
				auto kind =
				    key == hl::whitespace ? std::string_view{} : hl_color(key);

				if (kind.empty()) {
					auto it = dict.find(key);
					if (it != dict.end()) kind = it->second;
				}

				return kind;
			}

			size_t calc_length(std::string_view text) const noexcept {
				size_t len = 0;
				for (auto c : text) {
					switch (c) {
#define X_CASE(C, ID)           \
	case C:                     \
		len += id::ID.length(); \
		break;
						HTML_X(X_CASE)
#undef X_CASE
						default:
							++len;
					}
				}

				return len;
			}

			void append(std::string& out,
			            std::string_view text) const noexcept {
				for (auto c : text) {
					switch (c) {
#define X_CASE(C, ID)       \
	case C:                 \
		out.append(id::ID); \
		break;
						HTML_X(X_CASE)
#undef X_CASE
						default:
							out.push_back(c);
					}
				}
			}

			void append_dash(std::string& out,
			                 std::string_view text) const noexcept {
				for (auto c : text) {
					switch (c) {
#define X_CASE(C, ID)       \
	case C:                 \
		out.append(id::ID); \
		break;
						HTML_X(X_CASE)
#undef X_CASE
						case '_':
							out.push_back('-');
							break;
						default:
							out.push_back(c);
					}
				}
			}
		};

		struct html_length {
			paint pt;
			size_t operator()(text_span const& text) const noexcept {
				return pt.calc_length(pt.text_from(text));
			}
			size_t operator()(span const& s) const noexcept {
				return id::SPAN_OPEN.length() +
				       pt.calc_length(pt.kind(s.kind)) +
				       id::SPAN_CHILDREN.length() + (*this)(s.contents) +
				       id::SPAN_CLOSE.length();
			}
			size_t operator()(highlighted_line const& contents) const noexcept {
				size_t result = 0;
				for (auto const& item : contents) {
					result += std::visit(*this, item);
				}
				return result;
			}
		};

		struct html_text {
			paint pt;
			std::string& out;
			void operator()(text_span const& text) const noexcept {
				pt.append(out, pt.text_from(text));
			}
			void operator()(span const& s) const noexcept {
				out.append(id::SPAN_OPEN);
				pt.append_dash(out, pt.kind(s.kind));
				out.append(id::SPAN_CHILDREN);
				(*this)(s.contents);
				out.append(id::SPAN_CLOSE);
			}

			void operator()(highlighted_line const& contents) const noexcept {
				for (auto const& item : contents) {
					std::visit(*this, item);
				}
			}
		};

		std::string html_from(std::string_view line,
		                      std::map<unsigned, std::string> const& dict,
		                      highlighted_line const& contents) {
			auto length = html_length{
			    .pt{.line = line, .dict = dict},
			}(contents);
			std::string result{};
			result.reserve(length);
			html_text{
			    .pt{.line = line, .dict = dict},
			    .out = result,
			}(contents);
			return result;
		}

		ref_ptr<cov::files> get_files(git::oid_view id,
		                              cov::repository const& repo) {
			std::error_code ec{};
			auto generic = repo.lookup<cov::object>(id, ec);
			if (!generic || ec) return {};

			if (auto report = as_a<cov::report>(generic); report) {
				auto files =
				    repo.lookup<cov::files>(report->file_list_id(), ec);
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
	}  // namespace

	std::pair<mstch::map, mstch::map> add_build_info(cov::repository& repo,
	                                                 git::oid_view oid,
	                                                 git::oid_view from,
	                                                 std::error_code& ec) {
		std::pair<mstch::map, mstch::map> result{};
		auto& [commit_ctx, report_ctx] = result;

		auto report = repo.lookup<cov::report>(oid, ec);
		if (ec) return result;

		auto const facade =
		    placeholder::object_facade::present_report(report, &repo);

		auto formatter_env = placeholder::environment::from(
		    repo, cov::color_feature{cov::use_feature::no},
		    cov::decorate_feature{cov::use_feature::no});

		placeholder::internal_environment int_env{
		    .client = &formatter_env,
		    .app = nullptr,
		    .tr = formatter::no_translation,
		    .tz = date::locate_zone("Etc/UTC"sv),
		};

		if (auto const git = facade->git(); git) {
			bool the_same = git->author.email == git->committer.email &&
			                git->author.name == git->committer.name;
			commit_ctx = mstch::map{
			    {"subject", str(subject_from(git->message))},
			    {"description", str(body_from(git->message))},
			    {"different_author", !the_same},
			    {"author", mstch_person(git->author)},
			    {"committer", mstch_person(git->committer)},
			    {"branch", str(git->branch)},
			    {"commit-id", report->commit_id().str()},
			    {"date-relative", date_relative(git->committer, int_env)},
			    {"date-YmdHM", date_YmdHM(git->committer, int_env)},
			};
		}

		auto entries = report->entries();
		mstch::array properties{};
		properties.reserve(entries.size());
		for (auto const& build : entries) {
			auto const props = report::build::properties(build->props_json());
			mstch::array propset{};
			propset.reserve(props.size());
			for (auto const& [name, prop] : props) {
				auto const is_bool = std::holds_alternative<bool>(prop);
				auto const is_on = is_bool && std::get<bool>(prop);
				auto const is_number = std::holds_alternative<long long>(prop);
				auto const number = is_number ? std::get<long long>(prop) : 0;
				auto const is_string =
				    std::holds_alternative<std::string>(prop);
				auto const string =
				    is_string ? std::get<std::string>(prop) : ""s;

				propset.push_back(mstch::map{
				    {"is-flag", is_bool},
				    {"is-number", is_number},
				    {"is-string", is_string},
				    {"prop-type", is_bool     ? "flag"s
				                  : is_number ? "number"s
				                              : "string"s},
				    {"is-on", is_on},
				    {"value", is_bool     ? is_on ? "on"s : "off"s
				              : is_number ? std::to_string(number)
				                          : string},
				    {"name", name},
				});
			}

			properties.push_back(mstch::map{{"propset", std::move(propset)}});
		}

		auto reporter = placeholder::git_person{.date = report->add_time_utc()};

		auto label = from.str(9);
		auto base_ref_is_tag = false;
		auto base_ref_is_branch = false;

		{
			auto const refs = repo.refs()->iterator();
			for (auto const& ref : *refs) {
				auto const peeled = ref->peel_target();
				if (peeled && peeled->direct_target() &&
				    from == *peeled->direct_target()) {
					if (ref->references_tag()) {
						base_ref_is_tag = true;
						label = ref->shorthand();
						break;
					}
					if (!base_ref_is_branch && ref->references_branch()) {
						base_ref_is_branch = true;
						label = ref->shorthand();
						continue;
					}
				}
			}
		}

		report_ctx = mstch::map{
		    {"report-id", report->oid().str()},
		    {"base",
		     mstch::map{
		         {"present", !from.is_zero()},
		         {"id", from.str()},
		         {"label", label},
		         {"is-tag", base_ref_is_tag},
		         {"is-branch", base_ref_is_branch},
		     }},
		    {"properties", std::move(properties)},
		    {"date-relative", date_relative(reporter, int_env)},
		    {"date-YmdHM", date_YmdHM(reporter, int_env)},
		};

		return result;
	}

	void add_mark(mstch::map& ctx, translatable mark) {
		ctx["is-passing"] = false;
		ctx["is-incomplete"] = false;
		ctx["is-failing"] = false;
		switch (mark) {
			case translatable::mark_passing:
				ctx["mark"] = "passing"s;
				ctx["is-passing"] = true;
				break;
			case translatable::mark_incomplete:
				ctx["mark"] = "incomplete"s;
				ctx["is-incomplete"] = true;
				break;
			case translatable::mark_failing:
				ctx["mark"] = "failing"s;
				ctx["is-failing"] = true;
				break;
			default:
				break;
		};
	}

	std::vector<std::pair<std::string, std::string>> make_breadcrumbs(
	    std::string_view crumbs,
	    projection::entry_type type,
	    link_service const& links) {
		auto path = make_u8path(crumbs);
		decltype(path) expanded{};

		std::vector<std::pair<std::string, std::string>> result;

		for (auto const& segment : path) {
			if (expanded.empty())
				expanded = segment;
			else
				expanded /= segment;
			projection::label entry{.display = get_u8path(segment),
			                        .expanded = get_u8path(expanded)};
			auto link = links.link_for(type, entry);
			result.emplace_back(std::move(link), std::move(entry.display));
		}

		return result;
	}

	void add_navigation(mstch::map& ctx,
	                    bool is_root,
	                    bool is_module,
	                    bool is_file,
	                    std::string_view path,
	                    link_service const& links) {
		auto breadcrumbs = mstch::map{{
		    "heading"s,
		    mstch::map{{"href"s, links.resource_link("index.html")},
		               {"display"s, "repository"s}},
		}};

		if (!is_root) {
			auto crumbs =
			    make_breadcrumbs(path,
			                     is_module ? projection::entry_type::module
			                               : projection::entry_type::directory,
			                     links);
			breadcrumbs["leaf"] = crumbs.back().second;
			crumbs.pop_back();
			auto nav = mstch::array{};
			nav.reserve(crumbs.size());
			for (auto const& [href, display] : crumbs) {
				nav.push_back(
				    mstch::map{{"display"s, display}, {"href"s, href}});
			}

			breadcrumbs["nav"] = std::move(nav);
		}

		ctx["is-root"] = is_root;
		ctx["is-module"] = !is_root && is_module;
		ctx["is-file"] = !is_root && !is_module && is_file;
		ctx["is-directory"] = !is_root && !is_module && !is_file;
		ctx["breadcrumbs"] = std::move(breadcrumbs);
	}

	struct add_table_row_options {
		std::string_view label{};
		icon_type icon{};
		std::string_view link{};
		std::string_view class_name{};
		std::span<core::cell_info const> cells{};
		std::span<core::column_info const> columns{};
	};

	std::string_view class_from_priority(core::col_priority priority) {
		switch (priority) {
			case core::col_priority::key:
				return "priority-key"sv;
			case core::col_priority::high:
				return "priority-high"sv;
			case core::col_priority::supplemental:
				return "priority-supplemental"sv;
		}
		return ""sv;
	}

	mstch::map add_table_row(add_table_row_options const& options) {
		std::string_view display{}, prefix{};
		auto const pos = options.label.rfind('/');
		if (pos == std::string_view::npos) {
			display = options.label;
		} else {
			prefix = options.label.substr(0, pos + 1);
			display = options.label.substr(pos + 1);
		}

		mstch::map ctx{
		    {"class-name", str(options.class_name)},
		    {"display", str(display)},
		    {"has-prefix", !prefix.empty()},
		    {"has-href", !options.link.empty()},
		    {"is-module", options.icon == icon_type::module},
		    {"is-directory", options.icon == icon_type::directory},
		    {"is-file", options.icon == icon_type::file},
		    {"is-total", options.icon == icon_type::summary},
		};

		if (!prefix.empty()) ctx["prefix"] = str(prefix);
		if (!options.link.empty()) ctx["href"] = str(options.link);

		mstch::array cells{};
		cells.reserve(options.cells.size());

		static constinit auto const dummy_column =
		    core::column_info::from<core::col_title::lines_total,
		                            core::col_priority::supplemental,
		                            core::col_data::counter>();
		auto it = options.columns.begin();
		for (auto const& cell : options.cells) {
			auto const& column =
			    it != options.columns.end() ? *it++ : dummy_column;

			auto prio = class_from_priority(column.priority);

			if (cell.value.empty() && cell.change.empty()) {
				cells.push_back(mstch::map{
				    {"is-simple", true},
				    {"value-classes", std::string{prio.data(), prio.size()}},
				    {"change-classes", std::string{prio.data(), prio.size()}},
				});
				continue;
			}

			auto value = cell.value;
			auto change = cell.change;
			if (value.empty() && !cell.change.empty())
				value = column.data_type == core::col_data::counter ? "0"s
				                                                    : "0.00"s;

			std::vector<std::string_view> value_classes{}, change_classes{};
			value_classes.reserve(5);
			change_classes.reserve(4);

			value_classes.push_back("value"sv);
			change_classes.push_back("diff"sv);

			{
				auto change_class =
				    cell.change.empty() ||
				            column.priority == core::col_priority::supplemental
				        ? ""sv
				    : cell.change_is_negative ? "down"sv
				                              : "up"sv;
				if (!change_class.empty())
					change_classes.push_back(change_class);
			}

			if (column.data_type == core::col_data::percentage) {
				if (!value.empty()) value.push_back('%');
				if (!change.empty()) change.push_back('%');
			}

			if (value.empty()) value.assign("\xc2\xa0"sv);
			if (change.empty()) change.assign("\xc2\xa0"sv);

			using enum core::value_category;
			auto color = cell.category == passing      ? "passing"sv
			             : cell.category == incomplete ? "incomplete"sv
			                                           : "failing"sv;
			auto bg_color = fmt::format("{}-bg", color);

			if (column.priority == core::col_priority::high) {
				value_classes.push_back("bold"sv);
				value_classes.push_back(color);
				value_classes.push_back(bg_color);
				change_classes.push_back(bg_color);
			}

			value_classes.push_back(prio);
			change_classes.push_back(prio);

			cells.push_back(mstch::map{
			    {"is-simple", false},
			    {"value", str(value)},
			    {"change", str(change)},
			    {"value-classes",
			     fmt::format("{}", fmt::join(value_classes, " "))},
			    {"change-classes",
			     fmt::format("{}", fmt::join(change_classes, " "))},
			});
		}

		ctx["cells"] = std::move(cells);

		return ctx;
	}

	void add_table(mstch::map& ctx,
	               core::projected_entries const& projection,
	               link_service const& links) {
		if (projection.rows.size() == 0) return;

		mstch::map stats_ctx{};
		{
			mstch::array columns{};
			columns.reserve(projection.columns.size() + 1);
			columns.push_back(
			    mstch::map{{"is-name", true}, {"display", "Name"}});
			for (auto const col : projection.columns) {
				auto column_ctx = mstch::map{};

				auto display = ""sv;
				switch (col.title) {
					case core::col_title::branches_covered:
						display = "Branches"sv;
						break;
					case core::col_title::branches_relevant:
						display = "Relevant"sv;
						break;
					case core::col_title::branches_missing:
						display = "Missing"sv;
						break;
					case core::col_title::functions_covered:
						display = "Functions"sv;
						break;
					case core::col_title::functions_relevant:
						display = "Relevant"sv;
						break;
					case core::col_title::functions_missing:
						display = "Missing"sv;
						break;
					case core::col_title::lines_covered:
						display = "Lines"sv;
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

				ctx["is-name"] = false;
				ctx["is-branches-covered"] =
				    col.title == core::col_title::branches_covered;
				ctx["is-branches-relevant"] =
				    col.title == core::col_title::branches_relevant;
				ctx["is-branches-missing"] =
				    col.title == core::col_title::branches_missing;
				ctx["is-functions-covered"] =
				    col.title == core::col_title::functions_covered;
				ctx["is-functions-relevant"] =
				    col.title == core::col_title::functions_relevant;
				ctx["is-functions-missing"] =
				    col.title == core::col_title::functions_missing;
				ctx["is-lines-covered"] =
				    col.title == core::col_title::lines_covered;
				ctx["is-lines-missing"] =
				    col.title == core::col_title::lines_missing;
				ctx["is-lines-visited"] =
				    col.title == core::col_title::lines_visited;
				ctx["is-lines-relevant"] =
				    col.title == core::col_title::lines_relevant;
				ctx["is-lines-total"] =
				    col.title == core::col_title::lines_total;

				auto prio = ""s;
				switch (col.priority) {
					case core::col_priority::key:
						prio = "key"s;
						break;
					case core::col_priority::high:
						prio = "high"s;
						break;
					case core::col_priority::supplemental:
						prio = "supplemental"s;
						break;
				}

				if (!display.empty()) {
					column_ctx["display"] = str(display);
				}

				if (!prio.empty()) column_ctx["priority"] = prio;

				columns.push_back(std::move(column_ctx));
			}

			stats_ctx["columns"] = std::move(columns);
		}

		mstch::array rows{};
		rows.reserve(projection.rows.size());
		for (auto const& row : projection.rows) {
			std::string_view class_name{};
			icon_type icon{};

			switch (row.type) {
				case projection::entry_type::module:
					icon = icon_type::module;
					class_name = "module"sv;
					break;
				case projection::entry_type::directory:
					icon = icon_type::directory;
					class_name = "dir"sv;
					break;
				case projection::entry_type::standalone_file:
				case projection::entry_type::file:
					icon = icon_type::file;
					class_name = "file"sv;
					break;
			}
			auto const link = links.link_for(row.type, row.name);
			rows.push_back(add_table_row({
			    .label = row.name.display,
			    .icon = icon,
			    .link = link,
			    .class_name = class_name,
			    .cells = row.data,
			    .columns = projection.columns,
			}));
		}

		stats_ctx["rows"] = std::move(rows);

		if (!projection.footer.empty()) {
			stats_ctx["total"] = add_table_row({
			    .label = "Total"sv,
			    .icon = icon_type::summary,
			    .class_name = "total"sv,
			    .cells = projection.footer,
			    .columns = projection.columns,
			});
		} else {
			stats_ctx["total"] = false;
		}

		ctx["stats"] = std::move(stats_ctx);
	}

	void add_file_source(mstch::map& ctx,
	                     cov::repository& repo,
	                     git::oid_view ref,
	                     std::string_view path,
	                     cxx_filt::Replacements const& replacements,
	                     std::error_code& ec) {
		ctx["file-is-present"] = false;

		auto const files = get_files(ref, repo);
		if (!files) return;

		auto const* file_entry = files->by_path(path);
		if (!file_entry || file_entry->contents().is_zero()) {
			return;
		}

		core::cvg_info cvg{};
		bool with_functions{true};

		if (!file_entry->line_coverage().is_zero()) {
			auto const file_cvg = repo.lookup<cov::line_coverage>(
			    file_entry->line_coverage(), ec);
			if (file_cvg && !ec)
				cvg = core::cvg_info::from_coverage(file_cvg->coverage());
		}

		if (with_functions && !file_entry->function_coverage().is_zero()) {
			auto const file_cvg = repo.lookup<cov::function_coverage>(
			    file_entry->function_coverage(), ec);
			if (file_cvg && !ec) cvg.add_functions(*file_cvg);
		}

		cvg.find_chunks();

		auto const data = file_entry->get_contents(repo, ec);
		if (ec) return;

		cvg.load_syntax(
		    std::string_view{reinterpret_cast<char const*>(data.data()),
		                     data.size()},
		    path);

		auto fn = cvg.funcs();

		ctx["file-is-present"] = true;
		mstch::array chunks_ctx;
		chunks_ctx.reserve(cvg.chunks.size());

		auto const last_line =
		    std::max(1u, static_cast<unsigned>(cvg.syntax.lines.size())) - 1;
		ctx["last-line"] = last_line;

		auto first = true;
		for (auto const& [start, stop] : cvg.chunks) {
			mstch::map chunk_ctx{
			    {"missing-before", first && start > 0},
			    {"missing-after", stop < last_line},
			};
			first = false;

			mstch::array lines_ctx{};
			lines_ctx.reserve(stop - start + 1);

			bool ran_out_of_lines{false};
			for (auto line_no = start; line_no <= stop; ++line_no) {
				if (!cvg.has_line(line_no)) {
					ran_out_of_lines = true;
					break;
				}

				mstch::array fn_ctx{};

				fn.at(line_no, [=, &fn_ctx,
				                &replacements](auto const& function) {
					auto const aliases = core::cvg_info::soft_alias(function);
					fn_ctx.reserve(fn_ctx.size() + aliases.size());
					for ([[maybe_unused]] auto const& alias : aliases) {
						auto fn = mstch::map{
						    {"name",
						     cxx_filt::Parser::statement_from(alias.label)
						         .simplified(replacements)
						         .str()},
						    {"count", alias.count},
						    {"class-name",
						     alias.count ? "passing"s : "failing"s},
						};

						fn_ctx.push_back(std::move(fn));
					}
				});

				auto const count = cvg.count_for(line_no);

				auto const& line = cvg.syntax.lines[line_no];
				auto const length = length_of(line.contents);
				auto const line_text = cvg.file_text.substr(line.start, length);

				// TODO: add rest of the line
				lines_ctx.push_back(mstch::map{
				    {"line-no", line_no + 1},
				    {"functions", std::move(fn_ctx)},
				    {"is-null", !count},
				    {"count", count.value_or(0)},
				    {"class-name", !count   ? "none"s
				                   : *count ? "passing"s
				                            : "failing"s},
				    {"text",
				     html_from(line_text, cvg.syntax.dict, line.contents)},
				});
			}

			if (!lines_ctx.empty()) {
				chunk_ctx["lines"] = std::move(lines_ctx);
				chunks_ctx.push_back(std::move(chunk_ctx));
			}

			if (ran_out_of_lines) break;
		}

		ctx["file-is-empty"] = chunks_ctx.empty();
		ctx["file-chunks"] = std::move(chunks_ctx);
	}
}  // namespace cov::app::web
