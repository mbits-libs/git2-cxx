// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)
//
// IT IS NATURAL FOR TEST IN HERE TO FAIL FROM TIME TO TIME. IF THEY SWITCH FROM
// SUCCEEDING TO FAILING, A TEST NEEDS TO BE UPDATED TO REFLECT CHANGE IN THE
// REST OF THE CODE.

#pragma once

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <cov/app/dirs.hh>
#include <memory>
#include <native/platform.hh>
#include <string_view>
#include <web/mstch_cache.hh>

using namespace std::literals;

namespace cov::app::web::testing {
	struct counting_lngs : lng_provider {
#define X(LNG) mutable unsigned LNG##_counter_{1};
		MSTCH_LNG(X)
#undef X
#define X(LNG) mutable std::string LNG##_storage_{};
		MSTCH_LNG(X)
#undef X

		std::string_view get(lng id) const noexcept override {
			try {
				switch (id) {
#define X(LNG)                                                         \
	case lng::LNG:                                                     \
		LNG##_storage_ = fmt::format("{} ({})", #LNG, LNG##_counter_); \
		++LNG##_counter_;                                              \
		return LNG##_storage_;
					MSTCH_LNG(X)
#undef X
				}
			} catch (...) {
			}
			return ""sv;
		}
	};

	struct tests : platform::sys_provider<tests> {
		static std::filesystem::path const& sys_root() {
			// <sys-root>/bin/web-tests ->  <sys-root>/bin/.. -> <sys-root>
			static auto const root =
			    platform::exec_path().parent_path().parent_path();

			return root;
		}

		static std::shared_ptr<mstch::callback> octicons() {
			static std::shared_ptr<mstch::callback> result =
			    octicon_callback::create(sys_root() / directory_info::site_res /
			                             "octicons.json"sv);
			return result;
		}

		static std::shared_ptr<mstch::callback> lng_callback() {
			static counting_lngs provider{};
			static std::shared_ptr<mstch::callback> result =
			    lng_callback::create(&provider);
			return result;
		}
	};
}  // namespace cov::app::web::testing
