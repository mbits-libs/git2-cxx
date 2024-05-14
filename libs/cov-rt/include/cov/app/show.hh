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
#include <cov/core/report_stats.hh>
#include <string>
#include <vector>

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

		std::string color_for(placeholder::color clr) const;

		struct add_table_row_options {
			std::string_view label{};
			char entry_flag{};
			std::span<core::cell_info const> cells{};
			std::span<core::column_info const> columns{};
		};

		std::vector<std::string> add_table_row(
		    add_table_row_options const& options) const;

		void print_table(std::vector<projection::entry> const& entries) const;
	};
}  // namespace cov::app::show
