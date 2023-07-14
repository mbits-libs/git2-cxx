// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/git2/odb.hh>
#include "setup.hh"

namespace git::testing {
	using namespace ::std::literals;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;

	struct hasher_param {
		std::string_view hash{};
		std::string_view content{};

		friend std::ostream& operator<<(std::ostream& out,
		                                hasher_param const& param) {
			out << '"';
			for (auto c : param.content) {
				switch (c) {
					case '\n':
						out << "\\n";
						break;
					case '\r':
						out << "\\r";
						break;
					case '\t':
						out << "\\t";
						break;
					case '\v':
						out << "\\v";
						break;
					case '\a':
						out << "\\a";
						break;
					default:
						if (c < 32) {
							char buffer[10];
							snprintf(buffer, sizeof(buffer), "%03o", c);
							out << buffer;
						} else {
							out << c;
						}
						break;
				}
			}
			return out << '"';
		}
	};

	struct hasher : TestWithParam<hasher_param> {
		git::odb create() {
			std::error_code ec{};
			auto result = git::odb::create(ec);
			if (ec) throw ec;
			return result;
		}
		git::odb const odb = create();
		std::string actual =
		    std::string(static_cast<size_t>(GIT_OID_HEXSZ), '\0');
	};

	TEST_P(hasher, write) {
		auto [expected, bytes] = GetParam();
		auto const path = setup::test_dir() / "hasher-write"sv;
		remove_all(path);
		create_directories(path);
		std::error_code ec{};
		auto local_odb = git::odb::open(path, ec);
		ASSERT_FALSE(ec);
		git_oid oid;
		auto const result = local_odb.write(
		    &oid, git::bytes{bytes.data(), bytes.size()}, GIT_OBJECT_BLOB);
		git_oid_fmt(actual.data(), &oid);
		ASSERT_EQ(expected, actual);
		ASSERT_FALSE(result);
	}

	TEST_P(hasher, hash) {
		auto [expected, bytes] = GetParam();
		git_oid oid;
		odb.hash(&oid, git::bytes{bytes.data(), bytes.size()}, GIT_OBJECT_BLOB);
		git_oid_fmt(actual.data(), &oid);
		ASSERT_EQ(expected, actual);
	}

	constexpr hasher_param hashes[] = {
	    {"e69de29bb2d1d6434b8b29ae775ad8c2e48c5391"sv, {}},
	    {"b870d82622c1a9ca6bcaf5df639680424a1904b0"sv,
	     "ref: refs/heads/main\n"sv},
	};

	INSTANTIATE_TEST_SUITE_P(hashes, hasher, ValuesIn(hashes));

	struct exists_param {
		std::string_view repo{};
		std::string_view hash{};
		bool expected{true};

		friend std::ostream& operator<<(std::ostream& out,
		                                exists_param const& param) {
			if (param.hash.empty())
				out << "{}";
			else
				out << param.hash;
			out << ' ';
			if (!param.expected) out << '!';
			return out << "@[$REPOS/" << param.repo << ']';
		}
	};

	struct exists : TestWithParam<exists_param> {};

	TEST_P(exists, hash) {
		auto [repo, hash, expected] = GetParam();
		std::error_code ec{};
		git::odb const odb =
		    git::odb::open(setup::test_dir() / setup::make_u8path(repo), ec);
		ASSERT_FALSE(ec);
		git_oid oid;
		ASSERT_EQ(0, git_oid_fromstrn(&oid, hash.data(), hash.length()));

		auto actual = odb.exists(oid);
		ASSERT_EQ(expected, actual);
	}

	constexpr auto bare_git = "bare.git/objects"sv;
	constexpr auto bare_sub = "gitdir/.git/modules/bare/objects"sv;
	constexpr auto gitdir = "gitdir/.git/objects"sv;

	constexpr exists_param good_hashes[] = {
	    {bare_git, "71bb790a4e8eb02b82f948dfb13d9369b768d047"sv},
	    {bare_git, "d85c6a23a700aa20fda7cfae8eaa9e80ea22dde0"sv},
	    {bare_git, "ed631389fc343f7788bf414c2b3e77749a15deb6"sv},
	    {bare_sub, "71bb790a4e8eb02b82f948dfb13d9369b768d047"sv},
	    {bare_sub, "d85c6a23a700aa20fda7cfae8eaa9e80ea22dde0"sv},
	    {bare_sub, "ed631389fc343f7788bf414c2b3e77749a15deb6"sv},
	    {gitdir, "42d6ab7898301b32aad70191b30eff94e73a2934"sv},
	    {gitdir, "5618e17182d490463731fa55d3c93518ae7bc227"sv},
	    {gitdir, "56e4e98288fac4ca89846502f0ed1d38b3182e40"sv},
	    {gitdir, "71e6ee11cf9c7dd8f2f71415c3ee4a3a411d9c85"sv},
	    {gitdir, "bca9d5f2a50092f1ad2f52d03bded3bd9d2b5edd"sv},
	};

	INSTANTIATE_TEST_SUITE_P(good, exists, ValuesIn(good_hashes));

	constexpr exists_param bad_hashes[] = {
#if 0
	    {bare_git, "00000000000000000000000000000000000000000"sv, false},
	    {gitdir, "00000000000000000000000000000000000000000"sv, false},
	    {bare_sub, "00000000000000000000000000000000000000000"sv, false},
	    {bare_git, {}, false},
	    {gitdir, {}, false},
	    {bare_sub, {}, false},
#endif

	    {bare_git, "42d6ab7898301b32aad70191b30eff94e73a2934"sv, false},
	    {bare_git, "5618e17182d490463731fa55d3c93518ae7bc227"sv, false},
	    {bare_git, "56e4e98288fac4ca89846502f0ed1d38b3182e40"sv, false},

	    {bare_sub, "71e6ee11cf9c7dd8f2f71415c3ee4a3a411d9c85"sv, false},
	    {bare_sub, "bca9d5f2a50092f1ad2f52d03bded3bd9d2b5edd"sv, false},

	    {gitdir, "71bb790a4e8eb02b82f948dfb13d9369b768d047"sv, false},
	    {gitdir, "d85c6a23a700aa20fda7cfae8eaa9e80ea22dde0"sv, false},
	    {gitdir, "ed631389fc343f7788bf414c2b3e77749a15deb6"sv, false},
	};

	INSTANTIATE_TEST_SUITE_P(bad, exists, ValuesIn(bad_hashes));
}  // namespace git::testing
