// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <args/actions.hpp>

namespace cov {
	enum class use_feature { no, yes, automatic };

	template <typename tag>
	struct feature {
		use_feature value{use_feature::automatic};
		// aggressively inlined?
		constexpr feature() = default;  // GCOV_EXCL_LINE
		constexpr feature(use_feature v) : value{v} {}
		feature& operator=(use_feature v) {
			value = v;
			return *this;
		}
		feature& operator=(feature const&) = default;

		bool operator==(feature const&) const noexcept = default;
		bool operator==(use_feature val) const noexcept { return value == val; }
	};
}  // namespace cov

namespace args {
	template <typename tag>
	struct names_helper<cov::feature<tag>> {
		using name_info = std::pair<std::string_view, cov::feature<tag>>;
		static inline simple_span<name_info> const& names() {
			using enum_stg = cov::use_feature;
			static constexpr name_info enum_names[] = {
			    {tag::no_name(), {enum_stg::no}},
			    {tag::yes_name(), {enum_stg::yes}},
			    {"auto", {enum_stg::automatic}},
			};

			static constexpr simple_span<name_info> span{enum_names};
			return span;
		}
	};

	template <typename tag>
	struct enum_traits<cov::feature<tag>>
	    : enum_traits_base<cov::feature<tag>, names_helper<cov::feature<tag>>> {
		using parent = enum_traits_base<cov::feature<tag>,
		                                names_helper<cov::feature<tag>>>;
		using name_info = typename parent::name_info;
	};

	template <typename tag>
	requires std::derived_from<tag, cov::feature<tag>>
	struct converter<tag> : enum_converter<cov::feature<tag>> {
	};
}  // namespace args

#define MAKE_FEATURE(NAME, YES, NO)                                \
	struct NAME##_feature : feature<NAME##_feature> {              \
		using feature<NAME##_feature>::feature;                    \
		using feature<NAME##_feature>::operator=;                  \
		constexpr static auto yes_name() noexcept { return #YES; } \
		constexpr static auto no_name() noexcept { return #NO; }   \
	}

namespace cov {
	MAKE_FEATURE(decorate, short, no);
	MAKE_FEATURE(color, always, never);
}  // namespace cov
