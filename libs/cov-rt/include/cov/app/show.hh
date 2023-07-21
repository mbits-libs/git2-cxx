// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/app/args.hh>
#include <cov/app/cov_log_tr.hh>
#include <cov/app/cov_show_tr.hh>
#include <cov/app/cov_tr.hh>
#include <cov/app/errors_tr.hh>
#include <cov/app/rt_path.hh>
#include <cov/app/show_range.hh>
#include <cov/format.hh>
#include <cov/projection.hh>
#include <cov/repository.hh>
#include <cov/revparse.hh>
#include <string>
#include <vector>

namespace cov {
	struct data_table;
}

namespace cov::app::show {
	struct parser : base_parser<covlng, loglng, errlng, showlng> {
		parser(::args::args_view const& arguments,
		       str::translator_open_info const& langs);

		struct response {
			revs range{};
			std::string path{};
			cov::repository repo{};
		};

		response parse();
		using colorizer_t = std::string (*)(placeholder::color clr, void*);
		colorizer_t colorizer() const noexcept;

		cov::repository open_here() const {
			return cov::app::open_here(*this, tr_);
		}

		std::optional<std::string> rev{};
		std::optional<std::string> module{};
		show_range show{};
	};

	enum class with : unsigned {
		branches = 0x0001,
		functions = 0x0002,
		branches_missing = 0x0004,
		functions_missing = 0x0008,
		lines_missing = 0x0010,
	};

	inline with operator|(with lhs, with rhs) noexcept {
		return static_cast<with>(std::to_underlying(lhs) |
		                         std::to_underlying(rhs));
	}

	inline with& operator|=(with& lhs, with rhs) noexcept {
		lhs = lhs | rhs;
		return lhs;
	}

	inline with operator~(with lhs) noexcept {
		return static_cast<with>(~std::to_underlying(lhs));
	}

	inline with operator&(with lhs, with rhs) noexcept {
		return static_cast<with>(std::to_underlying(lhs) &
		                         std::to_underlying(rhs));
	}

	struct environment {
		enum class row_type : bool { footer = false, data = true };
		std::string (*colorizer)(placeholder::color, void*) = nullptr;
		void* app = nullptr;
		placeholder::rating marks{};

		template <typename T>
		static bool is_zero(T val) {
			return val == T{};
		}

		template <typename Int>
		static bool is_zero(io::v1::stats::ratio<Int> const& val) {
			return val.whole == 0 && val.fraction == 0;
		}

		std::string val(auto const& V) const { return fmt::format("{}", V); }

		std::string apply_mark(std::string_view label,
		                       placeholder::color color,
		                       io::v1::stats const& stats) const {
			return fmt::format("{}{}{}", color_for(color, &stats), label,
			                   color_for(placeholder::color::reset));
		}

		std::string val_sign(
		    auto const& V,
		    placeholder::color base = placeholder::color::faint_green) const {
			using color = placeholder::color;
			using T = std::remove_cvref_t<decltype(V)>;
			if (is_zero(V)) return {};
			if (V < T{}) {
				if (base == color::faint_green)
					base = color::faint_red;
				else if (base == color::faint_red)
					base = color::faint_green;
			}
			return fmt::format("{}{:+}{}", color_for(base), V,
			                   color_for(color::reset));
		}

		template <typename Getter>
		void percentage(std::vector<std::string>& cells,
		                file_diff const& change,
		                Getter const& getter,
		                placeholder::color rating_color) const {
			cells.push_back(apply_mark(val(getter(change.coverage.current)),
			                           rating_color,
			                           getter(change.stats.current)));
			cells.push_back(val_sign(getter(change.coverage.diff),
			                         placeholder::color::faint_green));
		}

		template <typename Getter>
		void count(
		    std::vector<std::string>& cells,
		    file_diff const& change,
		    Getter const& getter,
		    placeholder::color base = placeholder::color::faint_green) const {
			cells.push_back(val(getter(change.stats.current)));
			cells.push_back(val_sign(getter(change.stats.diff), base));
		}

		template <typename Getter>
		void dimmed_count(
		    std::vector<std::string>& cells,
		    file_diff const& change,
		    Getter const& getter,
		    placeholder::color base = placeholder::color::faint_green) const {
			auto const& value = getter(change.stats.current);
			std::string str{};
			if (!is_zero(value)) {
				str = val(value);
			}
			cells.push_back(str);
			cells.push_back(val_sign(getter(change.stats.diff), base));
		}

		std::string color_for(placeholder::color clr,
		                      io::v1::stats const* stats = nullptr) const;
		void add(data_table& table,
		         char type,
		         projection::entry_stats const& stats,
		         std::string_view label,
		         with flags,
		         row_type row = row_type::data) const;
		void print_table(std::vector<projection::entry> const& entries) const;
	};
}  // namespace cov::app::show
