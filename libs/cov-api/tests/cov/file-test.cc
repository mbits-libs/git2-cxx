// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/git2/bytes.hh>
#include <cov/io/file.hh>
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

	}  // namespace

	TEST(file, ctors) {
		io::file one{};
		io::file two = io::fopen(setup::test_dir() / "file"sv, "w");
		ASSERT_FALSE(one);
		ASSERT_TRUE(two);
		std::swap(one, two);
		ASSERT_TRUE(one);
		ASSERT_FALSE(two);
	}

	TEST(file, io) {
		auto const filename = setup::test_dir() / "file"sv;
		std::error_code ec{};
		std::filesystem::remove(filename, ec);
		ASSERT_FALSE(ec) << "What: " << ec.message();

		{
			auto in = io::fopen(filename, "rb");
			ASSERT_FALSE(in);
		}
		{
			auto out = io::fopen(filename, "wb");
			ASSERT_TRUE(out);
			ASSERT_EQ(std::size(readme_z),
			          out.store(readme_z, std::size(readme_z)));
		}
		{
			auto in = io::fopen(filename, "rb");
			ASSERT_TRUE(in);
			std::vector<unsigned char> buffer(std::size(readme_z), '\0');
			ASSERT_EQ(std::size(readme_z),
			          in.load(buffer.data(), buffer.size()));
			ASSERT_EQ((std::basic_string_view{readme_z, std::size(readme_z)}),
			          (std::basic_string_view{buffer.data(), buffer.size()}));
		}
	}

	TEST(file, skip) {
		auto const filename = setup::test_dir() / "file"sv;
		std::error_code ec{};
		std::filesystem::remove(filename, ec);
		ASSERT_FALSE(ec) << "What: " << ec.message();

		{
			auto in = io::fopen(filename, "rb");
			ASSERT_FALSE(in);
		}
		{
			auto out = io::fopen(filename, "wb");
			ASSERT_TRUE(out);
			ASSERT_EQ(std::size(readme_z),
			          out.store(readme_z, std::size(readme_z)));
		}
		{
			static constexpr auto pre_read = 4u;
			static constexpr auto skip = 7u;
			static constexpr auto read = 10u;

			auto in = io::fopen(filename, "rb");
			ASSERT_TRUE(in);
			std::vector<unsigned char> buffer(std::size(readme_z), '\0');

			ASSERT_EQ(pre_read, in.load(buffer.data(), pre_read));
			ASSERT_EQ(
			    (std::basic_string_view{readme_z, std::size(readme_z)}.substr(
			        0, pre_read)),
			    (std::basic_string_view{buffer.data(), pre_read}));

			ASSERT_TRUE(in.skip(skip));

			ASSERT_EQ(read, in.load(buffer.data(), read));
			ASSERT_EQ(
			    (std::basic_string_view{readme_z, std::size(readme_z)}.substr(
			        pre_read + skip, read)),
			    (std::basic_string_view{buffer.data(), read}));
		}
	}

	TEST(file, lines) {
		static constexpr std::string_view input[] = {
		    "first line"sv,  "second line"sv, "third line"sv,
		    "fourth line"sv, "last line"sv,
		};

		// write
		{
			auto newline = '\n';
			auto out = io::fopen(setup::test_dir() / "lines"sv, "wb");
			ASSERT_TRUE(out);
			bool first = true;
			for (auto const line : input) {
				if (first)
					first = false;
				else
					out.store(&newline, 1);
				out.store(line.data(), line.size());
			}
		}

		// read
		std::vector<std::string> actual{};
		std::vector<std::string> expected{std::begin(input), std::end(input)};

		actual.reserve(std::size(input));
		auto in = io::fopen(setup::test_dir() / "lines"sv, "rb");
		ASSERT_TRUE(in);
		while (!in.feof())
			actual.push_back(in.read_line());

		ASSERT_EQ(expected, actual);
	}

	TEST(file, long_lines) {
		static constexpr std::string_view input[] = {
		    "first line"sv,
		    "second line"sv,
		    "012345678901234567890123456789012345678901234567890123456789"
		    "012345678901234567890123456789012345678901234567890123456789"
		    "012345678901234567890123456789012345678901234567890123456789"
		    "012345678901234567890123456789012345678901234567890123456789"
		    "012345678901234567890123456789012345678901234567890123456789"
		    "012345678901234567890123456789012345678901234567890123456789"
		    "012345678901234567890123456789012345678901234567890123456789"
		    "012345678901234567890123456789012345678901234567890123456789"
		    "012345678901234567890123456789012345678901234567890123456789"
		    "012345678901234567890123456789012345678901234567890123456789"
		    "012345678901234567890123456789012345678901234567890123456789"
		    "012345678901234567890123456789012345678901234567890123456789"
		    "012345678901234567890123456789012345678901234567890123456789"
		    "012345678901234567890123456789012345678901234567890123456789"
		    "012345678901234567890123456789012345678901234567890123456789"
		    "012345678901234567890123456789012345678901234567890123456789"
		    "012345678901234567890123456789012345678901234567890123456789"
		    "012345678901234567890123456789012345678901234567890123456789"
		    "012345678901234567890123456789012345678901234567890123456789"
		    "012345678901234567890123456789012345678901234567890123456789"sv,
		    "fourth line"sv,
		    "last line"sv,
		};

		// write
		{
			auto newline = '\n';
			auto out = io::fopen(setup::test_dir() / "lines"sv, "wb");
			ASSERT_TRUE(out);
			bool first = true;
			for (auto const line : input) {
				if (first)
					first = false;
				else
					out.store(&newline, 1);
				out.store(line.data(), line.size());
			}
		}

		// read
		std::vector<std::string> actual{};
		std::vector<std::string> expected{std::begin(input), std::end(input)};

		actual.reserve(std::size(input));
		auto in = io::fopen(setup::test_dir() / "lines"sv, "rb");
		ASSERT_TRUE(in);
		while (!in.feof())
			actual.push_back(in.read_line());

		ASSERT_EQ(expected, actual);
	}
}  // namespace cov::testing
