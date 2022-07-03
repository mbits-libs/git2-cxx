// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <git2/oid.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cov/io/db_object.hh>
#include <cov/io/read_stream.hh>
#include <cov/io/report.hh>
#include <git2/bytes.hh>
#include "setup.hh"

namespace cov::testing {
	using namespace std::literals;
	namespace {
		static constexpr auto tested_text =
		    "rprt\x00\x00\x01\x00"

		    // parent report
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    // file list
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    // added
		    "\x33\x22\x11\x00\x77\x66\x55\x44"
		    // stats
		    "\xE2\x04\x00\x00"
		    "\x2C\x01\x00\x00"
		    "\x2B\x01\x00\x00"
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
		    // string offset/size
		    "\x00\x00\x00\x00"
		    "\x10\x00\x00\x00"

		    // strings
		    // 3456789'123456789'123456789
		    "Initial commit\x00"        //  0, 15 :: 0x00
		    "Johnny Appleseed\x00"      // 15, 17 :: 0x0F
		    "develop\x00"               // 32,  8 :: 0x20
		    "johnny@appleseed.com\x00"  // 40,  - :: 0x28
		    "\x00\x00\x00"sv;
	}
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
		git_oid const* parent_report() const noexcept override {
			return &state.parent_report;
		}
		git_oid const* file_list() const noexcept override {
			return &state.file_list;
		}
		git_oid const* commit() const noexcept override {
			return &state.commit;
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

		struct state_type {
			git_oid parent_report;
			git_oid file_list;
			git_oid commit;
			std::string branch;
			std::string author_name;
			std::string author_email;
			std::string committer_name;
			std::string committer_email;
			std::string message;
			sys_seconds commit_time_utc;
			sys_seconds add_time_utc;
			io::v1::coverage_stats stats;
		} state;
	};

	TEST(report, load) {
		io::bytes_read_stream stream{
		    git::bytes{tested_text.data(), tested_text.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::REPORT, io::handlers::report>();

		std::error_code ec{};
		auto const result = dbo.load(stream, ec);
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
		ASSERT_EQ(1250u, rprt->stats().total);
		ASSERT_EQ(300u, rprt->stats().relevant);
		ASSERT_EQ(299u, rprt->stats().covered);
		ASSERT_TRUE(git_oid_is_zero(rprt->parent_report()));
		ASSERT_TRUE(git_oid_is_zero(rprt->file_list()));
		ASSERT_TRUE(git_oid_is_zero(rprt->commit()));
	}

	TEST(report, load_partial) {
		auto const data = tested_text.substr(
		    0, sizeof(io::v1::report) + sizeof(io::file_header));
		io::bytes_read_stream stream{git::bytes{data.data(), data.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::REPORT, io::handlers::report>();

		std::error_code ec{};
		auto const result = dbo.load(stream, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(result);
	}

	TEST(report, load_partial_no_strings) {
		auto const data =
		    "rprt\x00\x00\x01\x00"

		    // parent report
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    // file list
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    // added
		    "\x33\x22\x11\x00\x77\x66\x55\x44"
		    // stats
		    "\xE2\x04\x00\x00"
		    "\x2C\x01\x00\x00"
		    "\x2B\x01\x00\x00"
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
		    // string offset/size
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"sv;
		io::bytes_read_stream stream{git::bytes{data.data(), data.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::REPORT, io::handlers::report>();

		std::error_code ec{};
		auto const result = dbo.load(stream, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(result);
	}

	TEST(report, store) {
		test_stream stream{};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::REPORT, io::handlers::report>();

		auto const obj = make_ref<report_impl>();
		obj->state = {
		    .parent_report = {},
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
		            .total = 1250,
		            .relevant = 300,
		            .covered = 299,
		        },
		};
		auto const result = dbo.store(obj, stream);
		ASSERT_TRUE(result);
		ASSERT_EQ(tested_text, stream.view());
	}
}  // namespace cov::testing
