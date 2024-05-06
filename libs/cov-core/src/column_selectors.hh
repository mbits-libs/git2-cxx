// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/core/report_stats.hh>
#include <cov/format.hh>
#include <cov/repository.hh>
#include <string>

namespace cov::core {
	struct output_field {
		cov::placeholder::color color{};
		std::string value{};
	};

	struct column_output {
		output_field main{};
		output_field diff{};
		bool is_percentage{false};
	};

	enum class change_is { in_line_with_sign, reversed };

	struct selector {
		virtual ~selector() = default;
		virtual cell_info cell(placeholder::rating const&,
		                       file_diff const&) const = 0;

	protected:
		template <typename T>
		static bool is_zero(T val) {
			return val == T{};
		}

		template <typename Int>
		static bool is_zero(io::v1::stats::ratio<Int> const& val) {
			return val.whole == 0 && val.fraction == 0;
		}

		template <typename T>
		static bool is_neg(T val) {
			return val < T{};
		}

		static output_field output_diff(auto const& V,
		                                placeholder::color base) {
			using color = placeholder::color;
			using T = std::remove_cvref_t<decltype(V)>;
			if (is_zero(V)) return {};
			if (V < T{}) {
				if (base == color::faint_green)
					base = color::faint_red;
				else if (base == color::faint_red)
					base = color::faint_green;
			}
			return {.color = base, .value = fmt::format("{:+}", V)};
		}

		static std::string format_value(auto const& V, bool dimmable = false) {
			if (dimmable && is_zero(V)) return {};
			return fmt::format("{}", V);
		}

		static std::string format_signed_value(auto const& V) {
			if (is_zero(V)) return {};
			return fmt::format("{:+}", V);
		}
	};

	struct ratio_selector : selector {
		virtual io::v1::stats::ratio<> current(
		    cov::multi_ratio<> const& src) const noexcept = 0;
		virtual io::v1::stats::ratio<int> diff(
		    cov::multi_ratio<int> const& src) const noexcept = 0;
		virtual io::v1::stats stats(
		    io::v1::coverage_stats const& src) const noexcept = 0;
		virtual placeholder::stats rating_selector() const noexcept = 0;

		cell_info cell(placeholder::rating const& marks,
		               file_diff const& diff) const override;
	};

	struct counter_selector : selector {
		virtual std::uint32_t current(
		    io::v1::coverage_stats const& src) const noexcept = 0;
		virtual std::int32_t diff(
		    io::v1::coverage_diff const& src) const noexcept = 0;
		virtual bool is_dimmed() const noexcept = 0;
		virtual change_is change_direction() const noexcept = 0;

		cell_info cell(placeholder::rating const&,
		               file_diff const& diff) const override;
	};

	template <placeholder::stats RatingSelector, typename Getter>
	struct ratio_selector_impl : ratio_selector, Getter {
		ratio_selector_impl() = default;
		Getter const& getter() const noexcept { return *this; }

		io::v1::stats::ratio<> current(
		    cov::multi_ratio<> const& src) const noexcept override {
			return getter()(src);
		}
		io::v1::stats::ratio<int> diff(
		    cov::multi_ratio<int> const& src) const noexcept override {
			return getter()(src);
		}
		io::v1::stats stats(
		    io::v1::coverage_stats const& src) const noexcept override {
			return getter()(src);
		}
		placeholder::stats rating_selector() const noexcept override {
			return RatingSelector;
		}
	};

	template <bool IsDimmed, change_is ChangeDirection, typename Getter>
	struct counter_selector_impl : counter_selector, Getter {
		counter_selector_impl() = default;
		Getter const& getter() const noexcept { return *this; }

		std::uint32_t current(
		    io::v1::coverage_stats const& src) const noexcept override {
			return getter()(src);
		}
		std::int32_t diff(
		    io::v1::coverage_diff const& src) const noexcept override {
			return getter()(src);
		}
		bool is_dimmed() const noexcept override { return IsDimmed; }
		change_is change_direction() const noexcept override {
			return ChangeDirection;
		}
	};

	using selector_factory = selector const& (*)();

	template <placeholder::stats RatingSelector, typename Getter>
	inline constexpr selector_factory get_ratio_sel(Getter) {
		return +[]() -> selector const& {
			static ratio_selector_impl<RatingSelector, Getter> result{};
			return result;
		};
	}

	template <change_is ChangeDirection, typename Getter>
	inline constexpr selector_factory get_counter_sel(Getter) {
		return +[]() -> selector const& {
			static counter_selector_impl<false, ChangeDirection, Getter>
			    result{};
			return result;
		};
	}

	template <change_is ChangeDirection, typename Getter>
	inline constexpr selector_factory get_dimmed_counter_sel(Getter) {
		return +[]() -> selector const& {
			static counter_selector_impl<true, ChangeDirection, Getter>
			    result{};
			return result;
		};
	}
}  // namespace cov::core
