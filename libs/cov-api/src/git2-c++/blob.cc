// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <concepts>
#include <cov/git2/blob.hh>

namespace git {
	namespace {
		inline bytes::size_type as_size_type(bytes::size_type in) noexcept {
			return in;
		}

		template <typename A, typename B>
		concept NotA = !std::same_as<A, B>;

		inline bytes::size_type as_size_type(
		    NotA<bytes::size_type> auto in) noexcept {
			return static_cast<bytes::size_type>(in);
		}
	}  // namespace

	blob blob::lookup(repository_handle repo,
	                  std::string_view id,
	                  std::error_code& ec) noexcept {
		return repo.lookup<blob>(id, ec);
	}

	blob blob::lookup(repository_handle repo,
	                  git::oid_view id,
	                  std::error_code& ec) noexcept {
		return repo.lookup<blob>(id, ec);
	}

	bytes blob::raw() const noexcept {
		auto const* data =
		    reinterpret_cast<std::byte const*>(git_blob_rawcontent(get()));
		auto const size = as_size_type(git_blob_rawsize(get()));
		return {data, size};
	}

	git_buf blob::filtered(char const* as_path,
	                       std::error_code& ec) const noexcept {
		git_buf buf{};
#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
		git_blob_filter_options options = GIT_BLOB_FILTER_OPTIONS_INIT;
#ifdef __clang__
#pragma GCC diagnostic pop
#endif
		ec = as_error(git_blob_filter(&buf, get(), as_path, &options));
		return buf;
	}
}  // namespace git
