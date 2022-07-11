// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/branch.hh>
#include <cov/repository.hh>
#include <cov/tag.hh>
#include <git2/error.hh>
#include "path-utils.hh"

namespace cov {
	namespace {
		git::config open_config(std::filesystem::path const& common,
		                        std::error_code& ec) {
			auto result =
			    git::config::open_default(names::dot_config, "cov"sv, ec);
			if (!ec) ec = result.add_local_config(common);
			if (ec) result = nullptr;
			return result;
		}
	}  // namespace

	repository::~repository() = default;

	repository::repository(std::filesystem::path const& common,
	                       std::error_code& ec)
	    : commondir_{common}, cfg_{open_config(common, ec)} {
		if (!ec && !common.empty()) refs_ = references::make_refs(common);
	}

	ref_ptr<object> repository::dwim(std::string_view name) const {
		auto ref = refs_->dwim(name);
		if (ref) {
			if (ref->references_branch()) return branch::from(std::move(ref));
			if (ref->references_tag()) return tag::from(std::move(ref));
		}
		return ref;
	}
}  // namespace cov
