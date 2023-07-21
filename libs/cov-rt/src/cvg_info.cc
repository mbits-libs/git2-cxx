
// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <cov/app/cvg_info.hh>
#include <cov/app/line_printer.hh>

using namespace std::literals;

namespace cov::app {
	using namespace lighter;

	inline size_t length_of(highlighted_line const& items) {
		if (items.empty()) return 0u;
		return items.back().furthest_end();
	}

	size_t column_width(auto num) {
		if (!num) return 1;
		size_t result = 0;
		while (num) {
			++result;
			num /= 10;
		}
		return result;
	}

	cvg_info cvg_info::from_coverage(
	    std::span<io::v1::coverage const> const& lines) {
		cvg_info result{};
		{
			unsigned line = 0;
			for (auto&& cvg : lines) {
				if (cvg.is_null) {
					line += cvg.value;
					continue;
				}
				line++;
				result.coverage[line] = cvg.value;
			}
		}

		return result;
	}  // GCOV_EXCL_LINE[GCC]

	void cvg_info::add_functions(
	    std::span<std::unique_ptr<cov::function_coverage::entry> const> input) {
		for (auto const& entry : input) {
			auto name = entry->demangled_name();
			if (name.empty()) name = entry->name();
			functions.push_back({
			    .label = {.start = entry->start().line,
			              .end = entry->end().line},
			    .count = entry->count(),
			});
			functions.back().label.name.assign(name);
		}

		std::sort(functions.begin(), functions.end());
		auto dst = functions.begin();
		auto end = functions.end();
		if (dst == end) return;
		for (auto src = std::next(dst); src != end;) {
			if (src->label != dst->label) {
				++dst;
				++src;
				continue;
			}
			dst->count += src->count;
			src = functions.erase(src);
			end = functions.end();
		}
	}

	void cvg_info::find_chunks() {
		chunks.clear();

		auto fn_it = functions.begin();
		auto const fn_end = functions.end();

		for (auto [line, count] : coverage) {
			if (count) {
				bool covered = true;

				while (fn_it != fn_end && fn_it->label.start < line)
					++fn_it;
				while (fn_it != fn_end && fn_it->label.start == line) {
					auto const& function = *fn_it++;
					if (!function.count) {
						covered = false;
						break;
					}
				}
				if (covered) continue;
			}
			auto const start = line < 3 ? 0u : line - 3;
			auto const stop = line + 3;
			if (chunks.empty() || chunks.back().second + 3 < start) {
				chunks.push_back({start, stop});
				continue;
			}
			chunks.back().second = stop;
		}
	}

	void cvg_info::load_syntax(std::string_view text,
	                           std::string_view filename) {
		file_text = text;
		syntax = lighter::highlights::from(text, filename);
		if (!syntax.lines.empty() && syntax.lines.back().contents.empty()) {
			syntax.lines.pop_back();
		}

		if (chunks.empty())
			chunks.push_back({0, static_cast<unsigned>(syntax.lines.size())});
	}

	std::optional<unsigned> cvg_info::max_count() const noexcept {
		std::optional<unsigned> result{};

		for (auto const& [start, stop] : chunks) {
			for (auto line_no = start; line_no <= stop; ++line_no) {
				auto it = coverage.find(line_no);
				if (it != coverage.end())
					result =
					    result ? std::max(*result, it->second) : it->second;
			}
		}

		for (auto const& fun : functions) {
			result = result ? std::max(*result, fun.count) : fun.count;
		}

		return result;
	}

	view_columns cvg_info::column_widths() const noexcept {
		auto max = max_count();
		return {
		    .line_no_width = column_width(chunks.back().second + 1),
		    .count_width = max ? column_width(*max) : 0,
		};
	}

	std::optional<unsigned> cvg_info::count_for(
	    unsigned line_no) const noexcept {
		std::optional<unsigned> count = std::nullopt;
		auto it = coverage.find(line_no);
		if (it != coverage.end()) count = it->second;
		return count;
	}

	std::string cvg_info::to_string(function const& fn,
	                                view_columns const& widths,
	                                bool use_color,
	                                size_t max_width) const {
		if (max_width < std::numeric_limits<std::size_t>::max()) {
			static constexpr size_t MAGIC = 40;
			static constexpr size_t margins = 8;
			max_width -= std::min(max_width, widths.count_width);
			max_width -= std::min(max_width, widths.line_no_width);
			max_width -= std::min(max_width, margins);
			max_width = std::max(max_width, MAGIC);
		}

		std::string_view name = fn.label.name;
		std::string backing{};

		auto const shorten = name.size() > max_width;
		if (shorten) {
			backing.assign(name.substr(0, max_width - 3));
			name = backing;
		}

		auto const count_column = fmt::format("{}x", fn.count);
		auto prefix = ""sv, suffix = ""sv;
		if (use_color) {
			prefix = "\033[2;49;39m"sv;
			suffix = "\033[m"sv;
		}
		return fmt::format(
		    "{} {:>{}} |{} {}", prefix, count_column,
		    widths.line_no_width + 3 + widths.count_width + 1, suffix,
		    line_printer::to_string(fn.count, name, shorten, use_color));
	}

	std::string cvg_info::to_string(size_t line_no,
	                                view_columns const& widths,
	                                bool use_color) const {
		auto const count = count_for(static_cast<unsigned>(line_no));
		auto const& line = syntax.lines[line_no];
		auto const length = length_of(line.contents);
		auto const line_text = file_text.substr(line.start, length);

		auto const count_column = count ? fmt::format("{}x", *count) : " "s;
		return fmt::format(
		    " {:>{}} | {:>{}} | {}", line_no + 1, widths.line_no_width,
		    count_column, widths.count_width + 1,
		    line_printer::to_string(count, line_text, line.contents,
		                            syntax.dict, use_color));
	}
}  // namespace cov::app
