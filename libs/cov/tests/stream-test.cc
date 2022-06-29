// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/io/safe_stream.hh>
#include <git2/bytes.hh>
#include "setup.hh"

namespace cov::testing {
	namespace {
		using namespace ::std::literals;

		static constexpr auto readme_text = "blob 16\x00# Testing repos\n"sv;
		static constexpr unsigned char readme_z[] = {
		    0x78, 0x01, 0x4b, 0xca, 0xc9, 0x4f, 0x52, 0x30, 0x34, 0x63, 0x50,
		    0x56, 0x08, 0x49, 0x2d, 0x2e, 0xc9, 0xcc, 0x4b, 0x57, 0x28, 0x4a,
		    0x2d, 0xc8, 0x2f, 0xe6, 0x02, 0x00, 0x5b, 0x5a, 0x07, 0x9b};
		static constexpr auto readme_dir = "d8"sv;
		static constexpr auto readme_path =
		    "d8/5c6a23a700aa20fda7cfae8eaa9e80ea22dde0"sv;
		static constexpr auto readme_sha =
		    "d85c6a23a700aa20fda7cfae8eaa9e80ea22dde0"sv;

		template <size_t Length>
		inline git::bytes wrap(unsigned char const (&data)[Length]) {
			return git::bytes{data, Length};
		}

		auto prep_file(std::string_view name) {
			static constexpr auto content = "first line\nsecond line\n"sv;
			auto outname = setup::test_dir() / setup::make_path(name);
			auto const out = io::fopen(outname, "wb");
			out.store(content.data(), content.size());
			return outname;
		}

		auto load_file(std::filesystem::path const& path) {
			auto in = io::fopen(path);
			auto const bytes = in.read();
			in.close();
			return std::string{reinterpret_cast<char const*>(bytes.data()),
			                   bytes.size()};
		}
	}  // namespace

	TEST(stream, safe) {
		auto const outname = prep_file("secured.txt"sv);
		ASSERT_FALSE(outname.empty());
		{
			io::safe_stream out{outname};
			ASSERT_TRUE(out.opened());
			out.write(git::bytes{"first line\n"sv});
		}
		auto const data = load_file(outname);
		ASSERT_EQ("first line\nsecond line\n"sv, data);
	}

	TEST(stream, safe_commit) {
		auto const outname = prep_file("secured.txt"sv);
		ASSERT_FALSE(outname.empty());
		{
			io::safe_stream out{outname};
			ASSERT_TRUE(out.opened());
			out.write(git::bytes{"first line\n"sv});
			out.write(git::bytes{"second line\n"sv});
			out.write(git::bytes{"third line\n"sv});
			out.commit();
		}
		auto const data = load_file(outname);
		ASSERT_EQ("first line\nsecond line\nthird line\n"sv, data);
	}

	TEST(stream, safe_rollback) {
		auto const outname = prep_file("secured.txt"sv);
		ASSERT_FALSE(outname.empty());
		{
			io::safe_stream out{outname};
			ASSERT_TRUE(out.opened());
			out.write(git::bytes{"first line\n"sv});
			out.write(git::bytes{"second line\n"sv});
			out.write(git::bytes{"third line\n"sv});
			out.rollback();
		}
		auto const data = load_file(outname);
		ASSERT_EQ("first line\nsecond line\n"sv, data);
	}

	TEST(stream, unsecured) {
		auto const outname = prep_file("unsecured.txt"sv);
		ASSERT_FALSE(outname.empty());
		{
			auto out = io::fopen(outname, "wb");
			ASSERT_TRUE(out);
			static constexpr auto data = "something different\n"sv;
			out.store(data.data(), data.size());
			out.close();
		}
		auto const data = load_file(outname);
		ASSERT_NE("first line\nsecond line\n"sv, data);
	}

	TEST(stream, z_fail) {
		std::error_code ec{};
		std::filesystem::remove_all(
		    setup::test_dir() / setup::make_path(readme_dir), ec);
		ASSERT_FALSE(ec) << "What: " << ec.message();

		{
			auto const bytes = git::bytes{readme_text};
			auto out = io::safe_z_stream{setup::test_dir(), "binary"sv};
			ASSERT_TRUE(out.opened());
			out.write(bytes.subview(0, bytes.size() / 2));
			out.rollback();
		}

		ASSERT_FALSE(std::filesystem::exists(setup::test_dir() /
		                                     setup::make_path(readme_path)));
	}

	TEST(stream, z_success) {
		std::error_code ec{};
		std::filesystem::remove_all(
		    setup::test_dir() / setup::make_path(readme_dir), ec);
		ASSERT_FALSE(ec) << "What: " << ec.message();

		{
			auto const bytes = git::bytes{readme_text};
			auto out = io::safe_z_stream{setup::test_dir(), "binary"sv};
			ASSERT_TRUE(out.opened());
			out.write(bytes.subview(0, bytes.size() / 2));
			out.write(bytes.subview(bytes.size() / 2));
			auto const oid = out.finish();
			std::string actual =
			    std::string(static_cast<size_t>(GIT_OID_HEXSZ), '\0');
			git_oid_fmt(actual.data(), &oid);
			ASSERT_EQ(readme_sha, actual);
		}

		auto const path = setup::test_dir() / setup::make_path(readme_path);
		ASSERT_TRUE(std::filesystem::exists(path));

		auto in = io::fopen(path, "rb");
		auto const bytes = in.read();
		in.close();

		auto z = buffer_zstream{zstream::inflate};
		auto const used = z.append({bytes.data(), bytes.size()});
		auto const result = z.close();
		auto const bin = git::bytes{result};
		auto const view = std::string_view{
		    reinterpret_cast<char const*>(bin.data()), bin.size()};

		ASSERT_EQ(bytes.size(), used);
		ASSERT_EQ(readme_text, view);
	}
}  // namespace cov::testing
