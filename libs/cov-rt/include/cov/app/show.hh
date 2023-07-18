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

		std::string val(auto const& V, std::string_view suffix = {}) const {
			return fmt::format("{}{}", V, suffix);
		}

		std::string apply_mark(std::string_view label,
		                       io::v1::stats const& stats) const {
			return fmt::format(
			    "{}{}{}", color_for(placeholder::color::bold_rating, &stats),
			    label, color_for(placeholder::color::reset));
		}

		std::string val_sign(
		    auto const& V,
		    placeholder::color base = placeholder::color::faint_green,
		    std::string_view suffix = {}) const {
			using color = placeholder::color;
			using T = std::remove_cvref_t<decltype(V)>;
			if (is_zero(V)) return {};
			if (V < T{}) {
				if (base == color::faint_green)
					base = color::faint_red;
				else if (base == color::faint_red)
					base = color::faint_green;
			}
			return fmt::format("{}{:+}{}{}", color_for(base), V, suffix,
			                   color_for(color::reset));
		}

		std::string color_for(placeholder::color clr,
		                      io::v1::stats const* stats = nullptr) const;
		void add(data_table& table,
		         char type,
		         projection::entry_stats const& stats,
		         std::string_view label,
		         row_type row = row_type::data) const;
		void print_table(std::vector<projection::entry> const& entries) const;
	};
}  // namespace cov::app::show
