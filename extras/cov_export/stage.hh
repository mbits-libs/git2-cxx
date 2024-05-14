// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <c++filt/expression.hh>
#include <cov/app/dirs.hh>
#include <cov/format.hh>
#include <filesystem>
#include <native/platform.hh>
#include <vector>
#include <web/link_service.hh>
#include <web/mstch_cache.hh>

namespace cov::app::web {
	using std::literals::operator""sv;

	struct page {
		std::filesystem::path filename{};
		std::string module_filter{};
		std::string fname_filter{};
	};

	struct stage {
		placeholder::rating marks;
		cov::ref_ptr<cov::modules> mods;
		std::vector<file_stats> diff;
		std::filesystem::path out_dir;
		cov::repository& repo;
		git::oid_view ref;
		git::oid_view base;
		cxx_filt::Replacements replacements;
		export_link_service links{};
		dir_cache tmplt{
		    {
		        installed_site(),
		        runtime_site(),
#ifndef NDEBUG
		        build_site(),
		        source_site(),
#endif
		    },
		    "templates",
		};
		std::shared_ptr<octicon_callback> octicons = octicon_callback::create(
		    platform::core_extensions::sys_root() / directory_info::site_res /
		    "octicons.json"sv);
		mstch::map commit_ctx{}, report_ctx{};

		void initialize(std::error_code& ec);
		std::vector<web::page> list_pages_in_report() const;

		struct page_state {
			page pg;
			std::filesystem::path full_path;
			std::string template_name{};

			mstch::map create_context(stage const& stg, std::error_code& ec);
		};

		page_state next_page(page const& pg, std::error_code& ec);

	private:
		static std::filesystem::path runtime_site();
		static std::filesystem::path installed_site();
#ifndef NDEBUG
		static std::filesystem::path build_site();
		static std::filesystem::path source_site();
#endif
	};
}  // namespace cov::app::web
