// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/object.hh>
#include <type_traits>

namespace cov::testing {
	namespace classes {
		class derived1;
		class derived2;
		class derived3;

		struct base {
			virtual ~base() = default;
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
			unsigned x{};
			unsigned y{};
			unsigned z{};
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
}  // namespace cov::testing
