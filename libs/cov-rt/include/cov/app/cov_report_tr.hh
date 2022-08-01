// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/app/strings/cov_report.hh>
#include <cov/app/tr.hh>

namespace cov::app {
	using reportlng = str::cov_report::faulty;
	using ReportStrings = str::cov_report::Strings;

	template <>
	struct lngs_traits<reportlng>
	    : base_lngs_traits<reportlng, "cov_report", ReportStrings> {};
}  // namespace cov::app
