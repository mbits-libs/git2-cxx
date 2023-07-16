// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cov/git2/bytes.hh>
#include <cov/io/build.hh>
#include <cov/io/db_object.hh>
#include <cov/io/read_stream.hh>
#include "setup.hh"

namespace cov::testing {
	namespace {
		using namespace std::literals;
		class memory final : public write_stream {
		public:
			std::vector<std::byte> data{};

			bool opened() const noexcept final { return true; }
			size_t write(git::bytes block) final {
				auto const size = data.size();
				data.insert(data.end(), block.begin(), block.end());
				return data.size() - size;
			}

			std::string_view view() const noexcept {
				return {reinterpret_cast<char const*>(data.data()),
				        data.size()};
			}
		};
	}  // namespace

	struct build_impl : counted_impl<cov::build> {
		struct {
			git::oid oid{};
			git::oid file_list_id{};
			sys_seconds add_time_utc{};
			std::string_view props_json{};
			io::v1::coverage_stats stats{};
		} info;

		git::oid const& oid() const noexcept override { return info.oid; }
		git::oid const& file_list_id() const noexcept override {
			return info.file_list_id;
		}
		sys_seconds add_time_utc() const noexcept override {
			return info.add_time_utc;
		}
		std::string_view props_json() const noexcept override {
			return info.props_json;
		}
		io::v1::coverage_stats const& stats() const noexcept override {
			return info.stats;
		}
	};

	TEST(build, load) {
		static constexpr auto s =
		    "bld \x00\x00\x01\x00"

		    // strings
		    "\x11\x00\x00\x00"
		    "\x01\x00\x00\x00"

		    // file list
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    // added
		    "\x33\x22\x11\x00\x77\x66\x55\x44"

		    // propset
		    "\x00\x00\x00\x00"

		    // stats
		    "\xE2\x04\x00\x00"
		    "\x2C\x01\x00\x00"
		    "\x2B\x01\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    // strings
		    "\x00\x00\x00\x00"
		    ""sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::BUILD, io::handlers::build>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
		                 << ec.category().name() << ')';
		ASSERT_TRUE(result);
		ASSERT_TRUE(result->is_object());
		auto const obj = static_cast<object const*>(result.get());
		ASSERT_EQ(obj_build, obj->type());
		auto const build = as_a<cov::build>(obj);
		ASSERT_TRUE(build);
		ASSERT_TRUE(build->file_list_id().is_zero());
	}

	TEST(build, partial_load_no_space) {
		static constexpr auto s =
		    "bld \x00\x00\x01\x00"

		    // strings
		    "\x12\x00\x00\x00"
		    "\x10\x00\x00\x00"

		    // file list
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    // added
		    "\x33\x22\x11\x00\x77\x66\x55\x44"

		    // propset
		    "\x00\x00\x00\x00"
		    ""sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::BUILD, io::handlers::build>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(result);
	}

	TEST(build, partial_load_strings_inside_header) {
		static constexpr auto s =
		    "bld \x00\x00\x01\x00"

		    // strings
		    "\x00\x00\x00\x00"
		    "\x10\x00\x00\x00"

		    // file list
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    // added
		    "\x33\x22\x11\x00\x77\x66\x55\x44"

		    // propset
		    "\x00\x00\x00\x00"

		    // stats
		    "\xE2\x04\x00\x00"
		    "\x2C\x01\x00\x00"
		    "\x2B\x01\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    ""sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::BUILD, io::handlers::build>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(result);
	}

	TEST(build, partial_load_no_space_for_strings) {
		static constexpr auto s =
		    "bld \x00\x00\x01\x00"

		    // strings
		    "\x12\x00\x00\x00"
		    "\x10\x00\x00\x00"

		    // file list
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    // added
		    "\x33\x22\x11\x00\x77\x66\x55\x44"

		    // propset
		    "\x00\x00\x00\x00"

		    // stats
		    "\xE2\x04\x00\x00"
		    "\x2C\x01\x00\x00"
		    "\x2B\x01\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    ""sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::BUILD, io::handlers::build>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(result);
	}

	TEST(build, partial_load_no_strings) {
		static constexpr auto s =
		    "bld \x00\x00\x01\x00"

		    // strings
		    "\x11\x00\x00\x00"
		    "\x10\x00\x00\x00"

		    // file list
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    // added
		    "\x33\x22\x11\x00\x77\x66\x55\x44"

		    // propset
		    "\x00\x00\x00\x00"

		    // stats
		    "\xE2\x04\x00\x00"
		    "\x2C\x01\x00\x00"
		    "\x2B\x01\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    // strings
		    "\x00\x00\x00\x00"
		    ""sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::BUILD, io::handlers::build>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(result);
	}

	TEST(build, partial_load_wrong_string) {
		static constexpr auto s =
		    "bld \x00\x00\x01\x00"

		    // strings
		    "\x11\x00\x00\x00"
		    "\x01\x00\x00\x00"

		    // file list
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    // added
		    "\x33\x22\x11\x00\x77\x66\x55\x44"

		    // propset
		    "\x10\x00\x00\x00"

		    // stats
		    "\xE2\x04\x00\x00"
		    "\x2C\x01\x00\x00"
		    "\x2B\x01\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    // strings
		    "\x00\x00\x00\x00"
		    ""sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::BUILD, io::handlers::build>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(result);
	}

	TEST(build, store) {
		static constexpr auto expected =
		    "bld \x00\x00\x01\x00"

		    // strings
		    "\x11\x00\x00\x00"
		    "\x01\x00\x00\x00"

		    // file list
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    // added
		    "\x33\x22\x11\x00\x77\x66\x55\x44"

		    // propset
		    "\x00\x00\x00\x00"

		    // stats
		    "\xE2\x04\x00\x00"
		    "\x2C\x01\x00\x00"
		    "\x2B\x01\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    // strings
		    "\x00\x00\x00\x00"
		    ""sv;
		memory stream{};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::BUILD, io::handlers::build>();

		auto const obj = make_ref<build_impl>();
		obj->info.add_time_utc = sys_seconds{0x11223344556677s};
		obj->info.stats = {
		    .lines_total = 1250,
		    .lines = {.relevant = 300, .visited = 299},
		    .functions = io::v1::stats::init(),
		    .branches = io::v1::stats::init(),
		};
		auto const result = dbo.store(obj, stream);
		ASSERT_TRUE(result);
		ASSERT_EQ(expected, stream.view());
	}
}  // namespace cov::testing
