// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <cov/counted.hh>
#include <cov/io/db_object.hh>
#include <filesystem>

namespace cov {
	struct backend : public counted {
		template <typename Object>
		ref_ptr<Object> lookup(git_oid const& id) {
			return as_a<Object>(lookup_object(id));
		}
		virtual bool write(git_oid&, ref_ptr<object> const&) = 0;

	private:
		virtual ref_ptr<object> lookup_object(git_oid const& id) = 0;
	};

	ref_ptr<backend> loose_backend_create(std::filesystem::path const&);
}  // namespace cov
