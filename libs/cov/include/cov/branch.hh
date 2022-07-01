// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <git2/oid.h>
#include <cov/reference.hh>
#include <string_view>

namespace cov {
	struct branch_list;

	// no "upstream" is planned for now...
	struct branch : object {
		obj_type type() const noexcept override { return obj_branch; };
		bool is_branch() const noexcept final { return true; }
		static bool is_valid_name(std::string_view);
		static ref_ptr<branch> create(std::string_view name,
		                              git_oid const& id,
		                              references&);
		static ref_ptr<branch> lookup(std::string_view name, references&);
		static ref_ptr<branch> from(ref_ptr<reference>&&);
		static ref_ptr<branch_list> iterator(references&);
		virtual std::string_view name() const noexcept = 0;
		virtual git_oid const* id() const noexcept = 0;
	};

	struct branch_list : object {
		obj_type type() const noexcept override { return obj_branch_list; };
		bool is_branch_list() const noexcept final { return true; }
		virtual ref_ptr<branch> next() = 0;

		using iter_t = iterator_t<branch_list, branch>;
		iter_t begin() { return {this}; }
		iter_t end() { return {}; }
	};
}  // namespace cov
