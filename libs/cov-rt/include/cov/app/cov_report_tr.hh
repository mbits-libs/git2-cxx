// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/app/strings/cov_report.hh>
#include <cov/app/tr.hh>

namespace cov::app {
	using replng = str::cov_report::lng;
	using ReportStrings = str::cov_report::Strings;

	template <>
	struct lngs_traits<replng>
	    : base_lngs_traits<replng, "cov_report", ReportStrings> {};
}  // namespace cov::app
