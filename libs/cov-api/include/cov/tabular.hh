// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <fmt/format.h>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cov {
	template <typename Iterator>
	inline Iterator aligned_format_to(Iterator it,
	                                  std::string_view label,
	                                  size_t width,
	                                  char align = '<') {
		switch (align) {
			default:
			case '<':
				return fmt::format_to(it, "{:<{}}", label, width);
			case '>':
				return fmt::format_to(it, "{:>{}}", label, width);
			case '^':
				return fmt::format_to(it, "{:^{}}", label, width);
		}
	}

	struct inv_t {
		size_t escape_seq;
		size_t utf8_tail;
	};

	inv_t count_invisible(std::string_view label);

	struct data_cell {
		std::string value{};
		inv_t invisible{count_invisible(value)};

		data_cell() = default;
		data_cell(std::string const& label) : value{label} {};
		data_cell(std::string&& label) : value{std::move(label)} {};
	};

	struct span_t {
		size_t len{};

		explicit constexpr span_t(size_t len) : len{len} {}
		constexpr span_t() = default;
		constexpr span_t(const span_t&) = default;
		constexpr span_t& operator=(const span_t&) = default;
	};
	template <size_t span_len>
	struct col_span : span_t {
		consteval col_span() : span_t{span_len} {}
	};
	struct table_column {
		std::string label;
		size_t span{2};
		char align{'^'};
		table_column() = default;
		table_column(std::string&& label) : label{std::move(label)} {}
		explicit table_column(std::string const& label) : label{label} {}
		table_column(std::string&& label, span_t span, char align = '^')
		    : label{std::move(label)}, span{span.len}, align{align} {}
		table_column(std::string const& label, span_t span, char align = '^')
		    : label{label}, span{span.len}, align{align} {}
		table_column(std::string&& label, char align)
		    : label{std::move(label)}, align{align} {}
		table_column(std::string const& label, char align)
		    : label{label}, align{align} {}
	};

	struct data_table {
		std::vector<table_column> header{};
		std::vector<std::vector<data_cell>> rows{};
		std::vector<std::vector<data_cell>> footer{};
		std::vector<size_t> widths{};

		void emplace(std::vector<std::string>&& row, bool is_data = true);
		void print(FILE* f = stdout) const;
		std::string format() const;

		template <typename Iterator>
		void format_to(Iterator it) const {
			auto col_widths = calc_headers();

			if (!header.empty()) {
				it = sep_format_to(it, col_widths);

				auto width_it = col_widths.begin();
				for (auto const& item : header) {
					auto const& col = *width_it++;
					*it++ = '|';
					*it++ = ' ';
					it = aligned_format_to(it, item.label,
					                       col.col_widths + col.header_inv,
					                       item.align);
					*it++ = ' ';
				}
				*it++ = '|';
				*it++ = '\n';
			}

			it = sep_format_to(it, col_widths);

			for (auto const& row : rows)
				it = row_format_to(it, row, col_widths);

			if (!footer.empty()) {
				if (!rows.empty()) it = sep_format_to(it, col_widths);

				for (auto const& row : footer)
					it = row_format_to(it, row, col_widths);
			}

			it = sep_format_to(it, col_widths);
		}

	private:
		struct header_sizes {
			size_t start_col{};
			size_t first_width{};
			size_t col_widths{};
			size_t header_inv{};
		};

		std::vector<header_sizes> calc_headers() const;

		template <typename Iterator>
		static Iterator sep_format_to(
		    Iterator it,
		    std::vector<header_sizes> const& col_widths) {
			for (auto const& col : col_widths) {
				it = fmt::format_to(it, "+{:-<{}}", "", col.col_widths + 2);
			}
			*it++ = '+';
			*it++ = '\n';
			return it;
		}

		template <typename Iterator>
		Iterator row_format_to(
		    Iterator it,
		    std::vector<data_cell> const& row,
		    std::vector<header_sizes> const& col_widths) const {
			auto has_pipe = false;
			auto width_it = col_widths.begin();

			for (size_t index = 0; index < row.size(); ++index) {
				if (!widths[index]) continue;

				if (index) *it++ = ' ';

				auto width = widths[index];
				auto align = '<';
				if (width_it != col_widths.end() &&
				    width_it->start_col == index) {
					has_pipe = true;
					width = width_it->first_width;
					align = '>';

					*it++ = '|';
					*it++ = ' ';
					++width_it;
				}
				it = aligned_format_to(it, row[index].value,
				                       width + row[index].invisible.escape_seq,
				                       align);
			}
			if (has_pipe) {
				*it++ = ' ';
				*it++ = '|';
			}
			*it++ = '\n';
			return it;
		}
	};
}  // namespace cov
