// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/git2/repository.hh>

namespace git {
	namespace {
		using namespace std::filesystem;
#ifdef __cpp_lib_char8_t
		std::string get_path(path p) {
			auto const s8 = p.make_preferred().u8string();
			return {reinterpret_cast<char const*>(s8.data()), s8.length()};
		}
#else
		std::string get_path(path p) { return p.make_preferred().u8string(); }
#endif
	}  // namespace

	repository repository::init(path const& p,
	                            bool is_bare,
	                            std::error_code& ec) {
		return git::create_handle<repository>(
		    ec,
		    [](git_repository** out, path const& forwarded,
		       bool forwarded_is_bare) {
			    return git_repository_init(out, get_path(forwarded).c_str(),
			                               forwarded_is_bare ? 1 : 0);
		    },
		    p, is_bare);
	}

	repository repository::open(path const& p, std::error_code& ec) {
		return git::create_handle<repository>(
		    ec,
		    [](git_repository** out, path const& forwarded) {
			    return git_repository_open(out, get_path(forwarded).c_str());
		    },
		    p);
	}

	repository repository::wrap(const git::odb& odb, std::error_code& ec) {
		return git::create_handle<repository>(ec, git_repository_wrap_odb,
		                                      odb.get());
	}

	std::filesystem::path repository::discover(path const& start_path,
	                                           Discover accross_fs,
	                                           std::error_code& ec) noexcept {
		std::filesystem::path out;

		git_buf buf{};
		try {
			ec = as_error(git_repository_discover(
			    &buf, get_path(start_path).c_str(),
			    accross_fs == Discover::AcrossFs ? 1 : 0, NULL));
			if (!ec) out = buf.ptr;
		} catch (std::bad_alloc&) {
			ec = make_error_code(std::errc::not_enough_memory);
			out.clear();
		}

		git_buf_dispose(&buf);
		return out;
	}
}  // namespace git
