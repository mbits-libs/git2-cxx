// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cov/git2/bytes.hh>
#include <cov/io/db_object.hh>
#include <cov/io/line_coverage.hh>
#include <cov/io/read_stream.hh>
#include "setup.hh"

namespace cov::testing {
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
			return {reinterpret_cast<char const*>(data.data()), data.size()};
		}
	};

	struct line_coverage_impl : counted_impl<cov::line_coverage> {
		std::vector<io::v1::coverage> lines{};

		std::span<io::v1::coverage const> coverage() const noexcept override {
			return lines;
		}
	};

	TEST(line_coverage, load) {
		static constexpr auto s =
		    "lnes\x00\x00\x01\x00"
		    "\x00\x00\x00\x00"sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::COVERAGE, io::handlers::line_coverage>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
		                 << ec.category().name() << ')';
		ASSERT_TRUE(result);
		ASSERT_TRUE(result->is_object());
		auto const obj = static_cast<object const*>(result.get());
		ASSERT_EQ(obj_line_coverage, obj->type());
		auto const lines = as_a<cov::line_coverage>(obj);
		ASSERT_TRUE(lines);
		ASSERT_TRUE(lines->coverage().empty());
	}

	TEST(line_coverage, load_1) {
		static constexpr auto s =
		    "lnes\x00\x00\x01\x00"
		    "\x01\x00\x00\x00"
		    "\x20\x00\x00\x80"sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::COVERAGE, io::handlers::line_coverage>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
		                 << ec.category().name() << ')';

		ASSERT_TRUE(result);
		ASSERT_TRUE(result->is_object());
		auto const obj = static_cast<object const*>(result.get());

		ASSERT_EQ(obj_line_coverage, obj->type());
		auto const lines = as_a<cov::line_coverage>(obj);
		ASSERT_TRUE(lines);
		ASSERT_EQ(1u, lines->coverage().size());

		auto const& front = lines->coverage().front();
		ASSERT_TRUE(front.is_null);
		ASSERT_EQ(32u, front.value);
	}

	TEST(line_coverage, load_partial) {
		static constexpr auto s =
		    "lnes\x00\x00\x01\x00"
		    "\x02\x00\x00\x00"
		    "\x20\x00\x00\x80"sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::COVERAGE, io::handlers::line_coverage>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_FALSE(result);
		ASSERT_EQ(ec, io::errc::bad_syntax);
	}

	TEST(line_coverage, store) {
		static constexpr auto expected =
		    "lnes\x00\x00\x01\x00"
		    "\x00\x00\x00\x00"sv;
		memory stream{};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::COVERAGE, io::handlers::line_coverage>();

		auto const obj = make_ref<line_coverage_impl>();
		auto const result = dbo.store(obj, stream);
		ASSERT_TRUE(result);
		ASSERT_EQ(expected, stream.view());
	}

	TEST(line_coverage, store_1) {
		static constexpr auto expected =
		    "lnes\x00\x00\x01\x00"
		    "\x01\x00\x00\x00"
		    "\x20\x00\x00\x80"sv;
		memory stream{};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::COVERAGE, io::handlers::line_coverage>();

		auto const obj = make_ref<line_coverage_impl>();
		obj->lines.push_back({.value = 32, .is_null = 1});
		auto const result = dbo.store(obj, stream);
		ASSERT_TRUE(result);
		ASSERT_EQ(expected, stream.view());
	}
}  // namespace cov::testing
