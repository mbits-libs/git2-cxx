// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/reference.hh>
#include <filesystem>
#include <string>

namespace cov {
	enum class ref_tgt : int {
		other,
		branch,
		tag,
	};

	ref_ptr<reference> direct_reference_create(ref_tgt tgt_kind,
	                                           std::string const& name,
	                                           size_t shorthand_prefix,
	                                           git_oid const& target);

	ref_ptr<reference> symbolic_reference_create(
	    ref_tgt tgt_kind,
	    std::string const& name,
	    size_t shorthand_prefix,
	    std::string const& target,
	    ref_ptr<references> const& peel_source);

	ref_ptr<reference> null_reference_create(ref_tgt tgt_kind,
	                                         std::string const& name,
	                                         size_t shorthand_prefix);

	ref_ptr<reference_list> reference_list_create(
	    std::filesystem::path const& path,
	    std::string const& prefix,
	    ref_ptr<references> const& source);
}  // namespace cov
