// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <cov/counted.hh>
#include <cov/git2/oid.hh>
#include <cov/io/db_object.hh>
#include <filesystem>

namespace cov {
	struct backend : public counted {
		template <typename Object>
		ref_ptr<Object> lookup(git::oid_view id) {
			return as_a<Object>(lookup_object(id));
		}
		template <typename Object>
		ref_ptr<Object> lookup(git::oid_view id, size_t character_count) {
			return as_a<Object>(lookup_object(id, character_count));
		}
		virtual bool write(git::oid&, ref_ptr<object> const&) = 0;

		static ref_ptr<backend> loose_backend(std::filesystem::path const&);

	private:
		friend struct repository;
		virtual ref_ptr<object> lookup_object(git::oid_view id) const = 0;
		virtual ref_ptr<object> lookup_object(git::oid_view id,
		                                      size_t character_count) const = 0;
	};
}  // namespace cov
