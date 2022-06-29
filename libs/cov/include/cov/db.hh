// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <cov/counted.hh>
#include <cov/io/db_object.hh>
#include <filesystem>

namespace cov {
	struct backend : public counted {
		template <typename Object>
		ref<Object> lookup(git_oid const& id) {
			return as_a<Object>(lookup_object(id));
		}
		virtual bool write(git_oid&, ref<object> const&) = 0;

	private:
		virtual ref<object> lookup_object(git_oid const& id) = 0;
	};

	ref<backend> loose_backend_create(std::filesystem::path const&);
}  // namespace cov
