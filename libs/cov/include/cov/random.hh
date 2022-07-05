// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <random>

namespace cov {
	template <size_t len>
	struct mt_t;
	template <>
	struct mt_t<4> {
		using type = std::mt19937;
	};
	template <>
	struct mt_t<8> {
		using type = std::mt19937_64;
	};
	template <size_t len>
	using mt = typename mt_t<len>::type;

	struct seed_sequence {  // GCOV_EXCL_LINE -- automagical functions
		std::random_device rd{};

		using result_type = std::random_device::result_type;
		using device_type = cov::mt<sizeof(size_t)>;

		template <typename RandomAccessIterator>
		void generate(RandomAccessIterator begin, RandomAccessIterator end) {
			using value_type =
			    typename std::iterator_traits<RandomAccessIterator>::value_type;
			std::uniform_int_distribution<value_type> bits{
			    (std::numeric_limits<value_type>::min)(),
			    (std::numeric_limits<value_type>::max)(),
			};

			for (auto it = begin; it != end; ++it) {
				*it = bits(rd);
			}
		}

		static device_type mt() {
			seed_sequence seq{};
			return device_type{seq};
		}
	};
}  // namespace cov
