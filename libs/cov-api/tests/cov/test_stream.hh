// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <cov/streams.hh>
#include <limits>
#include <vector>

namespace cov::testing {
	class test_stream final : public write_stream {
	public:
		static constexpr size_t infinite = std::numeric_limits<size_t>::max();
		std::vector<std::byte> data{};
		size_t free_size{infinite};

		bool opened() const noexcept final;
		size_t write(git::bytes block) final;
		std::string_view view() const noexcept;
	};
}  // namespace cov::testing
