// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <c++filt/expression.hh>
#include <cov/core/report_stats.hh>
#include <cov/format.hh>
#include <cov/projection.hh>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <web/link_service.hh>
#include <web/mstch_cache.hh>

namespace cov::app::web {
	std::vector<std::pair<std::string, std::string>> make_breadcrumbs(
	    std::string_view crumbs,
	    projection::entry_type type,
	    link_service const& links);

	std::pair<mstch::map, mstch::map> add_build_info(cov::repository& repo,
	                                                 git::oid_view oid,
	                                                 git::oid_view from,
	                                                 std::error_code& ec);
	void add_mark(mstch::map& ctx, translatable mark);
	void add_navigation(mstch::map& ctx,
	                    bool is_root,
	                    bool is_module,
	                    bool is_file,
	                    std::string_view path,
	                    link_service const& links);
	void add_table(mstch::map& ctx,
	               core::projected_entries const& projection,
	               link_service const& links);
	void add_file_source(mstch::map& ctx,
	                     cov::repository& repo,
	                     git::oid_view ref,
	                     std::string_view path,
	                     cxx_filt::Replacements const& replacements,
	                     std::error_code& ec);
}  // namespace cov::app::web
