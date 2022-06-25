// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/io/read_stream.hh>
#include <git2-c++/bytes.hh>
#include "setup.hh"

namespace cov::testing {
	namespace {
		using namespace ::std::literals;

		static constexpr unsigned char readme_z[] = {
		    0x78, 0x01, 0x4b, 0xca, 0xc9, 0x4f, 0x52, 0x30, 0x34, 0x63, 0x50,
		    0x56, 0x08, 0x49, 0x2d, 0x2e, 0xc9, 0xcc, 0x4b, 0x57, 0x28, 0x4a,
		    0x2d, 0xc8, 0x2f, 0xe6, 0x02, 0x00, 0x5b, 0x5a, 0x07, 0x9b};

		template <size_t Length>
		inline git::bytes wrap(unsigned char const (&data)[Length]) {
			return git::bytes{data, Length};
		}

		template <typename Stream>
		struct storage;
		template <>
		struct storage<io::direct_read_stream> {
			static io::direct_read_stream open(std::error_code& ec) {
				auto const filename =
				    setup::test_dir() / setup::make_path("file"sv);
				std::filesystem::remove(filename, ec);
				io::fopen(filename, "wb").store(readme_z, std::size(readme_z));
				return io::direct_read_stream{io::fopen(filename, "rb")};
			}
		};
		template <>
		struct storage<io::bytes_read_stream> {
			static io::bytes_read_stream open(std::error_code&) {
				return io::bytes_read_stream{
				    git::bytes{readme_z, std::size(readme_z)}};
			}
		};
	}  // namespace

	template <typename Stream>
	class read_stream : public ::testing::Test {};
	TYPED_TEST_SUITE_P(read_stream);

	TYPED_TEST_P(read_stream, io) {
		std::error_code ec{};
		TypeParam in = storage<TypeParam>::open(ec);
		ASSERT_FALSE(ec) << "What: " << ec.message();

		static constexpr auto pre_read = 4u;
		static constexpr auto skip = 7u;
		static constexpr auto read = 10u;

		ASSERT_TRUE(in);
		std::vector<unsigned char> buffer{};

		ASSERT_TRUE(in.load(buffer, pre_read));
		ASSERT_EQ((std::basic_string_view{readme_z, std::size(readme_z)}.substr(
		              0, pre_read)),
		          (std::basic_string_view{buffer.data(), buffer.size()}));

		ASSERT_TRUE(in.skip(skip));

		ASSERT_TRUE(in.load(buffer, read));
		ASSERT_EQ((std::basic_string_view{readme_z, std::size(readme_z)}.substr(
		              pre_read + skip, read)),
		          (std::basic_string_view{buffer.data(), buffer.size()}));
	}

	TYPED_TEST_P(read_stream, overshoot) {
		std::error_code ec{};
		TypeParam in = storage<TypeParam>::open(ec);
		ASSERT_FALSE(ec) << "What: " << ec.message();

		ASSERT_TRUE(in);
		std::vector<uint32_t> buffer{};

		ASSERT_FALSE(
		    in.load(buffer, std::size(readme_z) / sizeof(uint32_t) + 3));
		ASSERT_TRUE(buffer.empty());
	}

	TYPED_TEST_P(read_stream, uint32) {
		std::error_code ec{};
		TypeParam in = storage<TypeParam>::open(ec);
		ASSERT_FALSE(ec) << "What: " << ec.message();

		ASSERT_TRUE(in);
		uint32_t data{};

		ASSERT_TRUE(in.load(data));
		ASSERT_EQ(*reinterpret_cast<uint32_t const*>(readme_z), data);
	}

	REGISTER_TYPED_TEST_SUITE_P(read_stream, io, overshoot, uint32);

	using types =
	    ::testing::Types<io::direct_read_stream, io::bytes_read_stream>;
	INSTANTIATE_TYPED_TEST_SUITE_P(std, read_stream, types);

	struct header {
		uint32_t magick;
		char wizzard[1024];
	};

	static_assert(Simple<uint32_t>);
	static_assert(Simple<header>);
	static_assert(!Simple<std::string>);
}  // namespace cov::testing
