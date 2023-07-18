// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/app/strings/log_format.hh>
#include <cov/app/tr.hh>
#include <cov/format.hh>
#include <string>

namespace cov::app {
	using fmtlng = str::log_format::lng;
	using LogFormatStrings = str::log_format::Strings;

	template <>
	struct lngs_traits<fmtlng>
	    : base_lngs_traits<fmtlng, "log_format", LogFormatStrings> {};

	template <typename... Lambda>
	struct overloaded : Lambda... {
		using Lambda::operator()...;
	};
	template <class... Lambda>
	overloaded(Lambda...) -> overloaded<Lambda...>;

	template <typename Final>
	struct format_str {
		void attach_to(cov::placeholder::environment& env) noexcept {
			env.app = static_cast<Final*>(this);
			env.translate = translate;
		}

	private:
		static std::string translate(long long count,
		                             translatable scale,
		                             void* app) {
			using counted = str::log_format::counted;
			auto const& impl = *reinterpret_cast<Final const*>(app);
			auto const& tr = impl.tr();

			auto const str =
			    overloaded{[&tr](fmtlng id) -> std::string {
				               auto const view = tr(id);
				               return {view.data(), view.size()};
			               },
			               [&tr](counted id, long long count) -> std::string {
				               auto const view = tr(id, count);
				               return fmt::format(fmt::runtime(view), count);
			               }};

			switch (scale) {
				case translatable::in_the_future:
					return str(fmtlng::IN_THE_FUTURE);
				case translatable::seconds_ago:
					return str(counted::SECONDS_AGO, count);
				case translatable::minutes_ago:
					return str(counted::MINUTES_AGO, count);
				case translatable::hours_ago:
					return str(counted::HOURS_AGO, count);
				case translatable::days_ago:
					return str(counted::DAYS_AGO, count);
				case translatable::weeks_ago:
					return str(counted::WEEKS_AGO, count);
				case translatable::months_ago:
					return str(counted::MONTHS_AGO, count);
				case translatable::years_months_ago:
					return str(counted::YEARS_MONTHS_AGO, count);
				case translatable::years_ago:
					return str(counted::YEARS_AGO, count);
				case translatable::mark_failing:
					return str(fmtlng::MARK_FAILING);
				case translatable::mark_incomplete:
					return str(fmtlng::MARK_INCOMPLETE);
				case translatable::mark_passing:
					return str(fmtlng::MARK_PASSING);
			}
			// all enums are handled above
			return std::to_string(count);  // GCOV_EXCL_LINE
		}
	};

	struct simple_format_str : format_str<simple_format_str>, LogFormatStrings {
		LogFormatStrings const& tr() const noexcept { return *this; }
	};
}  // namespace cov::app
