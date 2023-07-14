// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/format.hh>
#include <cov/git2/global.hh>
#include <cov/repository.hh>
#include "../path-utils.hh"
#include "../print-view.hh"

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#define _isatty(FD) isatty(FD)
#define _fileno(OBJ) fileno(OBJ)
#endif

namespace cov::placeholder {
	std::ostream& operator<<(std::ostream& out, refs const& r) {
		out << '{';
		if (!r.HEAD.empty())
			testing::print_view(out << ".HEAD=", r.HEAD) << 's';
		bool first = true;
		if (!r.tags.empty()) {
			out << ",.tags={";
			for (auto const& [ref, hash] : r.tags) {
				if (first)
					first = false;
				else
					out << ',';
				testing::print_view(out << '{', ref) << 's';
				testing::print_view(out << ',', hash) << 's';
				out << '}';
			}
			out << '}';
		}
		first = true;
		if (!r.heads.empty()) {
			out << ",.heads={";
			for (auto const& [ref, hash] : r.heads) {
				if (first)
					first = false;
				else
					out << ',';
				testing::print_view(out << '{', ref) << 's';
				testing::print_view(out << ',', hash) << 's';
				out << '}';
			}
			out << '}';
		}
		if (!r.HEAD_ref.empty())
			testing::print_view(out << ",.HEAD_ref=", r.HEAD_ref) << 's';
		return out << '}';
	}

	std::ostream& operator<<(std::ostream& out, ratio const& r) {
		return out << '{' << r.num << ',' << r.den << '}';
	}

	std::ostream& operator<<(std::ostream& out, rating const& r) {
		return out << "{.incomplete=" << r.incomplete << ",.passing"
		           << r.passing << '}';
	}

	std::ostream& operator<<(std::ostream& out, context const& ctx) {
		out << "{.hash_length=" << ctx.hash_length << "u, .names=" << ctx.names
		    << ", .marks" << ctx.marks;
		if (!ctx.time_zone.empty())
			testing::print_view(out << ", .time_zone=", ctx.time_zone);
		if (!ctx.locale.empty())
			testing::print_view(out << ", .locale=", ctx.locale);
		if (ctx.app) out << ", .app=" << ctx.app;
		if (ctx.translate) out << ", .translate=" << ctx.translate;
		if (ctx.colorize) {
			if (ctx.colorize == formatter::shell_colorize)
				out << ", .colorize=formatter::shell_colorize";
			else
				out << ", .colorize=" << ctx.colorize;
		}
		if (ctx.decorate) out << ", .decorate=true";
		return out << '}';
	}
}  // namespace cov::placeholder

namespace cov::testing {
	using namespace ::std::literals;
	namespace ph = placeholder;

	namespace {
		bool is_terminal(FILE* out) noexcept {
#ifndef _WIN32
			char const* term = getenv("TERM");
#endif
			return (_isatty(_fileno(out)) != 0)
#ifndef _WIN32
			       && term && strcmp(term, "dumb") != 0
#endif
			    ;  // NOLINT
		}
	}  // namespace

	struct context_test {
		std::string_view name;
		std::string_view location;
		std::vector<path_info> steps;
		ph::context expected{};
		struct {
			color_feature clr{use_feature::yes};
			decorate_feature decorate{use_feature::yes};
		} tweaks{};

		friend std::ostream& operator<<(std::ostream& out,
		                                context_test const& param) {
			return out << param.name;
		}
	};
	class context : public ::testing::TestWithParam<context_test> {};

	TEST_P(context, create) {
		auto const& [_, location, steps, almost_expected, tweaks] = GetParam();

		git::init init{};
		{
			std::error_code ec{};
			path_info::op(steps, ec);
			ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
			                 << ec.category().name() << ')';
		}

		std::error_code ec{};
		auto const repo = cov::repository::open(
		    setup::test_dir(), setup::test_dir() / location, ec);
		ASSERT_FALSE(ec);

		auto const actual =
		    ph::context::from(repo, tweaks.clr, tweaks.decorate);

		auto expected = almost_expected;
		expected.now = actual.now;

		if (tweaks.clr == use_feature::automatic)
			expected.colorize =
			    is_terminal(stdout) ? formatter::shell_colorize : nullptr;
		if (tweaks.decorate == use_feature::automatic)
			expected.decorate = is_terminal(stdout);

		ASSERT_EQ(expected, actual);
	}

#define RATIO_CFG(DIRNAME, RATIO)                                              \
	make_setup(remove_all(DIRNAME ""sv),                                       \
	           create_directories(DIRNAME "/subdir"sv),                        \
	           create_directories(DIRNAME "/.git/objects/pack"sv),             \
	           create_directories(DIRNAME "/.git/objects/info"sv),             \
	           create_directories(DIRNAME "/.git/refs/tags"sv),                \
	           touch(DIRNAME "/.git/HEAD"sv), touch(DIRNAME "/.git/config"sv), \
	           touch(DIRNAME "/.git/refs/heads/main"sv),                       \
	           init_repo(DIRNAME "/.git/.covdata"sv, DIRNAME "/.git/"sv),      \
	           touch(DIRNAME "/.git/.covdata/config"sv,                        \
	                 "[core]\n  gitdir = ..\n  rating = " RATIO ""sv))

	static context_test const tests[] = {
	    {
	        "empty"sv,
	        "empty-repo/.git/.covdata"sv,
	        make_setup(
	            remove_all("empty-repo"sv),
	            create_directories("empty-repo/subdir"sv),
	            create_directories("empty-repo/.git/objects/pack"sv),
	            create_directories("empty-repo/.git/objects/info"sv),
	            create_directories("empty-repo/.git/refs/tags"sv),
	            touch("empty-repo/.git/HEAD"sv),
	            touch("empty-repo/.git/config"sv),
	            touch("empty-repo/.git/refs/heads/main"sv),
	            init_repo("empty-repo/.git/.covdata"sv, "empty-repo/.git/"sv)),
	        {.hash_length = 9u,
	         .names = {.HEAD = "main"s},
	         .marks{.incomplete = {3, 4}, .passing{9, 10}},
	         .colorize = formatter::shell_colorize,
	         .decorate = true},
	    },
	    {
	        "empty, no features"sv,
	        "no-feat/.git/.covdata"sv,
	        make_setup(remove_all("no-feat"sv),
	                   create_directories("no-feat/subdir"sv),
	                   create_directories("no-feat/.git/objects/pack"sv),
	                   create_directories("no-feat/.git/objects/info"sv),
	                   create_directories("no-feat/.git/refs/tags"sv),
	                   touch("no-feat/.git/HEAD"sv),
	                   touch("no-feat/.git/config"sv),
	                   touch("no-feat/.git/refs/heads/main"sv),
	                   init_repo("no-feat/.git/.covdata"sv, "no-feat/.git/"sv)),
	        {.hash_length = 9u,
	         .names = {.HEAD = "main"s},
	         .marks{.incomplete = {3, 4}, .passing{9, 10}}},
	        {.clr = use_feature::no, .decorate = use_feature::no},
	    },
	    {
	        "some names"sv,
	        "some-names/.git/.covdata"sv,
	        make_setup(
	            remove_all("some-names"sv),
	            create_directories("some-names/subdir"sv),
	            create_directories("some-names/.git/objects/pack"sv),
	            create_directories("some-names/.git/objects/info"sv),
	            create_directories("some-names/.git/refs/tags"sv),
	            touch("some-names/.git/HEAD"sv),
	            touch("some-names/.git/config"sv),
	            touch("some-names/.git/refs/heads/main"sv),
	            init_repo("some-names/.git/.covdata"sv, "some-names/.git/"sv),
	            touch("some-names/.git/.covdata/refs/heads/main"sv,
	                  "11223344556677889900aabbccddeeff12345678\n"sv),
	            touch("some-names/.git/.covdata/refs/tags/v1.0"sv,
	                  "34c845392a2c508e1a6d7755740485f24f4e19c9\n"sv)),
	        {.hash_length = 9u,
	         .names = {.HEAD = "main"s,
	                   .tags = {{"v1.0"s,
	                             "34c845392a2c508e1a6d7755740485f24f4e19c9"s}},
	                   .heads = {{"main"s,
	                              "11223344556677889900aabbccddeeff12345678"s}},
	                   .HEAD_ref = "11223344556677889900aabbccddeeff12345678"s},
	         .marks{.incomplete = {3, 4}, .passing{9, 10}}},
	        {.clr = use_feature::automatic, .decorate = use_feature::automatic},
	    },
	    {
	        "road to nowhere"sv,
	        "road-to-nowhere/.git/.covdata"sv,
	        make_setup(
	            remove_all("road-to-nowhere"sv),
	            create_directories("road-to-nowhere/subdir"sv),
	            create_directories("road-to-nowhere/.git/objects/pack"sv),
	            create_directories("road-to-nowhere/.git/objects/info"sv),
	            create_directories("road-to-nowhere/.git/refs/tags"sv),
	            touch("road-to-nowhere/.git/HEAD"sv),
	            touch("road-to-nowhere/.git/config"sv),
	            touch("road-to-nowhere/.git/refs/heads/main"sv),
	            init_repo("road-to-nowhere/.git/.covdata"sv,
	                      "road-to-nowhere/.git/"sv),
	            touch("road-to-nowhere/.git/.covdata/refs/heads/main"sv,
	                  "11223344556677889900aabbccddeeff12345678\n"sv),
	            touch("road-to-nowhere/.git/.covdata/refs/tags/v1.0"sv,
	                  "34c845392a2c508e1a6d7755740485f24f4e19c9\n"sv),
	            touch("road-to-nowhere/.git/.covdata/refs/tags/v1.0-rc.3"sv,
	                  "ref: refs/tags/v1.0\n"sv),
	            touch("road-to-nowhere/.git/.covdata/refs/tags/here"sv,
	                  "ref: refs/there\n"sv),
	            touch("road-to-nowhere/.git/.covdata/refs/there"sv,
	                  "ref: refs/tags/nowhere\n"sv)),
	        {.hash_length = 9u,
	         .names = {.HEAD = "main"s,
	                   .tags = {{"v1.0"s,
	                             "34c845392a2c508e1a6d7755740485f24f4e19c9"s},
	                            {"v1.0-rc.3"s,
	                             "34c845392a2c508e1a6d7755740485f24f4e19c9"s}},
	                   .heads = {{"main"s,
	                              "11223344556677889900aabbccddeeff12345678"s}},
	                   .HEAD_ref = "11223344556677889900aabbccddeeff12345678"s},
	         .marks{.incomplete = {3, 4}, .passing{9, 10}},
	         .colorize = formatter::shell_colorize,
	         .decorate = true},
	    },
	    {
	        "rating%"sv,
	        "ratings-A/.git/.covdata"sv,
	        RATIO_CFG("ratings-A", "88%, 66%"),
	        {.hash_length = 9u,
	         .names = {.HEAD = "main"s},
	         .marks{.incomplete = {33, 50}, .passing{22, 25}},
	         .colorize = formatter::shell_colorize,
	         .decorate = true},
	    },
	    {
	        "rating/ratio"sv,
	        "ratings-B/.git/.covdata"sv,
	        RATIO_CFG("ratings-B", "55 / 80, 2345/3060"),
	        {.hash_length = 9u,
	         .names = {.HEAD = "main"s},
	         .marks{.incomplete = {11, 16}, .passing{469, 612}},
	         .colorize = formatter::shell_colorize,
	         .decorate = true},
	    },
	    {
	        "rating/word"sv,
	        "ratings-C/.git/.covdata"sv,
	        RATIO_CFG("ratings-C", "word/3060, 55 / -=+=-"),
	        {.hash_length = 9u,
	         .names = {.HEAD = "main"s},
	         .marks{.incomplete = {3, 4}, .passing{9, 10}},
	         .colorize = formatter::shell_colorize,
	         .decorate = true},
	    },
	    {
	        "rating word%"sv,
	        "ratings-D/.git/.covdata"sv,
	        RATIO_CFG("ratings-D", "word%, 95%!"),
	        {.hash_length = 9u,
	         .names = {.HEAD = "main"s},
	         .marks{.incomplete = {3, 4}, .passing{9, 10}},
	         .colorize = formatter::shell_colorize,
	         .decorate = true},
	    },
	};

	INSTANTIATE_TEST_SUITE_P(tests, context, ::testing::ValuesIn(tests));
}  // namespace cov::testing
