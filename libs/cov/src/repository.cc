// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/repository.hh>
#include <git2/error.hh>
#include "path-utils.hh"

namespace cov {
	namespace {
		git::config open_config(std::filesystem::path const& common,
		                        std::error_code& ec) {
			auto result =
			    git::config::open_default(names::dot_config, "cov"sv, ec);
			if (!ec) ec = git::make_error_code(result.add_local_config(common));
			if (ec) result = nullptr;
			return result;
		}
	}  // namespace
	repository::~repository() = default;

	repository::repository(std::filesystem::path const& common,
	                       std::error_code& ec)
	    : commondir_{common}, cfg_{open_config(common, ec)} {}
}  // namespace cov
