// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include "git2-c++/repository.hh"

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

	repository repository::open(path const& p) {
		return git::create_handle<repository>(
		    [](git_repository** out, path const& forwarded) {
			    return git_repository_open(out, get_path(forwarded).c_str());
		    },
		    p);
	}

	repository repository::wrap(const git::odb& odb) {
		return git::create_handle<repository>(git_repository_wrap_odb,
		                                      odb.get());
	}

	std::filesystem::path repository::discover(path const& start_path,
	                                           Discover accross_fs) {
		std::filesystem::path out;

		git_buf buf{};
		auto const ret = git_repository_discover(
		    &buf, get_path(start_path).c_str(),
		    accross_fs == Discover::AcrossFs ? 1 : 0, NULL);
		if (!ret) out = buf.ptr;

		git_buf_dispose(&buf);
		return out;
	}
}  // namespace git
