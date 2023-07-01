// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4189)
#endif
#include <fmt/format.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <cov/tabular.hh>

namespace cov {
	namespace {
		enum class state { raw, esc, csi };

		void vformat_to(fmt::detail::buffer<char>& buf,
		                data_table const& table) {
			using fmt::detail::buffer_appender;
			using fmt::detail::default_arg_formatter;
			auto out = buffer_appender<char>(buf);
			table.format_to(out);
		}

	}  // namespace

	inv_t count_invisible(std::string_view label) {
		static constexpr unsigned char MASK = 0b1100'0000;
		static constexpr unsigned char CONT = 0b1000'0000;
		size_t escape_seq{}, utf8_tail{};
		auto st = state::raw;
		for (auto c : label) {
			switch (st) {
				case state::raw:
					if (c == 0x1B) st = state::esc;
					if ((static_cast<unsigned char>(c) & MASK) == CONT)
						++utf8_tail;
					break;
				case state::esc:
					if (c == '[') {
						st = state::csi;
						escape_seq += 2;
					}
					break;
				case state::csi:
					++escape_seq;
					if (c == 'm') st = state::raw;
					break;
			}
		}
		return {.escape_seq = escape_seq, .utf8_tail = utf8_tail};
	}

	void data_table::emplace(std::vector<std::string>&& row, bool is_data) {
		if (widths.size() < row.size()) {
			widths.resize(row.size());
		}

		std::vector<data_cell> cells{row.size()};
		auto it = widths.begin();
		auto dst_row = cells.begin();

		for (auto& data : row) {
			auto& prev = *it++;
			auto& dst = *dst_row++;
			dst = std::move(data);
			prev = std::max(prev, dst.value.size() - (dst.invisible.escape_seq +
			                                          dst.invisible.utf8_tail));
		}

		(is_data ? rows : footer).emplace_back(std::move(cells));
	}

	void data_table::print(FILE* f) const {
		using namespace fmt;
		auto buffer = memory_buffer{};
		vformat_to(buffer, *this);
		if (detail::is_utf8()) {
			detail::print(f, string_view{buffer.data(), buffer.size()});
		} else {
			// GCOV_EXCL_START
			buffer.push_back(0);
			std::fputs(buffer.data(), f);
			// GCOV_EXCL_STOP
		}
	}

	std::string data_table::format() const {
		auto buffer = fmt::memory_buffer{};
		vformat_to(buffer, *this);
		return to_string(buffer);
	}

	std::vector<data_table::header_sizes> data_table::calc_headers() const {
		std::vector<header_sizes> col_widths;

		if (header.empty()) return col_widths;

		size_t min_col = 0;
		col_widths.reserve(header.size());
		for (auto const& item : header) {
			auto const item_width = item.label.length();
			auto const [inv, utf8_tail] = count_invisible(item.label);
			auto const vis = item_width - inv;

			size_t col_width = 0;
			size_t first_width = 0;
			for (size_t span_index = 0; span_index < item.span; ++span_index) {
				auto const index = min_col + span_index;
				if (index >= widths.size()) break;
				if (col_width && widths[index]) ++col_width;
				col_width += widths[index];
				if (!first_width) first_width = widths[index];
			}
			auto const col_width_ = std::max(col_width, vis);

			first_width += col_width_ - col_width;
			col_widths.push_back({.start_col = min_col,
			                      .first_width = first_width,
			                      .col_widths = col_width_,
			                      .header_inv = inv});
			min_col += item.span;
		}

		return col_widths;
	}
}  // namespace cov
