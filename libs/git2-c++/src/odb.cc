// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include "git2-c++/odb.hh"

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

	odb odb::open(std::filesystem::path const& path) {
		return create_handle<odb>(git_odb_open, get_path(path).c_str());
	}

	void odb::hash(git_oid* out,
	               bytes const& data,
	               git_object_t type) noexcept {
		git_odb_hash(out, data.data(), data.size(), type);
	}

	bool odb::exists(git_oid const& id) const noexcept {
		return !!git_odb_exists(get(), &id);
	}

	bool odb::write(git_oid* out,
	                bytes const& data,
	                git_object_t type) const noexcept {
		return !git_odb_write(out, get(), data.data(), data.size(), type);
	}
}  // namespace git
