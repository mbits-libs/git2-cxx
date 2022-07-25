// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/branch.hh>
#include <cov/reference.hh>
#include "branch-tag.hh"
#include "path-utils.hh"

namespace cov {
	struct branch_impl;
	struct branch_list_impl;

	struct branch_policy {
		using type = branch;
		using list_type = branch_list;
		using type_impl = branch_impl;
		using list_type_impl = branch_list_impl;
		static auto prefix() noexcept { return names::heads_dir_prefix; }
		static auto subdir() noexcept { return names::heads_dir; }
		static bool right_reference(ref_ptr<reference> const& ref) noexcept {
			return ref->references_branch();
		}
	};

	struct branch_impl : detail::link<branch_policy> {
		// trigger cpplint
		using detail::link<branch_policy>::link;
	};

	struct branch_list_impl : detail::link_list<branch_policy> {
		using detail::link_list<branch_policy>::link_list;
	};

	bool branch::is_valid_name(std::string_view name) {
		return branch_impl::is_valid_name(name);
	}

	ref_ptr<branch> branch::create(std::string_view name,
	                               git_oid const& id,
	                               references& refs) {
		return branch_impl::create(name, id, refs);
	}

	ref_ptr<branch> branch::lookup(std::string_view name, references& refs) {
		return branch_impl::lookup(name, refs);
	}

	ref_ptr<branch> branch::from(ref_ptr<reference>&& ref) {
		return branch_impl::from(std::move(ref));
	}

	ref_ptr<branch_list> branch::iterator(references& refs) {
		return branch_impl::iterator(refs);
	}
}  // namespace cov
