// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"
#endif
#include <date/date.h>
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
#include <fmt/format.h>
#include <git2/oid.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <vector>

namespace cov {
	using date::sys_seconds;
	using std::size_t;
	using std::uint32_t;
	using std::chrono::seconds;

	constexpr inline uint32_t MK_TAG(char c1, char c2, char c3, char c4) {
		return ((static_cast<uint32_t>(c1) & 0xFF)) |
		       ((static_cast<uint32_t>(c2) & 0xFF) << 8) |
		       ((static_cast<uint32_t>(c3) & 0xFF) << 16) |
		       ((static_cast<uint32_t>(c4) & 0xFF) << 24);
	}
	consteval inline uint32_t operator""_tag(const char* s, size_t len) {
		return MK_TAG(len > 0 ? s[0] : ' ', len > 1 ? s[1] : ' ',
		              len > 2 ? s[2] : ' ', len > 3 ? s[3] : ' ');
	}
}  // namespace cov

namespace cov::io {
	enum class OBJECT : uint32_t {
		REPORT = "rprt"_tag,
		BUILD = "bld"_tag,
		FILES = "list"_tag,
		COVERAGE = "lnes"_tag,
		FUNCTIONS = "fnct"_tag,
		BRANCHES = "bran"_tag,
	};

	enum : uint32_t {
		VERSION_MAJOR = 0xFFFF'0000,
		VERSION_MINOR = 0x0000'FFFF,
		VERSION_v1_0 = 0x0001'0000,
		VERSION_v1_1 = 0x0001'0001
	};

	struct file_header {
		uint32_t magic;
		uint32_t version;
	};

	struct timestamp {
		uint32_t hi;
		uint32_t lo;
		// GCOV_EXCL_START[Clang]
		[[deprecated("use operator=(sys_seconds)")]] timestamp& operator=(
		    uint64_t time) noexcept {
			hi = static_cast<uint32_t>((time >> 32) & 0xFFFF'FFFF);
			lo = static_cast<uint32_t>((time)&0xFFFF'FFFF);
			return *this;
		}
		// GCOV_EXCL_STOP
		timestamp& operator=(sys_seconds seconds) noexcept {
			auto const time =
			    static_cast<uint64_t>(seconds.time_since_epoch().count());
			hi = static_cast<uint32_t>((time >> 32) & 0xFFFF'FFFF);
			lo = static_cast<uint32_t>((time)&0xFFFF'FFFF);
			return *this;
		}
		// GCOV_EXCL_START[Clang]
		[[deprecated("use to_seconds()")]] uint64_t to_time_t() const noexcept {
			return (static_cast<uint64_t>(hi) << 32) |
			       static_cast<uint64_t>(lo);
		}
		// GCOV_EXCL_STOP
		sys_seconds to_seconds() const noexcept {
			return sys_seconds{seconds{(static_cast<uint64_t>(hi) << 32) |
			                           static_cast<uint64_t>(lo)}};
		}
	};

	inline constexpr uint32_t clip_u32(size_t raw) {
		constexpr size_t max_u32 = std::numeric_limits<uint32_t>::max();
		return static_cast<uint32_t>(std::min(raw, max_u32));
	}

	inline constexpr uint32_t add_u32(uint32_t lhs, uint32_t rhs) {
		constexpr auto max_u32 = std::numeric_limits<uint32_t>::max();
		auto const rest = max_u32 - lhs;
		if (rest < rhs) return max_u32;
		return lhs + rhs;
	}

	inline constexpr void inc_u32(uint32_t& rhs) {
		constexpr auto max_u32 = std::numeric_limits<uint32_t>::max();
		if (rhs < max_u32) ++rhs;
	}

	enum class str : uint32_t;

	struct block {
		uint32_t offset;
		uint32_t size;
	};

	struct array_ref {
		uint32_t offset;
		uint32_t size;
		uint32_t count;

		template <typename Entry>
		static array_ref from(size_t byte_offset, size_t count) noexcept {
			static const auto u32s = [](size_t bytes) {
				return static_cast<uint32_t>(bytes / sizeof(uint32_t));
			};
			return {.offset = u32s(byte_offset),
			        .size = u32s(sizeof(Entry)),
			        .count = clip_u32(count)};
		}
	};

	namespace v1 {
		enum : uint32_t {
			VERSION = VERSION_v1_0,
		};

		struct coverage;

		struct stats_diff {
			int32_t relevant;
			int32_t visited;

			bool operator==(stats_diff const&) const noexcept = default;
		};

		struct stats {
			uint32_t relevant;
			uint32_t visited;

			static constexpr stats init() noexcept { return {0, 0}; }
			constexpr bool operator==(stats const&) const noexcept = default;

			constexpr stats& operator+=(stats const& rhs) noexcept {
				relevant = add_u32(relevant, rhs.relevant);
				visited = add_u32(visited, rhs.visited);
				return *this;
			}

			constexpr stats operator+(stats const& rhs) const noexcept {
				auto tmp = stats{*this};
				tmp += rhs;
				return tmp;
			}

			template <typename Int>
			using Ordering =
			    decltype(std::declval<Int>() <=> std::declval<Int>());
			template <typename Int>
			using IntMax = std::conditional_t<std::is_signed_v<Int>,
			                                  std::intmax_t,
			                                  std::uintmax_t>;

			template <typename Int = unsigned>
			struct ratio {
				Int whole;
				Int fraction;
				unsigned digits;

				auto operator<=>(ratio const& rhs) const noexcept {
					using intmax = IntMax<Int>;
					using ordering =
					    std::common_type_t<Ordering<Int>, Ordering<intmax>>;

					if (auto const cmp =
					        static_cast<ordering>(whole <=> rhs.whole);
					    cmp != 0)
						return cmp;

					if constexpr (std::is_signed_v<Int>) {
						if ((fraction < 0) != (rhs.fraction < 0))
							return static_cast<ordering>(fraction <=>
							                             rhs.fraction);
					}

					auto lhs_fraction = static_cast<intmax>(fraction);
					auto rhs_fraction = static_cast<intmax>(rhs.fraction);
					if (digits < rhs.digits) {
						auto const level_diff =
						    static_cast<intmax>(pow10(1, rhs.digits - digits));
						lhs_fraction *= level_diff;
					} else if (rhs.digits < digits) {
						auto const level_diff =  // GCOV_EXCL_LINE[GCC]
						    static_cast<intmax>(pow10(1, digits - rhs.digits));
						rhs_fraction *= level_diff;
					}

					return static_cast<ordering>(lhs_fraction <=> rhs_fraction);
				}

				bool operator==(ratio const& rhs) const noexcept {
					using intmax = IntMax<Int>;
					if (whole != rhs.whole) return false;

					if constexpr (std::is_signed_v<Int>) {
						if ((fraction < 0) != (rhs.fraction < 0)) return false;
					}

					auto lhs_fraction = static_cast<intmax>(fraction);
					auto rhs_fraction = static_cast<intmax>(rhs.fraction);
					if (digits < rhs.digits) {
						auto const level_diff =
						    static_cast<intmax>(pow10(1, rhs.digits - digits));
						lhs_fraction *= level_diff;
					} else if (rhs.digits < digits) {
						auto const level_diff =  // GCOV_EXCL_LINE[GCC]
						    static_cast<intmax>(pow10(1, digits - rhs.digits));
						rhs_fraction *= level_diff;
					}

					return lhs_fraction == rhs_fraction;
				}
			};

			static constexpr std::uintmax_t pow10(std::uintmax_t value,
			                                      unsigned digits) noexcept {
				if (!value) return value;
				while (digits) {
					value *= 10;
					--digits;
				}
				return value;
			}

			constexpr ratio<> calc(unsigned char digits = 0) const noexcept {
				if (!relevant) return {0, 0, digits};

				auto const divider = pow10(1, digits);
				auto out = visited * 100 * divider;

				// round towards the nearest whole
				out += relevant / 2;
				out /= relevant;

				return {static_cast<unsigned>(out / divider),
				        static_cast<unsigned>(out % divider), digits};
			}
		};

		struct coverage_diff {
			int32_t lines_total;
			stats_diff lines;
			stats_diff functions;
			stats_diff branches;

			bool operator==(coverage_diff const&) const noexcept = default;
		};

		struct coverage_stats {
			uint32_t lines_total;
			stats lines;
			stats functions;
			stats branches;

			static constexpr coverage_stats init() noexcept {
				return {0, stats::init(), stats::init(), stats::init()};
			}

			bool operator==(coverage_stats const&) const noexcept = default;

			coverage_stats& operator+=(coverage_stats const& rhs) noexcept {
				lines_total = add_u32(lines_total, rhs.lines_total);
				lines += rhs.lines;
				functions += rhs.functions;
				branches += rhs.branches;
				return *this;
			}

			coverage_stats operator+(coverage_stats const& rhs) const noexcept {
				auto tmp = coverage_stats{*this};
				tmp += rhs;
				return tmp;
			}

			inline coverage_stats& operator+=(coverage const& rhs) noexcept;
			coverage_stats operator+(coverage const& rhs) const noexcept {
				auto tmp = coverage_stats{*this};
				tmp += rhs;
				return tmp;
			}

			inline static coverage_stats from(
			    std::vector<coverage> const& lines) noexcept;
		};

		inline auto diff(std::unsigned_integral auto newer,
		                 std::unsigned_integral auto older) {
			using Unsigned =
			    std::common_type_t<decltype(newer), decltype(older)>;
			using Signed = std::make_signed_t<Unsigned>;
			static constexpr auto ceil =
			    static_cast<Unsigned>(std::numeric_limits<Signed>::max());
			auto const neg = newer < older;
			auto const abs = static_cast<Signed>(
			    std::min(neg ? older - newer : newer - older, ceil));
			return neg ? -abs : abs;
		}

		inline stats_diff diff(stats const& newer, stats const& older) {
			return {
			    .relevant = diff(newer.relevant, older.relevant),
			    .visited = diff(newer.visited, older.visited),
			};
		}

		inline coverage_diff diff(coverage_stats const& newer,
		                          coverage_stats const& older) {
			return {
			    .lines_total = diff(newer.lines_total, older.lines_total),
			    .lines = diff(newer.lines, older.lines),
			    .functions = diff(newer.functions, older.functions),
			    .branches = diff(newer.branches, older.branches),
			};
		}

		inline stats::ratio<int> diff(stats::ratio<> const& newer,
		                              stats::ratio<> const& older) {
			auto const digits = std::max(newer.digits, older.digits);
			auto const multiplier = stats::pow10(1, digits);

			auto const newer_back =
			    stats::pow10(newer.whole, newer.digits) + newer.fraction;
			auto const newer_val =
			    stats::pow10(newer_back, digits - newer.digits);
			auto const older_back =
			    stats::pow10(older.whole, older.digits) + older.fraction;
			auto const older_val =
			    stats::pow10(older_back, digits - older.digits);

			static constexpr auto ceil = static_cast<std::uintmax_t>(
			    std::numeric_limits<std::intmax_t>::max());
			auto const neg = newer_val < older_val;
			auto const abs = std::min(
			    neg ? older_val - newer_val : newer_val - older_val, ceil);

			auto const ret = static_cast<int>(abs / multiplier);
			auto const fraction = static_cast<int>(abs % multiplier);
			return {neg ? -ret : ret, neg && !ret ? -fraction : fraction,
			        digits};
		}

		struct email_ref {
			str name;
			str email;
		};

		struct report {
			struct commit {
				str branch;
				email_ref author;
				email_ref committer;
				str message;
				git_oid commit_id;
				timestamp committed;
			};

			struct entry {
				git_oid build;
				str propset;
				coverage_stats stats;
			};

			block strings;
			git_oid parent;
			git_oid file_list;
			array_ref builds;
			timestamp added;
			commit git;
			coverage_stats stats;
		};

		static_assert(sizeof(report) == sizeof(uint32_t[37]),
		              "report does not pack well here");

		static_assert(sizeof(report::commit) == sizeof(uint32_t[13]),
		              "report::commit does not pack well here");

		static_assert(sizeof(report::entry) == sizeof(uint32_t[13]),
		              "report::entry does not pack well here");

		struct build {
			block strings;
			git_oid file_list;
			timestamp added;
			str propset;
			coverage_stats stats;
		};

		static_assert(sizeof(build) == sizeof(uint32_t[17]),
		              "build does not pack well here");

		struct report_entry_stats {
			stats summary;
			git_oid details;
		};
#define FILES_ENTRY_BASIC \
	str path;             \
	git_oid contents;     \
	uint32_t lines_total; \
	report_entry_stats lines

		struct files {
			struct basic {
				FILES_ENTRY_BASIC;
			};

			struct ext {
				FILES_ENTRY_BASIC;
				report_entry_stats functions;
				report_entry_stats branches;
			};

			block strings;
			array_ref entries;
		};

		static_assert(sizeof(files) == sizeof(uint32_t[5]),
		              "files does not pack well here");

		static_assert(sizeof(files::basic) == sizeof(uint32_t[14]),
		              "files::basic does not pack well here");

		static_assert(sizeof(files::ext) == sizeof(uint32_t[28]),
		              "files::ext does not pack well here");

		struct line_coverage {
			uint32_t line_count;
		};

		struct coverage {
			uint32_t value : 31;
			uint32_t is_null : 1;
			bool operator==(coverage const&) const noexcept = default;
		};

		inline coverage_stats& coverage_stats::operator+=(
		    coverage const& rhs) noexcept {
			if (rhs.is_null) {
				lines_total = add_u32(lines_total, rhs.value);
				return *this;
			}
			inc_u32(lines_total);
			inc_u32(lines.relevant);
			if (rhs.value) inc_u32(lines.visited);
			return *this;
		}

		inline coverage_stats coverage_stats::from(
		    std::vector<coverage> const& lines) noexcept {
			return std::accumulate(lines.begin(), lines.end(),
			                       coverage_stats::init());
		}

		struct text_pos {
			uint32_t line;
			uint32_t column;
			bool operator==(text_pos const&) const noexcept = default;
		};

		struct function_coverage {
			block strings;
			array_ref entries;

			struct entry {
				str name;
				str demangled_name;
				uint32_t count;
				text_pos start;
				text_pos end;
			};
		};
	};  // namespace v1
}  // namespace cov::io

namespace fmt {
	template <typename Int>
	struct formatter<cov::io::v1::stats::ratio<Int>> {
		bool with_sign{false};

		FMT_CONSTEXPR auto parse(format_parse_context& ctx)
		    -> decltype(ctx.begin()) {
			auto it = ctx.begin(), end = ctx.end();
			if (it != end && (*it == '+' || *it == '-')) {
				with_sign = *it++ == '+';
			}

			while (it != end && *it != '}')
				++it;
			return it;
		}

		template <typename FormatContext>
		FMT_CONSTEXPR auto format(cov::io::v1::stats::ratio<Int> const& ratio,
		                          FormatContext& ctx) const
		    -> decltype(ctx.out()) {
			if constexpr (std::signed_integral<Int>) {
				if (!ratio.whole && ratio.fraction < 0)
					return fmt::format_to(ctx.out(), "-0.{:0{}}",
					                      -ratio.fraction, ratio.digits);
				if (with_sign) {
					return fmt::format_to(
					    ctx.out(), "{}{}.{:0{}}", ratio.whole < 0 ? '-' : '+',
					    ratio.whole < 0 ? -ratio.whole : ratio.whole,
					    ratio.fraction, ratio.digits);
				}
			} else {
				if (with_sign) {
					return fmt::format_to(ctx.out(), "+{}.{:0{}}", ratio.whole,
					                      ratio.fraction, ratio.digits);
				}
			}
			return fmt::format_to(ctx.out(), "{}.{:0{}}", ratio.whole,
			                      ratio.fraction, ratio.digits);
		}
	};
}  // namespace fmt
