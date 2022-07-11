// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <git2/errors.h>
#include <gtest/gtest.h>
#include <git2/error.hh>

namespace git::testing {
	using namespace ::std::literals;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;

	struct error_param {
		git_error_code git2;
		errc cxx;

		friend std::ostream& operator<<(std::ostream& out,
		                                error_param const& param) {
			return out << '(' << param.git2 << ") "
			           << git::category().message(param.git2);
		}
	};

	class error : public TestWithParam<error_param> {};

	TEST_P(error, same_value) {
		auto const [git2, cxx] = GetParam();
		ASSERT_EQ(git2, static_cast<int>(cxx));
	}

	TEST_P(error, same_error_code) {
		auto const [git2, cxx] = GetParam();
		ASSERT_EQ(cxx, git::make_error_code(git2));
	}

	TEST_P(error, has_message) {
		auto const [git2, cxx] = GetParam();
		ASSERT_FALSE(git::make_error_code(git2).message().empty());
	}

	TEST(error, unknown_has_message) {
		ASSERT_EQ("git::errc{100}", git::make_error_code(100).message());
	}

	TEST(error, category_has_name) {
		ASSERT_STREQ(git::category().name(),
		             git::make_error_code(GIT_ERROR).category().name());
		// same pointer:
		ASSERT_EQ(git::category().name(),
		          git::make_error_code(GIT_ERROR).category().name());
	}

	static constexpr error_param errors[] = {
	    {GIT_ERROR, errc::error},
	    {GIT_ENOTFOUND, errc::notfound},
	    {GIT_EEXISTS, errc::exists},
	    {GIT_EAMBIGUOUS, errc::ambiguous},
	    {GIT_EBUFS, errc::bufs},
	    {GIT_EUSER, errc::user},
	    {GIT_EBAREREPO, errc::barerepo},
	    {GIT_EUNBORNBRANCH, errc::unbornbranch},
	    {GIT_EUNMERGED, errc::unmerged},
	    {GIT_ENONFASTFORWARD, errc::nonfastforward},
	    {GIT_EINVALIDSPEC, errc::invalidspec},
	    {GIT_ECONFLICT, errc::conflict},
	    {GIT_ELOCKED, errc::locked},
	    {GIT_EMODIFIED, errc::modified},
	    {GIT_EAUTH, errc::auth},
	    {GIT_ECERTIFICATE, errc::certificate},
	    {GIT_EAPPLIED, errc::applied},
	    {GIT_EPEEL, errc::peel},
	    {GIT_EEOF, errc::eof},
	    {GIT_EINVALID, errc::invalid},
	    {GIT_EUNCOMMITTED, errc::uncommitted},
	    {GIT_EDIRECTORY, errc::directory},
	    {GIT_EMERGECONFLICT, errc::mergeconflict},
	    {GIT_PASSTHROUGH, errc::passthrough},
	    {GIT_ITEROVER, errc::iterover},
	    {GIT_RETRY, errc::retry},
	    {GIT_EMISMATCH, errc::mismatch},
	    {GIT_EINDEXDIRTY, errc::indexdirty},
	    {GIT_EAPPLYFAIL, errc::applyfail},
	};

	INSTANTIATE_TEST_SUITE_P(git, error, ValuesIn(errors));
}  // namespace git::testing
