#include "git2++/repository.hh"

namespace git {
	repository repository::open(char const* path) {
		return git::create_handle<repository>(git_repository_open, path);
	}

	repository repository::wrap(const git::odb& odb) {
		return git::create_handle<repository>(git_repository_wrap_odb, odb.get());
	}

	std::string repository::discover(char const* start_path, Discover accross_fs) {
		std::string out;

		git_buf buf{};
		auto const ret = git_repository_discover(&buf, start_path, accross_fs == Discover::AcrossFs ? 1 : 0, NULL);
		if (!ret)
			out = buf.ptr;

		git_buf_dispose(&buf);
		return out;
	}
}