// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <git2/oid.h>
#include <cov/reference.hh>
#include <string_view>

namespace cov {
	struct tag_list;

	struct tag : object {
		obj_type type() const noexcept override { return obj_tag; };
		bool is_tag() const noexcept final { return true; }
		static bool is_valid_name(std::string_view);
		static ref_ptr<tag> create(std::string_view name,
		                           git::oid_view id,
		                           references&);
		static ref_ptr<tag> lookup(std::string_view name, references&);
		static ref_ptr<tag> from(ref_ptr<reference>&&);
		static ref_ptr<tag_list> iterator(references&);
		virtual std::string_view name() const noexcept = 0;
		virtual git::oid const* id() const noexcept = 0;
	};

	struct tag_list : object {
		obj_type type() const noexcept override { return obj_tag_list; };
		bool is_tag_list() const noexcept final { return true; }
		virtual ref_ptr<tag> next() = 0;

		using iter_t = iterator_t<tag_list, tag>;
		iter_t begin() { return {this}; }
		iter_t end() { return {}; }
	};
}  // namespace cov
