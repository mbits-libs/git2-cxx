// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/object.hh>
#include <type_traits>

namespace cov::testing {
	namespace classes {
		class derived1;
		class derived2;
		class derived3;

		struct base : counted_impl<> {
			template <typename Derived>
			inline bool is_a() const noexcept;
			virtual bool is_derived1() const noexcept { return false; }
			virtual bool is_derived2() const noexcept { return false; }
			virtual bool is_derived3() const noexcept { return false; }
		};

		template <>
		bool base::is_a<derived1>() const noexcept {
			return is_derived1();
		}

		template <>
		bool base::is_a<derived2>() const noexcept {
			return is_derived2();
		}

		template <>
		bool base::is_a<derived3>() const noexcept {
			return is_derived3();
		}

		class base2 {
#ifndef __clang__
			unsigned x{};
			unsigned y{};
			unsigned z{};
#endif
		};

		class derived1 : public base {
		private:
			bool is_derived1() const noexcept final { return true; }
		};

		class derived2 : public base, private base2 {
		private:
			bool is_derived2() const noexcept final { return true; }
		};

		class derived3 : private base2, public base {
		private:
			bool is_derived3() const noexcept final { return true; }
		};

	}  // namespace classes

	struct is_a : ::testing::Test {
		template <typename Orig, typename Casted>
		void ref() {
			Orig orig{};
			classes::base& b{orig};
			auto result = as_a<Casted>(b);
			if constexpr (std::same_as<Orig, Casted>) {
				ASSERT_EQ(&orig, result);
			} else {
				ASSERT_EQ(nullptr, result);
			}
		}

		template <typename Orig, typename Casted>
		void cref() {
			Orig orig{};
			classes::base const& b{orig};
			auto result = as_a<Casted>(b);
			if constexpr (std::same_as<Orig, Casted>) {
				ASSERT_EQ(&orig, result);
			} else {
				ASSERT_EQ(nullptr, result);
			}
		}

		template <typename Orig, typename Casted>
		void ptr() {
			Orig orig{};
			classes::base* b{&orig};
			auto result = as_a<Casted>(b);
			if constexpr (std::same_as<Orig, Casted>) {
				ASSERT_EQ(&orig, result);
			} else {
				ASSERT_EQ(nullptr, result);
			}
		}

		template <typename Orig, typename Casted>
		void cptr() {
			Orig orig{};
			classes::base const* b{&orig};
			auto result = as_a<Casted>(b);
			if constexpr (std::same_as<Orig, Casted>) {
				ASSERT_EQ(&orig, result);
			} else {
				ASSERT_EQ(nullptr, result);
			}
		}

		template <typename Orig, typename Casted>
		void smart() {
			ref_ptr<Orig> orig{take(new Orig)};
			ref_ptr<classes::base> b{orig};

			std::error_code ec{};
			auto const result = as_a<Casted>(b, ec);
			if constexpr (std::same_as<Orig, Casted>) {
				EXPECT_FALSE(ec);
				EXPECT_EQ(3, orig.counter());
				ASSERT_EQ(orig.get(), result.get());
			} else {
				EXPECT_TRUE(ec);
				EXPECT_EQ(2, orig.counter());
				ASSERT_EQ(nullptr, result.get());
			}
		}

		template <typename Orig, typename Casted>
		void csmart() {
			ref_ptr<Orig> orig{take(new Orig)};
			ref_ptr<classes::base> const b{orig};

			std::error_code ec{};
			auto const result = as_a<Casted>(b, ec);
			if constexpr (std::same_as<Orig, Casted>) {
				EXPECT_FALSE(ec);
				EXPECT_EQ(3, orig.counter());
				ASSERT_EQ(orig.get(), result.get());
			} else {
				EXPECT_TRUE(ec);
				EXPECT_EQ(2, orig.counter());
				ASSERT_EQ(nullptr, result.get());
			}
		}
	};

#define IS_A_TEST_(test, type1, type2)          \
	TEST_F(is_a, test##_##type1##_##type2) {    \
		test<classes::type1, classes::type2>(); \
	}
#define IS_A_TEST(test)                   \
	IS_A_TEST_(test, derived1, derived1); \
	IS_A_TEST_(test, derived1, derived2); \
	IS_A_TEST_(test, derived1, derived3); \
	IS_A_TEST_(test, derived2, derived1); \
	IS_A_TEST_(test, derived2, derived2); \
	IS_A_TEST_(test, derived2, derived3); \
	IS_A_TEST_(test, derived3, derived1); \
	IS_A_TEST_(test, derived3, derived2); \
	IS_A_TEST_(test, derived3, derived3);

	IS_A_TEST(ref);
	IS_A_TEST(cref);
	IS_A_TEST(ptr);
	IS_A_TEST(cptr);
	IS_A_TEST(smart);
	IS_A_TEST(csmart);
}  // namespace cov::testing
