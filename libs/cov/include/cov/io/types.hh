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
		FILES = "list"_tag,
		COVERAGE = "lnes"_tag,
		FUNCTIONS = "fnct"_tag,
		BRANCHES = "bran"_tag,
		HILITES = "stxs"_tag,  // highlights list file
		HILITE_TAG = "sntx"_tag,
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

	inline uint32_t clip_u32(size_t raw) {
		constexpr size_t max_u32 = std::numeric_limits<uint32_t>::max();
		return static_cast<uint32_t>(std::min(raw, max_u32));
	}

	inline uint32_t add_u32(uint32_t lhs, uint32_t rhs) {
		constexpr auto max_u32 = std::numeric_limits<uint32_t>::max();
		auto const rest = max_u32 - lhs;
		if (rest < rhs) return max_u32;
		return lhs + rhs;
	}

	inline void inc_u32(uint32_t& rhs) {
		constexpr auto max_u32 = std::numeric_limits<uint32_t>::max();
		if (rhs < max_u32) ++rhs;
	}

	namespace v1 {
		enum : uint32_t {
			VERSION = VERSION_v1_0,
		};

		struct coverage;

		struct coverage_diff {
			int32_t total;
			int32_t relevant;
			int32_t covered;

			bool operator==(coverage_diff const&) const noexcept = default;
		};

		struct coverage_stats {
			uint32_t total;
			uint32_t relevant;
			uint32_t covered;

			static constexpr coverage_stats init() { return {0, 0, 0}; }

			bool operator==(coverage_stats const&) const noexcept = default;

			coverage_stats& operator+=(coverage_stats const& rhs) noexcept {
				total = add_u32(total, rhs.total);
				relevant = add_u32(relevant, rhs.relevant);
				covered = add_u32(covered, rhs.covered);
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

			inline static coverage_stats stats(
			    std::vector<coverage> const& lines) noexcept;

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
				auto out = covered * 100 * divider;

				// round towards the nearest whole
				out += relevant / 2;
				out /= relevant;

				return {static_cast<unsigned>(out / divider),
				        static_cast<unsigned>(out % divider), digits};
			}
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

		inline coverage_diff diff(coverage_stats const& newer,
		                          coverage_stats const& older) {
			return {
			    .total = diff(newer.total, older.total),
			    .relevant = diff(newer.relevant, older.relevant),
			    .covered = diff(newer.covered, older.covered),
			};
		}

		inline coverage_stats::ratio<int> diff(
		    coverage_stats::ratio<> const& newer,
		    coverage_stats::ratio<> const& older) {
			auto const digits = std::max(newer.digits, older.digits);
			auto const multiplier = coverage_stats::pow10(1, digits);

			auto const newer_back =
			    coverage_stats::pow10(newer.whole, newer.digits) +
			    newer.fraction;
			auto const newer_val =
			    coverage_stats::pow10(newer_back, digits - newer.digits);
			auto const older_back =
			    coverage_stats::pow10(older.whole, older.digits) +
			    older.fraction;
			auto const older_val =
			    coverage_stats::pow10(older_back, digits - older.digits);

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

		/*
		    REPORT:
		        file_header:
		            0:1: "rprt" (tag)
		            1:1: 1.0 (version)
		        report:
		            2:5: parent report (oid)
		            7:5: file list (oid)
		            12:2: added (timestamp)
		            coverage_stats:
		                14:1: total (uint)
		                15:1: relevant (uint)
		                16:1: covered (uint)
		            report_commit:
		                17:1: branch (str)
		                18:2: author (report_email)
		                20:2: committer (report_email)
		                22:1: message (str)
		                23:5: commit_id (oid)
		                28:2: committed (timestamp)
		            30:1: strings_offset (uint) := SO
		            31:1: strings_size (uint) := SIZE
		            32 + SO:SIZE: UTF8Z (like ASCIIZ, but UTF-8)
		*/

		struct report_email {
			uint32_t name;
			uint32_t email;
		};

		struct report_commit {
			uint32_t branch;
			report_email author;
			report_email committer;
			uint32_t message;
			git_oid commit_id;
			timestamp committed;
		};

		static_assert(sizeof(report_commit) == sizeof(uint32_t[13]),
		              "git_oid and/or header email does not pack well here");

		struct report {
			git_oid parent_report;
			git_oid file_list;
			timestamp added;
			coverage_stats stats;
			report_commit commit;
			uint32_t strings_offset;
			uint32_t strings_size;
		};

		static_assert(sizeof(report) == sizeof(uint32_t[30]),
		              "report_header does not pack well here");

		/*
		    REPORT FILES:
		        file_header:
		            0:1: "list" (tag)
		            1:1: 1.x (version)
		        report_files:
		            21:1: strings_offset (uint) := SO
		            22:1: strings_size (uint) := SIZE
		            23:1: entries_offset (uint) := EO
		            24:1: entries_count (uint) := EC
		            25:1: entry_size (uint) := ES
		            26 + SO:SIZE: UTF8Z (like ASCIIZ, but UTF-8)
		            26 + EO:ES: entry (report_entry x EC)

		        report_entry:
		            v1.0:
		                0:31-0: path (str)
		                coverage_stats:
		                    1:1: total (uint)
		                    2:1: relevant (uint)
		                    3:1: covered (uint)
		                4:5: contents (oid)
		                9:5: line_coverage (oid)
		            v1.1:
		                14:5: function_coverage (oid)
		                19:5: branch_coverage (oid)
		*/

		struct report_files {
			uint32_t strings_offset;  // all offsets in u32s, from the end of
			                          // header
			uint32_t strings_size;
			uint32_t entries_offset;
			uint32_t entries_count;
			uint32_t entry_size;
		};

		static_assert(sizeof(report_files) == sizeof(uint32_t[5]),
		              "report_files does not pack well here");

		struct report_entry_base {
			uint32_t path;
			coverage_stats stats;
			git_oid contents;
			git_oid line_coverage;
		};

		static_assert(sizeof(report_entry_base) == sizeof(uint32_t[14]),
		              "git_oid does not pack well here");

		struct report_entry_ext {
			uint32_t path;
			coverage_stats stats;
			git_oid contents;
			git_oid line_coverage;
			git_oid function_coverage;
			git_oid branch_coverage;
		};

		static_assert(sizeof(report_entry_ext) == sizeof(uint32_t[24]),
		              "git_oid does not pack well here");

		/*
		    LINE COVERAGE:
		        file_header:
		            0:1: "lnes" (tag)
		            1:1: 1.0 (version)
		        line_coverage:
		            2:1: line_count (uint) := LC
		            3:LC*1: coverage
		                31-31: is_null (flag)
		                30-0: value (uint)
		*/

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
				total = add_u32(total, rhs.value);
				return *this;
			}
			inc_u32(total);
			inc_u32(relevant);
			if (rhs.value) inc_u32(covered);
			return *this;
		}

		inline coverage_stats coverage_stats::stats(
		    std::vector<coverage> const& lines) noexcept {
			return std::accumulate(lines.begin(), lines.end(),
			                       coverage_stats{});
		}

	};  // namespace v1
}  // namespace cov::io

namespace fmt {
	template <typename Int>
	struct formatter<cov::io::v1::coverage_stats::ratio<Int>> {
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
		FMT_CONSTEXPR auto format(
		    cov::io::v1::coverage_stats::ratio<Int> const& ratio,
		    FormatContext& ctx) const -> decltype(ctx.out()) {
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
