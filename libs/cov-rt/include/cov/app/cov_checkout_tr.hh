// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/app/strings/cov_checkout.hh>
#include <cov/app/tr.hh>

namespace cov::app {
	using co_lng = str::cov_checkout::lng;
	using CheckoutStrings = str::cov_checkout::Strings;

	template <>
	struct lngs_traits<co_lng>
	    : base_lngs_traits<co_lng, "cov_checkout", CheckoutStrings> {};
}  // namespace cov::app
