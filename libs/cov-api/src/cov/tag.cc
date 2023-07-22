// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/reference.hh>
#include <cov/tag.hh>
#include "branch-tag.hh"
#include "path-utils.hh"

namespace cov {
	struct tag_impl;
	struct tag_list_impl;

	struct tag_policy {
		using type = tag;
		using list_type = tag_list;
		using type_impl = tag_impl;
		using list_type_impl = tag_list_impl;
		static auto prefix() noexcept { return names::tags_dir_prefix; }
		static auto subdir() noexcept { return names::tags_dir; }
		static bool right_reference(ref_ptr<reference> const& ref) noexcept {
			return ref->references_tag();
		}
	};

	struct tag_impl : detail::link<tag_policy> {
		using detail::link<tag_policy>::link;
	};

	struct tag_list_impl : detail::link_list<tag_policy> {
		using detail::link_list<tag_policy>::link_list;
	};

	bool tag::is_valid_name(std::string_view name) {
		return tag_impl::is_valid_name(name);
	}

	ref_ptr<tag> tag::create(std::string_view name,
	                         git::oid_view id,
	                         references& refs) {
		return tag_impl::create(name, id, refs);
	}

	ref_ptr<tag> tag::lookup(std::string_view name, references& refs) {
		return tag_impl::lookup(name, refs);
	}

	ref_ptr<tag> tag::from(ref_ptr<reference>&& ref) {
		return tag_impl::from(std::move(ref));
	}

	ref_ptr<tag_list> tag::iterator(references& refs) {
		return tag_impl::iterator(refs);
	}
}  // namespace cov
