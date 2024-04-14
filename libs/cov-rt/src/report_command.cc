// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/app/path.hh>
#include <cov/app/report_command.hh>
#include <cov/app/rt_path.hh>
#include <cov/app/tools.hh>
#include <cov/format.hh>
#include <cov/io/file.hh>
#include <json/json.hpp>

namespace cov::app::builtin::report {
	using namespace std::literals;
	using namespace std::filesystem;

	namespace {
		std::string quoted(std::string_view item) {
			return fmt::format("\xE2\x80\x98{}\xE2\x80\x99", item);
		}

		struct oid_ptr {
			git_oid const* ref;
			bool operator==(oid_ptr const& right) const noexcept {
				if (!ref) return !right.ref;
				if (!right.ref) return false;
				return git_oid_equal(ref, right.ref);
			}
		};

		bool same_coverage(cov::repository& repo,
		                   git::oid_view left_id,
		                   git::oid_view right_id) {
			std::error_code ec{};
			auto const left = repo.lookup<cov::files>(left_id, ec);
			if (ec || !left) return false;
			auto const right = repo.lookup<cov::files>(right_id, ec);
			if (ec || !right) return false;

			std::map<std::string_view,
			         std::pair<cov::io::v1::coverage_stats, oid_ptr>>
			    left_data, right_data;
			for (auto const& entry : left->entries()) {
				left_data[entry->path()] = {entry->stats(),
				                            {&entry->contents().id}};
			}
			for (auto const& entry : right->entries()) {
				right_data[entry->path()] = {entry->stats(),
				                             {&entry->contents().id}};
			}

			return left_data == right_data;
		}
	}  // namespace

	parser::parser(::args::args_view const& arguments,
	               str::translator_open_info const& langs)
	    : base_parser<errlng, replng>{langs, arguments} {
		static constexpr std::string_view filters[] = {
		    "cobertura"sv, "coveralls"sv, "strip-excludes"sv};

		parser_.arg(report_)
		    .meta(tr_(replng::REPORT_FILE_META))
		    .help(tr_(replng::REPORT_FILE_DESCRIPTION));
		parser_.arg(filter_, "f", "filter")
		    .meta(tr_(replng::FILTER_META))
		    .help(tr_.format(replng::FILTER_DESCRIPTION, quoted_list(filters)));
		parser_.arg(props_, "p", "prop")
		    .meta(tr_(replng::PROP_META))
		    .help(tr_(replng::PROP_DESCRIPTION))
		    .opt();
		parser_.set<std::true_type>(amend_, "amend")
		    .help(tr_(replng::AMEND_DESCRIPTION))
		    .opt();
	}

	parser::parse_results parser::parse() {
		using namespace str;

		auto rest = parser_.parse(::args::parser::allow_subcommands);

		if (!rest.empty()) {
			if (rest[0] != "--"sv) {
				error(fmt::format(fmt::runtime(tr_(args::lng::UNRECOGNIZED)),
				                  rest[0]));
			}
			rest = rest.shift();
		}

		parse_results result{open_here(*this, tr_)};

		if (!result.report.load_from_text(
		        report_contents(result.repo.git(), rest))) {
			if (filter_) {
				error(tr_.format(replng::ERROR_FILTERED_REPORT_ISSUES, report_,
				                 *filter_));
			} else {  // GCOV_EXCL_LINE[WIN32]
				error(tr_.format(replng::ERROR_REPORT_ISSUES, report_));
			}
		}
		result.props = cov::report::builder::properties(props_);
		return result;
	}  // GCOV_EXCL_LINE[GCC] -- and now it wants to throw something...

	std::string parser::report_contents(git::repository_handle repo,
	                                    ::args::arglist args) const {
		auto source = io::fopen(make_u8path(report_));
		if (!source) error(tr_.format(str::args::lng::FILE_NOT_FOUND, report_));

		auto content = source.read();
		if (filter_) {
			auto dir = repo.work_dir();
			if (!dir) dir = repo.common_dir();
			content = filter(content, *filter_, args, make_u8path(*dir));
		}

		return {reinterpret_cast<char const*>(content.data()), content.size()};
	}

	std::vector<std::byte> parser::filter(
	    std::vector<std::byte> const& contents,
	    std::string_view filter,
	    ::args::arglist args,
	    std::filesystem::path const& cwd) const {
		auto output = platform::run_filter(
		    platform::sys_root() / directory_info::share / "filters"sv, cwd,
		    filter, args, contents);

		if (!output.error.empty())
			fwrite(output.error.data(), 1, output.error.size(), stderr);

		if (!output.output.empty()) {
			const auto value = json::read_json(
			    {reinterpret_cast<char8_t const*>(output.output.data()),
			     output.output.size()},
			    {}, json::read_mode::ECMA);
			if (std::holds_alternative<std::monostate>(value)) {
				fwrite(output.output.data(), 1, output.output.size(), stdout);
				output.output.clear();
			}
		}

		if (output.return_code) {
			if (output.return_code == -ENOENT)
				data_error(replng::ERROR_FILTER_NOENT, filter);
			if (output.return_code == -EACCES)
				data_error(replng::ERROR_FILTER_ACCESS, filter);
			data_error(replng::ERROR_FILTER_FAILED, filter, output.return_code);
		}

		return std::move(output.output);
	}

	bool parser::store_build(git::oid& out,
	                         cov::repository& repo,
	                         git::oid_view file_list_id,
	                         date::sys_seconds add_time_utc,
	                         io::v1::coverage_stats const& stats,
	                         std::string_view props) {
		auto const build =
		    cov::build::create(file_list_id, add_time_utc, props, stats);
		return repo.write(out, build);
	}

	new_head parser::update_current_branch(
	    cov::repository& repo,
	    git::oid_view file_list_id,
	    git_info const& git,
	    git_commit const& commit,
	    date::sys_seconds add_time_utc,
	    io::v1::coverage_stats const& stats,
	    std::vector<std::unique_ptr<cov::report::build>>&& builds) {
		new_head result{};

		git_oid commit_id;
		git_oid_fromstrn(&commit_id, git.head.data(), git.head.length());

		while (true) {
			std::error_code ec{};
			auto const HEAD = repo.current_head();
			git::oid parent_id{};
			if (HEAD.tip) parent_id = *HEAD.tip;
			if (amend_) {
				auto current = HEAD.tip
				                   ? repo.lookup<cov::report>(parent_id, ec)
				                   : ref_ptr<cov::report>{};
				if (!current) {
					auto const msg = tr_(replng::ERROR_AMEND_IN_FRESH_REPO);
					error({msg.data(), msg.size()});
				}
				parent_id = current->parent_id();
			}

			auto parent = repo.lookup<cov::report>(parent_id, ec);
			if (parent) {
				const bool hard_equiv = commit_id == parent->commit_id() &&
				                        git.branch == parent->branch();
				const bool loose_equiv =
				    file_list_id == parent->file_list_id() ||
				    same_coverage(repo, file_list_id, parent->file_list_id());
				if (hard_equiv && loose_equiv) {
					auto HEAD_2 = repo.current_head();
					result.branch = std::move(HEAD_2.branch);
					if (HEAD_2.tip) result.tip = *HEAD_2.tip;
					result.same_report = true;
					break;
				}  // GCOV_EXCL_LINE[WIN32]
			}

			auto const report_obj = cov::report::create(
			    parent_id, file_list_id, commit_id, git.branch,
			    {commit.author.name, commit.author.mail},
			    {commit.committer.name, commit.committer.mail}, commit.message,
			    sys_seconds{commit.committed}, add_time_utc, stats,
			    std::move(builds));

			if (!repo.write(result.tip, report_obj)) {
				// GCOV_EXCL_START
				[[unlikely]];
				data_error(replng::ERROR_CANNOT_WRITE_TO_DB);
			}  // GCOV_EXCL_STOP

			if (repo.update_current_head(result.tip, HEAD)) {
				size_t size = HEAD.branch.size();
				for (auto const c : HEAD.branch)
					if (c == '%') ++size;
				result.branch.clear();
				result.branch.reserve(size);
				for (auto const c : HEAD.branch) {
					if (c == '%') result.branch.push_back('%');
					result.branch.push_back(c);
				}
				break;
			}
		}  // GCOV_EXCL_LINE[WIN32]

		return result;
	}  // GCOV_EXCL_LINE[GCC]

	void parser::print_report(std::string_view local_branch,
	                          size_t files,
	                          ref_ptr<cov::report> const& report,
	                          str::cov_report::Strings const& tr,
	                          repository const& repo) {
		static constexpr auto message_chunk1 = "%C(red)["sv;
		static constexpr auto message_chunk2 = " %hR]%Creset %s%n "sv;
		static constexpr auto message_chunk3 =
		    ", %C(rating:L)%pPL%Creset (%pVL/%pTL"sv;
		static constexpr auto message_chunk4 =
		    ")"
		    "%{?pTF[, Fn%C(rating:F)%pPF%Creset (%pVF/%pTF)%]}"
		    "%{?pTB[, Br%C(rating:B)%pPB%Creset (%pVB/%pTB)%]}"
		    "%n"
		    " %C(faint normal)"sv;
		static constexpr auto message_chunk5 =
		    "%Creset %C(faint yellow)%hG@%rD%Creset%n"sv;
		static constexpr auto message_chunk6 = "%{B[ %C(faint normal)"sv;
		static constexpr auto message_chunk7 =
		    " %hR:%Creset %C(rating)%pPL%Creset%mz%n%]}"sv;

		using str::cov_report::counted;

		auto const& stats = report->stats();

		std::string change{}, parent{};
		std::string file_count =
		    fmt::format(fmt::runtime(tr(counted::MESSAGE_FILE_COUNT,
		                                static_cast<intmax_t>(files))),
		                files);

		if (stats.lines.visited < stats.lines.relevant) {
			change = fmt::format(", -{}"sv,
			                     stats.lines.relevant - stats.lines.visited);
		}

		if (!report->parent_id().is_zero()) {
			parent = fmt::format(" {} %hP%n"sv,
			                     tr(replng::MESSAGE_FIELD_PARENT_REPORT));
		}

		std::string_view chunks[] = {
		    message_chunk1,
		    local_branch.empty() ? tr(replng::MESSAGE_DETACHED_HEAD)
		                         : local_branch,
		    message_chunk2,
		    file_count,
		    message_chunk3,
		    change,
		    message_chunk4,
		    tr(replng::MESSAGE_FIELD_GIT_COMMIT),
		    message_chunk5,
		    parent,
		    message_chunk6,
		    tr(replng::MESSAGE_FIELD_CONTAINS_BUILD),
		    message_chunk7,
		};

		std::string format{};
		size_t length = 0;
		for (auto chunk : chunks)
			length += chunk.size();
		format.reserve(length);
		for (auto chunk : chunks)
			format.append(chunk);

		auto facade =
		    placeholder::object_facade::present_report(report, nullptr);
		auto env = placeholder::environment::from(repo, use_feature::yes,
		                                          use_feature::yes);
		env.prop_names = false;
		auto const message = formatter::from(format).format(facade.get(), env);

		std::fputs(message.c_str(), stdout);
	}

	std::string parser::quoted_list(std::span<std::string_view const> names) {
		auto begin = std::begin(names);
		auto end = std::next(std::end(names), -1);
		std::string result{};
		bool first = true;
		for (auto it = begin; it != end; ++it) {
			if (first)
				first = false;
			else                        // GCOV_EXCL_LINE
				result.append(", "sv);  // GCOV_EXCL_LINE
			result.append(quoted(*it));
		}

		auto last = quoted(*end);
		return tr_.format(replng::FILTER_DESCRIPTION_LIST_END, result, last);
	}

	std::vector<stored_file> stored_file::from(
	    git_commit const& commit,
	    std::vector<file_info> const& info,
	    parser const& p) {
		std::vector<stored_file> result{};
		result.reserve(info.size());

		for (auto const& file : info) {
			result.push_back(from(commit, file));

			if (result.back().stg.flags == text::missing)
				p.data_error(replng::ERROR_CANNOT_FIND_FILE, file.name);

			if ((result.back().stg.flags & text::mismatched) ==
			    text::mismatched)
				p.data_warning(replng::WARNING_FILE_MODIFIED, file.name);
		}

		return result;
	}  // GCOV_EXCL_LINE[GCC]

	void stored_file::store(cov::repository& repo,
	                        file_info const& info,
	                        parser const& p) {
		if (!store_coverage(repo, info)) {
			// GCOV_EXCL_START
			[[unlikely]];
			p.data_error(replng::ERROR_CANNOT_WRITE_TO_DB);
		}  // GCOV_EXCL_STOP

		if ((stg.flags & text::in_repo) != text::in_repo ||
		    (stg.flags & text::mismatched) == text::mismatched) {
			auto const work_dir = repo.git_work_dir();
			if (!work_dir) {
				// GCOV_EXCL_START
				// we got here through either in_repo | in_fs |
				// mismatched, or in_fs | mismatched; this means we got
				// here with at least in_fs; making a test, which would
				// allow this to happen and yet failed here seems
				// infeasible
				[[unlikely]];
				p.data_error(replng::ERROR_BARE_GIT);
			}  // GCOV_EXCL_STOP

			auto const opened =
			    io::fopen(make_u8path(*work_dir) / make_u8path(info.name), "r");
			if (!opened) {
				// GCOV_EXCL_START
				// same here: we read this file once already...
				[[unlikely]];
				p.data_error(replng::ERROR_CANNOT_OPEN_FILE, info.name);
			}  // GCOV_EXCL_STOP

			auto bytes = opened.read();
			if (!store_contents(repo, {bytes.data(), bytes.size()})) {
				// GCOV_EXCL_START
				[[unlikely]];
				p.data_error(replng::ERROR_CANNOT_WRITE_TO_DB);
			}  // GCOV_EXCL_STOP
		}
	}

	bool stored_file::store_coverage(cov::repository& repo,
	                                 file_info const& info) {
		std::vector<io::v1::coverage> cvg{};
		std::tie(cvg, stats) = info.expand_coverage(stg.lines);
		auto const obj_cvg = cov::line_coverage::create(std::move(cvg));

		cov::function_coverage::builder builder{};
		builder.reserve(info.function_coverage.size());
		for (auto const& fun : info.function_coverage) {
			builder.add(fun.name, fun.demangled_name, fun.count, fun.start,
			            fun.end);
		}

		auto const obj_functions = builder.extract();
		auto const aliases = obj_functions->merge_aliases();

		for (auto const& fun : aliases) {
			++stats.functions.relevant;
			if (fun.count) ++stats.functions.visited;
		}

		return repo.write(lines_id, obj_cvg) &&
		       repo.write(functions_id, obj_functions);
	}

	bool stored_file::store_contents(cov::repository& repo,
	                                 git::bytes const& contents) {
		return repo.write(stg.existing, contents);
	}

	bool stored_file::store_tree(git::oid& id,
	                             cov::repository& repo,
	                             std::vector<file_info> const& file_infos,
	                             std::vector<stored_file> const& files) {
		cov::files::builder builder{};
		auto it = files.begin();
		for (auto const& file : file_infos) {
			auto& compiled = *it++;
			builder.add(file.name, compiled.stats, compiled.stg.existing,
			            compiled.lines_id, compiled.functions_id,
			            compiled.branches_id);
		}

		auto const obj = builder.extract(git::oid{});
		return repo.write(id, obj);
	}

}  // namespace cov::app::builtin::report
