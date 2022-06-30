// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/reference.hh>
#include <filesystem>

namespace cov {
	enum class ref_tgt {
		other,
		branch,
		tag,
	};

	ref<reference> direct_reference_create(ref_tgt tgt_kind,
	                                       std::string const& name,
	                                       size_t shorthand_prefix,
	                                       git_oid const& target);

	ref<reference> symbolic_reference_create(
	    ref_tgt tgt_kind,
	    std::string const& name,
	    size_t shorthand_prefix,
	    std::string const& target,
	    ref<references> const& peel_source);

	ref<reference_list> reference_list_create(std::filesystem::path const& path,
	                                          std::string const& prefix,
	                                          ref<references> const& source);
}  // namespace cov
