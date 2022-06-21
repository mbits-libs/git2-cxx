// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/hash/md5.hh>
#include <cov/hash/sha1.hh>

namespace hash::testing {
	using namespace ::std::literals;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;

	struct hash_iface {
		virtual ~hash_iface() = default;
		virtual void update(std::string_view) = 0;
		virtual std::string finalize() = 0;
	};

	struct hash_functions {
		std::string (*once)(std::string_view);
		std::unique_ptr<hash_iface> (*ctor)();
	};

	struct hash_info {
		std::string_view expected{};
		std::string_view content{};
		hash_functions (*hash_fn)();

		friend std::ostream& operator<<(std::ostream& out,
		                                hash_info const& param) {
			return out << '"' << param.expected << '"';
		}
	};

	class hash : public TestWithParam<hash_info> {};

	TEST_P(hash, digest) {
		auto const [expected, content, hash_fn] = GetParam();
		auto const actual = hash_fn().once(content);
		ASSERT_EQ(expected, actual);
	}

	TEST_P(hash, chunks_10) {
		static constexpr auto chunk_size = 10u;
		auto const [expected, content, hash_fn] = GetParam();
		auto engine = hash_fn().ctor();
		auto data = content;

		while (!data.empty()) {
			auto const chunk =
			    data.length() <= chunk_size ? data : data.substr(0, chunk_size);
			data = data.substr(chunk.length());
			engine->update(chunk);
		}

		auto const actual = engine->finalize();
		ASSERT_EQ(expected, actual);
	}

	template <typename Hash>
	std::string hash_once(std::string_view content) {
		return Hash::once({content.data(), content.size()}).str();
	}

	template <typename Hash>
	struct hash_impl : hash_iface {
		Hash engine{};
		void update(std::string_view content) override {
			engine.update({content.data(), content.size()});
		}
		std::string finalize() override { return engine.finalize().str(); }
		static std::unique_ptr<hash_iface> ctor() {
			return std::make_unique<hash_impl>();
		}
	};

	template <typename Hash>
	hash_functions hash_fn() {
		return {hash_once<Hash>, hash_impl<Hash>::ctor};
	}

	static constexpr hash_info md5s[] = {
	    {"d41d8cd98f00b204e9800998ecf8427e"sv, {}, hash_fn<md5>},
	    {
	        "9e107d9d372bb6826bd81d3542a419d6"sv,
	        "The quick brown fox jumps over the lazy dog"sv,
	        hash_fn<md5>,
	    },
	    {"0cc175b9c0f1b6a831c399e269772661"sv, "a"sv, hash_fn<md5>},
	    {"900150983cd24fb0d6963f7d28e17f72"sv, "abc"sv, hash_fn<md5>},
	    {"f96b697d7cb7938d525a2f31aaf161d0"sv, "message digest"sv,
	     hash_fn<md5>},
	    {"c3fcd3d76192e4007dfb496cca67e13b"sv, "abcdefghijklmnopqrstuvwxyz"sv,
	     hash_fn<md5>},
	    {
	        "d174ab98d277d9f5a5611c2c9f419d9f"sv,
	        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"sv,
	        hash_fn<md5>,
	    },
	    {
	        "57edf4a22be3c955ac49da2e2107b67a"sv,
	        "12345678901234567890123456789012345678901234567890123456789012345678901234567890"sv,
	        hash_fn<md5>,
	    },
	};
	INSTANTIATE_TEST_SUITE_P(md5, hash, ValuesIn(md5s));

	static constexpr hash_info sha1s[] = {
	    {"da39a3ee5e6b4b0d3255bfef95601890afd80709"sv, {}, hash_fn<sha1>},
	    {
	        "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12"sv,
	        "The quick brown fox jumps over the lazy dog"sv,
	        hash_fn<sha1>,
	    },
	    {"86f7e437faa5a7fce15d1ddcb9eaeaea377667b8"sv, "a"sv, hash_fn<sha1>},
	    {"a9993e364706816aba3e25717850c26c9cd0d89d"sv, "abc"sv, hash_fn<sha1>},
	    {"c12252ceda8be8994d5fa0290a47231c1d16aae3"sv, "message digest"sv,
	     hash_fn<sha1>},
	    {"32d10c7b8cf96570ca04ce37f2a19d84240d3a89"sv,
	     "abcdefghijklmnopqrstuvwxyz"sv, hash_fn<sha1>},
	    {
	        "761c457bf73b14d27e9e9265c46f4b4dda11f940"sv,
	        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"sv,
	        hash_fn<sha1>,
	    },
	    {
	        "50abf5706a150990a08b2c5ea40fa0e585554732"sv,
	        "12345678901234567890123456789012345678901234567890123456789012345678901234567890"sv,
	        hash_fn<sha1>,
	    },
	};
	INSTANTIATE_TEST_SUITE_P(sha1, hash, ValuesIn(sha1s));

}  // namespace hash::testing
