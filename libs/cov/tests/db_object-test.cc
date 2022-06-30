// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cov/io/db_object.hh>
#include <cov/io/read_stream.hh>
#include <git2/bytes.hh>
#include "setup.hh"

namespace cov::testing {
	using namespace std::literals;
	using ::testing::_;
	using ::testing::ByMove;
	using ::testing::Return;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;

	class errc : public TestWithParam<io::errc> {};

	TEST(errc, cat_name) {
		auto const actual = std::string_view{io::category().name()};
		ASSERT_FALSE(actual.empty());
	}

	TEST_P(errc, name) {
		auto value = GetParam();
		auto const actual = make_error_code(value).message();
		ASSERT_FALSE(actual.empty());
	}

	TEST_P(errc, direct_name) {
		auto value = GetParam();
		auto const actual = io::category().message(static_cast<int>(value));
		ASSERT_FALSE(actual.empty());
	}

	static constexpr io::errc values[] = {
	    static_cast<io::errc>(std::numeric_limits<int>::max()),
	    io::errc::bad_syntax,
	    io::errc::unknown_magic,
	    io::errc::unsupported_version,
	};
	INSTANTIATE_TEST_SUITE_P(values, errc, ValuesIn(values));

	class mock_handler : public io::db_handler {
	public:
		MOCK_METHOD(ref<counted>,
		            load,
		            (uint32_t magic,
		             uint32_t version,
		             read_stream& in,
		             std::error_code& ec),
		            (const, override));
		MOCK_METHOD(bool, recognized, (ref<counted> const& obj), (const));
		MOCK_METHOD(bool,
		            store,
		            (ref<counted> const& obj, write_stream& in),
		            (const, override));
	};

	class mock_counted : public counted_impl<> {};

	TEST(dbo, partial_header) {
		static constexpr auto s = "abcd\x00\x01\x02"sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		std::error_code ec{};
		auto const result = dbo.load(stream, ec);
		ASSERT_FALSE(result);
		ASSERT_EQ(ec, io::errc::bad_syntax);
	}

	TEST(dbo, wrong_version) {
		static constexpr auto s = "abcd\x00\x00\x02\x00"sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		std::error_code ec{};
		auto const result = dbo.load(stream, ec);
		ASSERT_FALSE(result);
		ASSERT_EQ(ec, io::errc::unsupported_version);
	}

	TEST(dbo, v1) {
		static constexpr auto s = "abcd\x00\x00\x01\x00"sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		std::error_code ec{};
		auto const result = dbo.load(stream, ec);
		ASSERT_FALSE(result);
		ASSERT_EQ(ec, io::errc::unknown_magic);
	}

	TEST(dbo, v1_1) {
		static constexpr auto s = "abcd\x01\x00\x01\x00"sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		std::error_code ec{};
		auto const result = dbo.load(stream, ec);
		ASSERT_FALSE(result);
		ASSERT_EQ(ec, io::errc::unknown_magic);
	}

	TEST(dbo, register_handler) {
		static constexpr auto s = "abcd\x00\x00\x01\x00"sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};

		auto handler = std::make_unique<mock_handler>();
		EXPECT_CALL(*handler, load("abcd"_tag, io::v1::VERSION, _, _));
		dbo.add_handler("abcd"_tag, std::move(handler));

		std::error_code ec{};
		auto const result = dbo.load(stream, ec);
		dbo.remove_handler("abcd"_tag);
		ASSERT_FALSE(result);
		ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
		                 << ec.category().name() << ')';
	}

	TEST(dbo, register_null) {
		static constexpr auto s = "abcd\x00\x00\x01\x00"sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler("abcd"_tag, {});

		std::error_code ec{};
		auto const result = dbo.load(stream, ec);
		ASSERT_FALSE(result);
		ASSERT_EQ(ec, io::errc::unknown_magic);
	}

	TEST(dbo, pass_created_object) {
		static constexpr auto s = "abcd\x00\x00\x01\x00"sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};

		auto obj = make_ref<mock_counted>();
		auto raw = obj.get();

		auto handler = std::make_unique<mock_handler>();
		EXPECT_CALL(*handler, load("abcd"_tag, io::v1::VERSION, _, _))
		    .WillRepeatedly(Return(ByMove(std::move(obj))));

		dbo.add_handler("abcd"_tag, std::move(handler));

		std::error_code ec{};
		auto const result = dbo.load(stream, ec);
		ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
		                 << ec.category().name() << ')';
		ASSERT_EQ(raw, result.get());
		ASSERT_FALSE(result->is_object());
		ASSERT_EQ(result, result.duplicate());
	}

	TEST(dbo, pass_created_object_OBJECT) {
		static constexpr auto s = "stxs\x00\x00\x01\x00"sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};

		auto obj = make_ref<mock_counted>();
		auto raw = obj.get();

		auto handler = std::make_unique<mock_handler>();
		EXPECT_CALL(*handler, load("stxs"_tag, io::v1::VERSION, _, _))
		    .WillRepeatedly(Return(ByMove(std::move(obj))));

		dbo.add_handler(io::OBJECT::HILITES, std::move(handler));

		std::error_code ec{};
		auto const result = dbo.load(stream, ec);
		ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
		                 << ec.category().name() << ')';
		ASSERT_EQ(raw, result.get());
		ASSERT_FALSE(result->is_object());
		ASSERT_EQ(result, result.duplicate());
	}
}  // namespace cov::testing
