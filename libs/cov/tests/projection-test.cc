// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <git2/errors.h>
#include <gtest/gtest.h>
#include <cov/module.hh>
#include <cov/projection.hh>
#include <git2/error.hh>
#include <git2/global.hh>

using namespace std::literals;

namespace cov::projection {
	static void PrintTo(entry const& entry, std::ostream* os) {
		*os << "{entry_type";
		switch (entry.type) {
			case entry_type::module:
				*os << "::module";
				break;
			case entry_type::directory:
				*os << "::directory";
				break;
			case entry_type::standalone_file:
				*os << "::standalone_file";
				break;
			case entry_type::file:
				*os << "::file";
				break;
			default:
				*os << '{' << std::to_underlying(entry.type) << '}';
		}
		*os << ",\"" << entry.name.display << "\"sv,\"" << entry.name.expanded
		    << "\"sv,stats(" << entry.stats.current.lines_total << ','
		    << entry.stats.current.lines.relevant << ','
		    << entry.stats.current.lines.visited << ")}";
	}
}  // namespace cov::projection
namespace cov::projection::testing {
	inline std::string as_str(std::string_view view) {
		return {view.data(), view.size()};
	}

	static constexpr auto this_project = R"x([module]
    sep = " :: "
[module "git2 (c++)"]
    path = libs/git2-c++
[module "cov :: API"]
    path = libs/cov
[module "cov :: runtime"]
    path = libs/cov-rt
[module "app :: library"]
    path = libs/app
[module "app :: main"]
    path = apps/cov/
    path = apps/builtins/
[module "hilite"]
    path = libs/hilite/hilite
    path = libs/hilite/lighter
[module "hilite :: c++"]
    path = libs/hilite/hilite-cxx
[module "hilite :: python"]
    path = libs/hilite/hilite-py3
[module "hilite :: type script"]
    path = libs/hilite/hilite-ts
)x"sv;

	struct file_info {
		std::string_view filename{};
		io::v1::coverage_stats stats{coverage_stats::init()};
	};

	struct entry_info {
		entry_type type{};
		std::string_view label{};
		std::string_view expanded{};
		io::v1::coverage_stats stats{coverage_stats::init()};
	};

	consteval coverage_stats stats(uint32_t total,
	                               uint32_t relevant,
	                               uint32_t covered) {
		return {total, {relevant, covered}, {0, 0}, {0, 0}};
	}

	static constexpr file_info all_files[] = {
	    {"apps/builtins/builtin_init.cc"sv, stats(335, 201, 201)},
	    {"apps/builtins/builtin_config.cc"sv, stats(189, 189, 73)},
	    {"apps/builtins/builtin_module.cc"sv, stats(760, 478, 478)},
	    {"apps/cov/cov.cc"sv, stats(456, 340, 159)},
	    {"libs/app/include/cov/app/args.hh"sv, stats(249, 207, 0)},
	    {"libs/app/include/cov/app/tr.hh"sv, stats(276, 220, 220)},
	    {"libs/app/src/main.cc"sv, stats(209, 136, 136)},
	    {"libs/app/src/args.cc"sv, stats(238, 236, 30)},
	    {"libs/app/src/win32.cc"sv, stats(316, 300, 129)},
	    {"libs/app/src/tr.cc"sv, stats(513, 438, 205)},
	    {"libs/app/src/strings/cov.cc"sv, stats(248, 192, 46)},
	    {"libs/app/src/strings/args.cc"sv, stats(608, 365, 365)},
	    {"libs/app/src/strings/cov_init.cc"sv, stats(672, 568, 203)},
	    {"libs/app/src/strings/cov_config.cc"sv, stats(193, 145, 145)},
	    {"libs/app/src/strings/errors.cc"sv, stats(761, 639, 639)},
	    {"libs/app/src/strings/cov_module.cc"sv, stats(164, 126, 0)},
	    {"libs/cov-rt/include/cov/app/tools.hh"sv, stats(560, 362, 70)},
	    {"libs/cov-rt/include/cov/app/config.hh"sv, stats(312, 267, 267)},
	    {"libs/cov-rt/src/root_command.cc"sv, stats(543, 366, 366)},
	    {"libs/cov-rt/src/config.cc"sv, stats(449, 276, 43)},
	    {"libs/cov-rt/src/tools.cc"sv, stats(510, 302, 31)},
	    {"libs/cov-rt/src/split_command.cc"sv, stats(541, 361, 361)},
	    {"libs/cov/include/cov/branch.hh"sv, stats(742, 509, 183)},
	    {"libs/cov-rt/src/win32.cc"sv, stats(703, 547, 547)},
	    {"libs/cov/include/cov/db.hh"sv, stats(261, 147, 147)},
	    {"libs/cov/include/cov/counted.hh"sv, stats(274, 194, 194)},
	    {"libs/cov/include/cov/format.hh"sv, stats(284, 252, 0)},
	    {"libs/cov/include/cov/discover.hh"sv, stats(613, 599, 82)},
	    {"libs/cov/include/cov/object.hh"sv, stats(251, 203, 203)},
	    {"libs/cov/include/cov/init.hh"sv, stats(410, 382, 41)},
	    {"libs/cov/include/cov/reference.hh"sv, stats(169, 135, 0)},
	    {"libs/cov/include/cov/random.hh"sv, stats(224, 190, 190)},
	    {"libs/cov/include/cov/report.hh"sv, stats(551, 364, 83)},
	    {"libs/cov/include/cov/repository.hh"sv, stats(269, 232, 0)},
	    {"libs/cov/include/cov/streams.hh"sv, stats(296, 238, 238)},
	    {"libs/cov/include/cov/tag.hh"sv, stats(674, 569, 569)},
	    {"libs/cov/include/cov/zstream.hh"sv, stats(393, 350, 60)},
	    {"libs/cov/include/cov/hash/hash.hh"sv, stats(734, 604, 604)},
	    {"libs/cov/include/cov/hash/md5.hh"sv, stats(410, 246, 114)},
	    {"libs/cov/include/cov/hash/sha1.hh"sv, stats(740, 455, 0)},
	    {"libs/cov/include/cov/io/db_object.hh"sv, stats(660, 543, 543)},
	    {"libs/cov/include/cov/io/file.hh"sv, stats(659, 563, 563)},
	    {"libs/cov/include/cov/io/read_stream.hh"sv, stats(163, 98, 41)},
	    {"libs/cov/include/cov/io/safe_stream.hh"sv, stats(689, 480, 203)},
	    {"libs/cov/include/cov/io/strings.hh"sv, stats(508, 435, 115)},
	    {"libs/cov/include/cov/io/types.hh"sv, stats(598, 360, 360)},
	    {"libs/cov/src/branch-tag.hh"sv, stats(178, 109, 109)},
	    {"libs/cov/src/branch.cc"sv, stats(155, 151, 151)},
	    {"libs/cov/src/counted.cc"sv, stats(657, 515, 0)},
	    {"libs/cov/src/db.cc"sv, stats(498, 289, 289)},
	    {"libs/cov/src/discover.cc"sv, stats(722, 644, 241)},
	    {"libs/cov/src/format.cc"sv, stats(572, 454, 454)},
	    {"libs/cov/src/init.cc"sv, stats(516, 284, 0)},
	    {"libs/cov/src/path-utils.hh"sv, stats(344, 277, 130)},
	    {"libs/cov/src/repository.cc"sv, stats(556, 510, 54)},
	    {"libs/cov/src/streams.cc"sv, stats(728, 534, 534)},
	    {"libs/cov/src/tag.cc"sv, stats(164, 117, 117)},
	    {"libs/cov/src/zstream.cc"sv, stats(380, 374, 374)},
	    {"libs/cov/src/hash/md5.cc"sv, stats(742, 560, 182)},
	    {"libs/cov/src/hash/sha1.cc"sv, stats(688, 481, 481)},
	    {"libs/cov/src/io/db_object-error.cc"sv, stats(519, 517, 60)},
	    {"libs/cov/src/io/db_object.cc"sv, stats(628, 539, 216)},
	    {"libs/cov/src/io/file.cc"sv, stats(130, 123, 0)},
	    {"libs/cov/src/io/line_coverage.cc"sv, stats(173, 138, 138)},
	    {"libs/cov/src/io/read_stream.cc"sv, stats(686, 405, 405)},
	    {"libs/cov/src/io/report.cc"sv, stats(255, 233, 233)},
	    {"libs/cov/src/io/report_files.cc"sv, stats(561, 506, 98)},
	    {"libs/cov/src/io/safe_stream.cc"sv, stats(272, 208, 0)},
	    {"libs/cov/src/io/strings.cc"sv, stats(666, 491, 0)},
	    {"libs/cov/src/ref/reference.cc"sv, stats(161, 91, 91)},
	    {"libs/cov/src/ref/reference_list.cc"sv, stats(409, 277, 277)},
	    {"libs/cov/src/ref/references.cc"sv, stats(440, 271, 74)},
	    {"libs/git2-c++/include/git2/blob.hh"sv, stats(257, 212, 212)},
	    {"libs/git2-c++/include/git2/bytes.hh"sv, stats(683, 417, 75)},
	    {"libs/git2-c++/include/git2/commit.hh"sv, stats(571, 360, 360)},
	    {"libs/git2-c++/include/git2/config.hh"sv, stats(266, 162, 71)},
	    {"libs/git2-c++/include/git2/diff.hh"sv, stats(361, 313, 90)},
	    {"libs/git2-c++/include/git2/error.hh"sv, stats(240, 195, 195)},
	    {"libs/git2-c++/include/git2/global.hh"sv, stats(545, 484, 484)},
	    {"libs/git2-c++/include/git2/object.hh"sv, stats(137, 107, 50)},
	    {"libs/git2-c++/include/git2/odb.hh"sv, stats(721, 697, 697)},
	    {"libs/git2-c++/include/git2/ptr.hh"sv, stats(746, 500, 500)},
	    {"libs/git2-c++/include/git2/repository.hh"sv, stats(464, 428, 115)},
	    {"libs/git2-c++/include/git2/tree.hh"sv, stats(263, 212, 105)},
	    {"libs/git2-c++/src/blob.cc"sv, stats(667, 467, 226)},
	    {"libs/git2-c++/src/commit.cc"sv, stats(152, 128, 38)},
	    {"libs/git2-c++/src/config.cc"sv, stats(737, 673, 673)},
	    {"libs/git2-c++/src/error.cc"sv, stats(218, 141, 0)},
	    {"libs/git2-c++/src/odb.cc"sv, stats(704, 571, 0)},
	    {"libs/git2-c++/src/repository.cc"sv, stats(222, 147, 147)},
	    {"libs/git2-c++/src/tree.cc"sv, stats(322, 307, 43)},
	    {"libs/hilite/hilite-cxx/src/cxx.cc"sv, stats(178, 136, 136)},
	    {"libs/hilite/hilite/include/cell/ascii.hh"sv, stats(739, 415, 415)},
	    {"libs/hilite/hilite/include/cell/character.hh"sv, stats(373, 372, 0)},
	    {"libs/hilite/hilite/include/cell/context.hh"sv, stats(198, 142, 70)},
	    {"libs/hilite/hilite/include/cell/operators.hh"sv, stats(439, 369, 95)},
	    {"libs/hilite/hilite/include/cell/parser.hh"sv, stats(703, 588, 284)},
	    {"libs/hilite/hilite/include/cell/repeat_operators.hh"sv,
	     stats(249, 192, 40)},
	    {"libs/hilite/hilite/include/cell/special.hh"sv, stats(433, 366, 366)},
	    {"libs/hilite/hilite/include/cell/string.hh"sv, stats(691, 545, 545)},
	    {"libs/hilite/hilite/include/cell/tokens.hh"sv, stats(418, 357, 357)},
	    {"libs/hilite/hilite/include/hilite/hilite.hh"sv, stats(205, 118, 118)},
	    {"libs/hilite/hilite/include/hilite/hilite_impl.hh"sv,
	     stats(583, 463, 463)},
	    {"libs/hilite/hilite/src/hilite.cc"sv, stats(445, 338, 338)},
	    {"libs/hilite/hilite/src/none.cc"sv, stats(712, 515, 515)},
	    {"libs/hilite/lighter/src/lighter.cc"sv, stats(504, 377, 377)},
	    {"libs/hilite/lighter/src/hilite/lighter.hh"sv, stats(563, 398, 398)},
	};

	static constexpr file_info narrow[] = {
	    {"builtins/builtin_init.cc"sv, stats(335, 201, 201)},
	    {"builtins/builtin_config.cc"sv, stats(189, 189, 73)},
	    {"builtins/builtin_module.cc"sv, stats(760, 478, 478)},
	    {"cov.cc"sv, stats(456, 340, 159)},
	    {"main.cc"sv, stats(276, 220, 220)},
	};

	ref_ptr<cov::modules> from_memory(std::string_view modules,
	                                  std::error_code& ec) {
		auto const cfg = git::config::create();
		ec = cfg.add_memory(modules);
		if (ec) return {};

		return cov::modules::from_config(cfg, ec);
	}

	std::vector<file_stats> make_files(
	    std::span<file_info const> const& files) {
		std::vector<file_stats> result{};
		result.reserve(files.size());

		for (auto const& file : files) {
			result.push_back({.filename{as_str(file.filename)},
			                  .current{file.stats},
			                  .previous{file.stats}});
		}

		return result;
	}

	std::vector<entry> make_entries(
	    std::span<entry_info const> const& entries) {
		std::vector<entry> result{};
		result.reserve(entries.size());

		for (auto const& entry : entries) {
			result.push_back({
			    .type = entry.type,
			    .name{as_str(entry.label), as_str(entry.expanded)},
			    .stats{.current{entry.stats}, .previous{entry.stats}},
			});
		}

		return result;
	}

	struct message {
		std::error_code ec;

		friend std::ostream& operator<<(std::ostream& out, message const& msg) {
			if (msg.ec.category() == git::category()) {
				auto const error = git_error_last();
				if (error && error->message && *error->message) {
					return out << msg.ec.message() << ": " << error->message;
				}
			}
			return out << msg.ec << ": " << msg.ec.message();
		}
	};

	class projection : public ::testing::Test {
	public:
		void test(
		    std::string_view cov_module,
		    std::string_view module_filter,
		    std::string_view fname_filter,
		    std::span<entry_info const> const& expected_entries,
		    std::vector<file_stats> const& files = make_files(all_files)) {
			git::init memory{};
			std::error_code ec;

			auto const mods = from_memory(cov_module, ec);
			ASSERT_FALSE(ec) << message{ec};
			ASSERT_TRUE(mods);

			auto const filter = report_filter{mods.get(), as_str(module_filter),
			                                  as_str(fname_filter)};
			auto const actual = filter.project(files);
			for (auto const& entry : actual) {
				auto entry_flag = '?';
				switch (entry.type) {
					case entry_type::module:
						entry_flag = 'M';
						break;
					case entry_type::directory:
						entry_flag = 'D';
						break;
					case entry_type::standalone_file:
						entry_flag = 'F';
						break;
					case entry_type::file:
						entry_flag = '.';
						break;
				}
				auto const ratio =
				    fmt::format("{}", entry.stats.current.lines.calc(2));
				fmt::print("{:>6}% {} {} : {}\n", ratio, entry_flag,
				           entry.name.display, entry.name.expanded);
			}

			auto expected = make_entries(expected_entries);
			ASSERT_EQ(expected, actual);
		}
	};

	TEST_F(projection, default_projection) {
		static constinit entry_info const expected[] = {
		    {entry_type::directory, "apps"sv, "apps"sv, stats(1740, 1208, 911)},
		    {entry_type::directory, "libs"sv, "libs"sv,
		     stats(46146, 35511, 21642)},
		};
		test({}, {}, {}, expected);
	}
	TEST_F(projection, default_projection_with_modules) {
		static constinit entry_info const expected[] = {
		    {entry_type::module, "app"sv, "app"sv, stats(6187, 4780, 3029)},
		    {entry_type::module, "cov"sv, "cov"sv, stats(25990, 19727, 10926)},
		    {entry_type::module, "git2 (c++)"sv, "git2 (c++)"sv,
		     stats(8276, 6521, 4081)},
		    {entry_type::module, "hilite"sv, "hilite"sv,
		     stats(7433, 5691, 4517)},
		};
		test(this_project, {}, {}, expected);
	}
	TEST_F(projection, mod_filter) {
		static constinit entry_info const expected[] = {
		    {entry_type::module, "c++"sv, "hilite :: c++"sv,
		     stats(178, 136, 136)},
		    {entry_type::directory, "libs/hilite"sv, "libs/hilite"sv,
		     stats(7255, 5555, 4381)},
		};
		test(this_project, "hilite"sv, {}, expected);
	}
	TEST_F(projection, deep_mod_filter) {
		static constinit entry_info const expected[] = {
		    {entry_type::directory, "libs/hilite/hilite-cxx/src"sv,
		     "libs/hilite/hilite-cxx/src"sv, stats(178, 136, 136)},
		};
		test(this_project, "hilite :: c++"sv, {}, expected);
	}
	TEST_F(projection, dir_filter) {
		static constinit entry_info const expected[] = {
		    {entry_type::directory, "hilite"sv, "libs/hilite/hilite"sv,
		     stats(6188, 4780, 3606)},
		    {entry_type::directory, "hilite-cxx/src"sv,
		     "libs/hilite/hilite-cxx/src"sv, stats(178, 136, 136)},
		    {entry_type::directory, "lighter/src"sv,
		     "libs/hilite/lighter/src"sv, stats(1067, 775, 775)},
		};
		test(this_project, {}, "libs/hilite"sv, expected);
	}
	TEST_F(projection, standalone_file) {
		static constinit entry_info const expected[] = {
		    {entry_type::standalone_file, "cxx.cc"sv,
		     "libs/hilite/hilite-cxx/src/cxx.cc"sv, stats(178, 136, 136)},
		};
		test(this_project, {}, "libs/hilite/hilite-cxx/src/cxx.cc"sv, expected);
	}
	TEST_F(projection, no_modules_dir_filter_with_slash) {
		static constinit entry_info const expected[] = {
		    {entry_type::directory, "builtins"sv, "apps/builtins"sv,
		     stats(1284, 868, 752)},
		    {entry_type::directory, "cov"sv, "apps/cov"sv,
		     stats(456, 340, 159)},
		};
		test({}, {}, "apps/"sv, expected);
	}
	TEST_F(projection, not_found) { test({}, {}, "src/"sv, {}); }
	TEST_F(projection, standalone_in_root) {
		static constinit entry_info const expected[] = {
		    {entry_type::standalone_file, "main.cc"sv, "main.cc"sv,
		     stats(276, 220, 220)},
		};
		test({}, {}, "main.cc"sv, expected, make_files(narrow));
	}
	TEST_F(projection, no_modules_no_filters) {
		static constinit entry_info const expected[] = {
		    {entry_type::directory, "builtins"sv, "builtins"sv,
		     stats(1284, 868, 752)},
		    {entry_type::file, "cov.cc"sv, "cov.cc"sv, stats(456, 340, 159)},
		    {entry_type::file, "main.cc"sv, "main.cc"sv, stats(276, 220, 220)},
		};
		test({}, {}, {}, expected, make_files(narrow));
	}
}  // namespace cov::projection::testing
