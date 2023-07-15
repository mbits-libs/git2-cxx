// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <git2/oid.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cov/git2/bytes.hh>
#include <cov/io/db_object.hh>
#include <cov/io/read_stream.hh>
#include <cov/io/report.hh>
#include <ranges>
#include "setup.hh"

namespace cov::testing {
	using namespace std::literals;
	namespace {
		static constexpr auto tested_text =
		    "rprt\x00\x00\x01\x00"

		    // strings
		    "\x25\x00\x00\x00"
		    "\x17\x00\x00\x00"

		    // parent report
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    // file list
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    // builds ref
		    "\x3C\x00\x00\x00"
		    "\x0D\x00\x00\x00"
		    "\x02\x00\x00\x00"

		    // added
		    "\x33\x22\x11\x00\x77\x66\x55\x44"
		    // - branch (str)
		    "\x3E\x00\x00\x00"
		    // - author (report_email)
		    "\x2D\x00\x00\x00"
		    "\x46\x00\x00\x00"
		    // - committer (report_email)
		    "\x2D\x00\x00\x00"
		    "\x46\x00\x00\x00"
		    // - message (str)
		    "\x1E\x00\x00\x00"
		    // - commit_id (oid)
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    // - committed (timestamp)
		    "\x33\x22\x11\x00\x77\x66\x55\x44"

		    // stats
		    "\xE2\x04\x00\x00"
		    "\x2C\x01\x00\x00"
		    "\x2B\x01\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    // strings
		    // 3456789'123456789'123456789
		    "\"prop\":\"set#1\"\x00"    //  0, 15 :: 0x00
		    "\"prop\":\"set#2\"\x00"    // 15, 15 :: 0x0F
		    "Initial commit\x00"        // 30, 15 :: 0x1E
		    "Johnny Appleseed\x00"      // 45, 17 :: 0x2D
		    "develop\x00"               // 62,  8 :: 0x3E
		    "johnny@appleseed.com\x00"  // 70, 21 :: 0x46
		    "\x00"

		    // prop=set#1
		    // - build_id (oid)
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    // - propset
		    "\x00\x00\x00\x00"
		    // - stats
		    "\xE2\x04\x00\x00"
		    "\x2C\x01\x00\x00"
		    "\x2B\x01\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    // prop=set#2
		    // - build_id (oid)
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    // - propset
		    "\x0F\x00\x00\x00"
		    // - stats
		    "\xE2\x04\x00\x00"
		    "\x2C\x01\x00\x00"
		    "\x2B\x01\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    ""sv;
	}  // namespace

	class test_stream final : public write_stream {
	public:
		static constexpr size_t infinite = std::numeric_limits<size_t>::max();
		std::vector<std::byte> data{};
		size_t free_size{infinite};

		bool opened() const noexcept final { return true; }
		size_t write(git::bytes block) final {
			if (free_size != infinite) {
				auto chunk = block.size();
				if (chunk > free_size) chunk = free_size;
				free_size -= chunk;
				block = block.subview(0, chunk);
			}
			auto const size = data.size();
			data.insert(data.end(), block.begin(), block.end());
			return data.size() - size;
		}

		std::string_view view() const noexcept {
			return {reinterpret_cast<char const*>(data.data()), data.size()};
		}
	};

	struct report_impl : counted_impl<cov::report> {
		git::oid const& oid() const noexcept override { return state.oid; }
		git::oid const& parent_id() const noexcept override {
			return state.parent_id;
		}
		git::oid const& file_list_id() const noexcept override {
			return state.file_list;
		}
		git::oid const& commit_id() const noexcept override {
			return state.commit;
		}
		std::string_view branch() const noexcept override {
			return state.branch;
		}
		std::string_view author_name() const noexcept override {
			return state.author_name;
		}
		std::string_view author_email() const noexcept override {
			return state.author_email;
		}
		std::string_view committer_name() const noexcept override {
			return state.committer_name;
		}
		std::string_view committer_email() const noexcept override {
			return state.committer_email;
		}
		std::string_view message() const noexcept override {
			return state.message;
		}
		sys_seconds commit_time_utc() const noexcept override {
			return state.commit_time_utc;
		}
		sys_seconds add_time_utc() const noexcept override {
			return state.add_time_utc;
		}
		io::v1::coverage_stats const& stats() const noexcept override {
			return state.stats;
		}
		std::span<std::unique_ptr<cov::report::build> const> entries()
		    const noexcept override {
			return state.entries;
		}

		struct state_type {
			git::oid oid;
			git::oid parent_id;
			git::oid file_list;
			git::oid commit;
			std::string branch;
			std::string author_name;
			std::string author_email;
			std::string committer_name;
			std::string committer_email;
			std::string message;
			sys_seconds commit_time_utc;
			sys_seconds add_time_utc;
			io::v1::coverage_stats stats;
			std::vector<std::unique_ptr<cov::report::build>> entries;
		} state;
	};

	TEST(report, load) {
		io::bytes_read_stream stream{
		    git::bytes{tested_text.data(), tested_text.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::REPORT, io::handlers::report>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
		                 << ec.category().name() << ')';
		ASSERT_TRUE(result);
		ASSERT_TRUE(result->is_object());
		auto const obj = static_cast<object const*>(result.get());
		ASSERT_EQ(obj_report, obj->type());
		auto const rprt = as_a<cov::report>(obj);
		ASSERT_TRUE(rprt);

		ASSERT_EQ("develop"sv, rprt->branch());
		ASSERT_EQ("Johnny Appleseed"sv, rprt->author_name());
		ASSERT_EQ("johnny@appleseed.com"sv, rprt->author_email());
		ASSERT_EQ("Johnny Appleseed"sv, rprt->committer_name());
		ASSERT_EQ("johnny@appleseed.com"sv, rprt->committer_email());
		ASSERT_EQ("Initial commit"sv, rprt->message());
		ASSERT_EQ(sys_seconds{0x11223344556677s}, rprt->commit_time_utc());
		ASSERT_EQ(sys_seconds{0x11223344556677s}, rprt->add_time_utc());
		ASSERT_EQ(1250u, rprt->stats().lines_total);
		ASSERT_EQ(300u, rprt->stats().lines.relevant);
		ASSERT_EQ(299u, rprt->stats().lines.visited);
		ASSERT_TRUE(rprt->parent_id().is_zero());
		ASSERT_TRUE(rprt->file_list_id().is_zero());
		ASSERT_TRUE(rprt->commit_id().is_zero());

		ASSERT_EQ(2, rprt->entries().size());

		static constexpr std::string_view props[] = {
		    "\"prop\":\"set#1\""sv,
		    "\"prop\":\"set#2\""sv,
		};

		auto prop = props;

		for (auto const& entry : rprt->entries()) {
			auto copy = *prop++;
			ASSERT_TRUE(entry->build_id().is_zero());
			ASSERT_EQ(copy, entry->props_json());
			ASSERT_EQ(1250u, entry->stats().lines_total);
			ASSERT_EQ(300u, entry->stats().lines.relevant);
			ASSERT_EQ(299u, entry->stats().lines.visited);
		}
	}

	TEST(report, load_partial) {
		auto const data = tested_text.substr(
		    0, sizeof(io::v1::report) + sizeof(io::file_header));
		io::bytes_read_stream stream{git::bytes{data.data(), data.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::REPORT, io::handlers::report>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(result);
	}

	TEST(report, load_partial_no_strings) {
		auto const data =
		    "rprt\x00\x00\x01\x00"

		    // strings
		    "\x25\x00\x00\x00"
		    "\x10\x00\x00\x00"

		    // parent report
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    // file list
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    // builds ref
		    "\x35\x00\x00\x00"
		    "\x0D\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    // added
		    "\x33\x22\x11\x00\x77\x66\x55\x44"
		    // - branch (str)
		    "\x20\x00\x00\x00"
		    // - author (report_email)
		    "\x0F\x00\x00\x00"
		    "\x28\x00\x00\x00"
		    // - committer (report_email)
		    "\x0F\x00\x00\x00"
		    "\x28\x00\x00\x00"
		    // - message (str)
		    "\x00\x00\x00\x00"
		    // - commit_id (oid)
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    // - committed (timestamp)
		    "\x33\x22\x11\x00\x77\x66\x55\x44"

		    // stats
		    "\xE2\x04\x00\x00"
		    "\x2C\x01\x00\x00"
		    "\x2B\x01\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    ""sv;
		io::bytes_read_stream stream{git::bytes{data.data(), data.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::REPORT, io::handlers::report>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(result);
	}

	TEST(report, load_partial_no_builds) {
		auto const data =
		    "rprt\x00\x00\x01\x00"

		    // strings
		    "\x25\x00\x00\x00"
		    "\x18\x00\x00\x00"

		    // parent report
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    // file list
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    // builds ref
		    "\x3D\x00\x00\x00"
		    "\x0D\x00\x00\x00"
		    "\x02\x00\x00\x00"

		    // added
		    "\x33\x22\x11\x00\x77\x66\x55\x44"
		    // - branch (str)
		    "\x20\x00\x00\x00"
		    // - author (report_email)
		    "\x0F\x00\x00\x00"
		    "\x28\x00\x00\x00"
		    // - committer (report_email)
		    "\x0F\x00\x00\x00"
		    "\x28\x00\x00\x00"
		    // - message (str)
		    "\x00\x00\x00\x00"
		    // - commit_id (oid)
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    // - committed (timestamp)
		    "\x33\x22\x11\x00\x77\x66\x55\x44"

		    // stats
		    "\xE2\x04\x00\x00"
		    "\x2C\x01\x00\x00"
		    "\x2B\x01\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    // strings
		    // 3456789'123456789'123456789
		    "Initial commit\x00"        //  0, 15 :: 0x00
		    "Johnny Appleseed\x00"      // 15, 17 :: 0x0F
		    "develop\x00"               // 32,  8 :: 0x20
		    "johnny@appleseed.com\x00"  // 40, 21 :: 0x28
		    "{\"prop\":\"set#1\"}\x00"  // 61, 17 :: 0x3D
		    "{\"prop\":\"set#2\"}\x00"  // 78, 17 :: 0x4E
		    "\x00"
		    ""sv;
		io::bytes_read_stream stream{git::bytes{data.data(), data.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::REPORT, io::handlers::report>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(result);
	}

	TEST(report, store) {
		test_stream stream{};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::REPORT, io::handlers::report>();

		auto const obj = make_ref<report_impl>();
		obj->state = {
		    .oid = {},
		    .parent_id = {},
		    .file_list = {},
		    .commit = {},
		    .branch = "develop"s,
		    .author_name = "Johnny Appleseed"s,
		    .author_email = "johnny@appleseed.com"s,
		    .committer_name = "Johnny Appleseed"s,
		    .committer_email = "johnny@appleseed.com"s,
		    .message = "Initial commit"s,
		    .commit_time_utc = sys_seconds{0x11223344556677s},
		    .add_time_utc = sys_seconds{0x11223344556677s},
		    .stats =
		        {
		            .lines_total = 1250,
		            .lines = {.relevant = 300, .visited = 299},
		            .functions = io::v1::stats::init(),
		            .branches = io::v1::stats::init(),
		        },
		    .entries =
		        cov::report::builder{}
		            .add_nfo({
		                .build_id{},
		                .props = "\"prop\":\"set#1\"",
		                .stats =
		                    {
		                        .lines_total = 1250,
		                        .lines = {.relevant = 300, .visited = 299},
		                        .functions = io::v1::stats::init(),
		                        .branches = io::v1::stats::init(),
		                    },
		            })
		            .add_nfo({
		                .build_id{},
		                .props = "\"prop\":\"set#2\"",
		                .stats =
		                    {
		                        .lines_total = 1250,
		                        .lines = {.relevant = 300, .visited = 299},
		                        .functions = io::v1::stats::init(),
		                        .branches = io::v1::stats::init(),
		                    },
		            })
		            .release()};
		auto const result = dbo.store(obj, stream);
		ASSERT_TRUE(result);
		ASSERT_EQ(tested_text, stream.view());
	}

	TEST(report, stats_calc) {
		static constexpr io::v1::coverage_stats stats{
		    .lines_total = 1'000'000,
		    .lines = {.relevant = 1'000'000, .visited = 657'823},
		};

		static constexpr std::tuple<unsigned char, io::v1::stats::ratio<>>
		    tests[] = {
		        {0u, {66u, 0u, 0u}},        {1u, {65u, 8u, 1u}},
		        {2u, {65u, 78u, 2u}},       {3u, {65u, 782u, 3u}},
		        {4u, {65u, 7823u, 4u}},     {5u, {65u, 78230u, 5u}},
		        {6u, {65u, 782300u, 6u}},   {7u, {65u, 7823000u, 7u}},
		        {8u, {65u, 78230000u, 8u}}, {9u, {65u, 782300000u, 9u}},
		    };

		for (auto const& [digits, expected] : tests) {
			auto const actual = stats.lines.calc(digits);
			EXPECT_EQ(expected, actual)
			    << "Digits: " << static_cast<unsigned>(digits);
		}
	}
}  // namespace cov::testing
