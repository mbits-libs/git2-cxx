// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include "git2-c++/blob.hh"

namespace git {
	blob blob::lookup(repository_handle repo, std::string_view id) noexcept {
		return repo.lookup<blob>(id);
	}

	blob blob::lookup(repository_handle repo, git_oid const& id) noexcept {
		return repo.lookup<blob>(id);
	}

	bytes blob::raw() const noexcept {
		auto const* data =
		    reinterpret_cast<std::byte const*>(git_blob_rawcontent(get()));
		auto const size =
		    static_cast<bytes::size_type>(git_blob_rawsize(get()));
		return {data, size};
	}

	git_buf blob::filtered(char const* as_path) const noexcept {
		git_buf buf{};
		git_blob_filter_options options = GIT_BLOB_FILTER_OPTIONS_INIT;
		git_blob_filter(&buf, get(), as_path, &options);
		return buf;
	}
}  // namespace git
