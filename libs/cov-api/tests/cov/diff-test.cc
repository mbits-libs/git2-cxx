// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <cov/git2/commit.hh>
#include <cov/git2/error.hh>
#include <cov/git2/global.hh>
#include <cov/io/file.hh>
#include <cov/report.hh>
#include <cov/repository.hh>
#include <span>
#include "setup.hh"

namespace cov {
	using namespace std::literals;

	static std::string_view name_for(file_diff::kind kind) {
		switch (kind) {
			case file_diff::normal:
				return "file_diff::normal"sv;
			case file_diff::renamed:
				return "file_diff::renamed"sv;
			case file_diff::copied:
				return "file_diff::copied"sv;
			case file_diff::added:
				return "file_diff::added"sv;
			case file_diff::deleted:
				return "file_diff::deleted"sv;
		}
		return "???"sv;
	}

	void PrintTo(file_stats const& file, std::ostream* out) {
		*out << "{\"" << file.filename << "\"s, {" << file.current.lines_total
		     << ", " << file.current.lines.relevant << ", "
		     << file.current.lines.visited << "}, {"
		     << file.previous.lines_total << ", "
		     << file.previous.lines.relevant << ", "
		     << file.previous.lines.visited << "},";
		if (file.previous_name.empty())
			*out << "{}";
		else
			*out << '"' << file.previous_name << "\"s";
		*out << ", " << name_for(file.diff_kind) << '}';
	}
}  // namespace cov

namespace cov::testing {

#if 1
	struct quick_stat {
		std::string_view path;
		io::v1::coverage_stats stats{};

		std::unique_ptr<cov::files::entry> with(git::tree const& tree) const {
			files::builder::info nfo{
			    .path = path,
			    .stats = stats,
			};

			std::error_code ignore;
			auto entry = tree.entry_bypath(path.data(), ignore);
			if (!ignore && entry) nfo.contents.assign(entry.oid());

			auto list_of_one = files::builder{}.add_nfo(nfo).release();
			return std::move(list_of_one.front());
		}
	};

	std::error_code add_report(repository& repo,
	                           git::repository const& git_repo,
	                           git::oid& report,
	                           std::string_view commit_id,
	                           std::span<quick_stat const> files,
	                           git::oid const* parent = nullptr) {
		std::error_code ec{};
		auto const now = std::chrono::floor<std::chrono::seconds>(
		    std::chrono::system_clock::now());

		std::cout << "commit " << commit_id << '\n';
		if (parent) std::cout << "parent " << *parent << '\n';
		auto commit = git_repo.lookup<git::commit>(commit_id, ec);
		if (ec) return ec;

		auto tree = commit.tree(ec);
		if (ec) return ec;

		auto stats = io::v1::coverage_stats::init();
		std::vector<std::unique_ptr<cov::files::entry>> entries{};
		entries.reserve(files.size());
		for (auto const& file : files) {
			stats += file.stats;
			entries.emplace_back(file.with(tree));
		}

		git::oid peeled_id{};
		auto peeled = files::create(git::oid{}, std::move(entries));
		if (!repo.write(peeled_id, peeled)) {
			return git::make_error_code(git::errc::error);
		}
		std::cout << "files " << peeled_id.str() << '\n';

		cov::report::builder builds{};
		{
			git::oid build_id{};
			auto const build = cov::build::create(peeled_id, now, {}, stats);
			if (!repo.write(build_id, build)) {
				return git::make_error_code(git::errc::error);
			}
			builds.add_nfo({.build_id = build_id, .props = {}, .stats = stats});
			std::cout << "build " << build_id << '\n';
		}

		auto author = commit.author();
		auto committer = commit.committer();

		git::oid zero{};
		auto result = cov::report::create(
		    parent ? *parent : zero, peeled_id, commit.oid(), "main"sv,
		    {author.name, author.email}, {committer.name, committer.email},
		    commit.message_raw(), committer.when, now, stats, builds.release());

		if (!repo.write(report, result)) {
			return git::make_error_code(git::errc::error);
		}
		std::cout << "report " << report << '\n';
		return {};
	}

	TEST(diff, install) {
		git::init app{};
		std::error_code ec{};
		remove_all(setup::test_dir() / "diffs/.git/.covdata"sv, ec);
		auto repo = cov::repository::init(
		    setup::test_dir(), setup::test_dir() / "diffs/.git/.covdata"sv,
		    setup::test_dir() / "diffs/.git"sv, ec);
		ASSERT_FALSE(ec) << ec.message();

		auto git_repo =
		    git::repository::open(setup::test_dir() / "diffs/.git"sv, ec);
		ASSERT_FALSE(ec) << ec.message();

		static constexpr quick_stat initial_files[] = {
		    {"main.cc"sv, {5, 1, 0, 0, 0, 0, 0}},
		};

		static constexpr quick_stat middle_files[] = {
		    {"src/main.cc"sv, {5, 1, 0, 0, 0, 0, 0}},
		    {"src/old-name.cc"sv, {6, 1, 0, 0, 0, 0, 0}},
		};

		static constexpr quick_stat current_files[] = {
		    {"src/main.cc"sv, {5, 1, 1, 0, 0, 0, 0}},
		    {"src/greetings.cc"sv, {8, 3, 3, 0, 0, 0, 0}},
		};

		git::oid initial{}, middle{}, current{};
		ec = add_report(repo, git_repo, initial,
		                "c86a87d7b7ac836d4686f4f2a4e251ac4c6e1366"sv,
		                initial_files);
		ASSERT_FALSE(ec) << ec.message();
		std::cout << '\n';
		ec = add_report(repo, git_repo, middle,
		                "d925136ac6bb564818b6e6b9869ebf871ea07e67"sv,
		                middle_files, &initial);
		ASSERT_FALSE(ec) << ec.message();
		std::cout << '\n';
		ec = add_report(repo, git_repo, current,
		                "9eebbdd12ec79e1cf43731d778d055b248375fda"sv,
		                current_files, &middle);
		ASSERT_FALSE(ec) << ec.message();

		auto main = io::fopen(
		    setup::test_dir() / "diffs/.git/.covdata/refs/heads/main"sv, "wb");
		main.store(fmt::format("{}\n", current).c_str(), 41);
	}
#endif

	char letter_for(file_diff::kind kind) {
		switch (kind) {
			case file_diff::normal:
				return 'N';
			case file_diff::renamed:
				return 'R';
			case file_diff::copied:
				return 'C';
			case file_diff::added:
				return 'A';
			case file_diff::deleted:
				return 'D';
		}
		return '?';
	}

	std::string format(unsigned value, int diff) {
		if (diff) return fmt::format("{} [{:+}]", value, diff);
		return fmt::format("{}", value);
	}

	std::string format(io::v1::stats::ratio<> value,
	                   io::v1::stats::ratio<int> diff) {
		if (diff.whole || diff.fraction)
			return fmt::format("{}.{:0{}}% [{:+}.{:0{}}%]", value.whole,
			                   value.fraction, value.digits, diff.whole,
			                   diff.fraction, diff.digits);
		return fmt::format("{}.{:0{}}%", value.whole, value.fraction,
		                   value.digits);
	}

	enum {
		id_letter,
		id_percent,
		id_covered,
		id_relevant,
		id_total,
		row_size,
	};

	void print(ref_ptr<cov::report> report,
	           std::vector<file_stats> const& files) {
		std::cout << "report " << report->oid() << '\n';
		if (!report->parent_id().is_zero())
			std::cout << "parent " << report->parent_id() << '\n';
		std::cout << "commit " << report->commit_id() << '\n';
		for (auto const& build : report->entries()) {
			std::cout << "build  " << build->build_id() << " '"
			          << build->props_json() << "'\n";
		}

		if (!files.empty()) std::cout << '\n';

		std::vector<std::vector<std::string>> rows{};
		rows.reserve(files.size());
		std::vector<size_t> sizes(row_size);
		for (auto const& file : files) {
			auto const diff = file.diff(2);
			std::vector<std::string> row(row_size + 2);
			row[id_letter] = fmt::format("{}", letter_for(file.diff_kind));
			row[id_percent] =
			    format(diff.coverage.current.lines, diff.coverage.diff.lines);
			row[id_covered] = format(diff.stats.current.lines.visited,
			                         diff.stats.diff.lines.visited);
			row[id_relevant] = format(diff.stats.current.lines.relevant,
			                          diff.stats.diff.lines.relevant);
			row[id_total] = format(diff.stats.current.lines_total,
			                       diff.stats.diff.lines_total);
			row[id_total + 1] = file.filename;

			if (file.diff_kind == file_diff::renamed ||
			    file.diff_kind == file_diff::copied) {
				row[id_total + 2] = file.previous_name;
			}

			for (size_t index = 0; index < row_size; ++index) {
				if (sizes[index] < row[index].size())
					sizes[index] = row[index].size();
			}

			rows.push_back(std::move(row));
		}

		auto const rename_offset = 15 + [&] {
			size_t length = 0;
			for (auto size : sizes)
				length += size;
			return length;
		}();
		for (auto const& row : rows) {
#define PRINT(ID) row[ID], sizes[ID]
			fmt::print("{} | {:>{}} | {:>{}} / {:>{}} | {:>{}} | {}\n",
			           row[id_letter], PRINT(id_percent), PRINT(id_covered),
			           PRINT(id_relevant), PRINT(id_total), row[id_total + 1]);
			if (!row[id_total + 2].empty())
				fmt::print("{:>{}}was: {}\n", " | "sv, rename_offset,
				           row[id_total + 2]);
		}
	}

	ref_ptr<cov::report> from_head(repository& repo, std::error_code& ec) {
		auto HEAD = as_a<cov::reference>(repo.dwim("HEAD"sv));
		if (!HEAD) return {};
		HEAD = HEAD->peel_target();
		if (!HEAD || HEAD->reference_type() != reference_type::direct)
			return {};
		return repo.lookup<cov::report>(*HEAD->direct_target(), ec);
	}

	ref_ptr<cov::report> from_parent(repository& repo,
	                                 ref_ptr<cov::report> const& report,
	                                 std::error_code& ec) {
		return repo.lookup<cov::report>(report->parent_id(), ec);
	}

	ref_ptr<cov::report> from_head(repository& repo,
	                               size_t depth,
	                               std::error_code& ec) {
		auto report = from_head(repo, ec);
		if (ec) return {};

		while (depth) {
			--depth;
			auto cand = from_parent(repo, report, ec);
			if (!ec && !cand) {
				ec = git::make_error_code(git::errc::notfound);
			}
			if (ec) return {};
			report = std::move(cand);
		}
		return report;
	}

	struct diff_test {
		std::string_view name;
		size_t depth_below_head;
		std::vector<file_stats> expected;
		struct {
			bool break_exactly{false};
			file_diff::initial initial{file_diff::initial_with_self};
		} tweaks{};

		friend std::ostream& operator<<(std::ostream& out,
		                                diff_test const& test) {
			return out << test.name;
		}
	};

	class diff : public ::testing::TestWithParam<diff_test> {};

	TEST_P(diff, report) {
		auto const& [_, depth_below_head, expected, tweaks] = GetParam();
		auto const [break_exactly, initial] = tweaks;

		git::init app{};
		std::error_code ec{};
		auto repo = cov::repository::open(
		    setup::test_dir() / "sysroot"sv,
		    setup::test_dir() / "diffs/.git/.covdata"sv, ec);
		ASSERT_FALSE(ec) << ec.message();

		auto report = from_head(repo, depth_below_head, ec);
		ASSERT_FALSE(ec);

		std::vector<file_stats> files;
		if (break_exactly) {
			git_diff_find_options opts{};
			git_diff_find_options_init(&opts, GIT_DIFF_FIND_OPTIONS_VERSION);
			opts.flags = GIT_DIFF_FIND_EXACT_MATCH_ONLY;
			files = repo.diff_with_parent(report, ec, &opts, initial);
		} else {
			files = repo.diff_with_parent(report, ec, nullptr, initial);
		}
		ASSERT_FALSE(ec);

		print(report, files);

		ASSERT_EQ(expected, files);
	}

	static const diff_test tests[] = {
	    {
	        "current"sv,
	        0,
	        {
	            {"src/greetings.cc",
	             {8, {3, 3}, {0, 0}, {0, 0}},
	             {6, {1, 0}, {0, 0}, {0, 0}},
	             "src/old-name.cc",
	             file_diff::renamed},
	            {"src/main.cc",
	             {5, {1, 1}, {0, 0}, {0, 0}},
	             {5, {1, 0}, {0, 0}, {0, 0}},
	             {},
	             file_diff::normal},
	        },
	    },
	    {
	        "current_break_exactly"sv,
	        0,
	        {
	            {"src/greetings.cc",
	             {8, {3, 3}, {0, 0}, {0, 0}},
	             {0, {0, 0}, {0, 0}, {0, 0}},
	             {},
	             file_diff::added},
	            {"src/main.cc",
	             {5, {1, 1}, {0, 0}, {0, 0}},
	             {5, {1, 0}, {0, 0}, {0, 0}},
	             {},
	             file_diff::normal},
	            {"src/old-name.cc",
	             {0, {0, 0}, {0, 0}, {0, 0}},
	             {6, {1, 0}, {0, 0}, {0, 0}},
	             {},
	             file_diff::deleted},
	        },
	        {.break_exactly = true},
	    },
	    {
	        "middle"sv,
	        1,
	        {
	            {"src/main.cc",
	             {5, {1, 0}, {0, 0}, {0, 0}},
	             {0, {0, 0}, {0, 0}, {0, 0}},
	             {},
	             file_diff::added},
	            {"src/old-name.cc",
	             {6, {1, 0}, {0, 0}, {0, 0}},
	             {5, {1, 0}, {0, 0}, {0, 0}},
	             "main.cc",
	             file_diff::renamed},
	        },
	    },
	    {
	        "bottom_to_self"sv,
	        2,
	        {
	            {"main.cc",
	             {5, {1, 0}, {0, 0}, {0, 0}},
	             {5, {1, 0}, {0, 0}, {0, 0}},
	             {},
	             file_diff::normal},
	        },
	    },
	    {
	        "bottom_to_empty"sv,
	        2,
	        {
	            {"main.cc",
	             {5, {1, 0}, {0, 0}, {0, 0}},
	             {0, {0, 0}, {0, 0}, {0, 0}},
	             {},
	             file_diff::added},
	        },
	        {.initial = file_diff::initial_add_all},
	    },
	};

	INSTANTIATE_TEST_SUITE_P(tests, diff, ::testing::ValuesIn(tests));
}  // namespace cov::testing
