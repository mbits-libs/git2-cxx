// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/git2/odb.hh>

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

	odb odb::create(std::error_code& ec) {
		return create_handle<odb>(ec, git_odb_new);
	}

	odb odb::open(std::filesystem::path const& path, std::error_code& ec) {
		return create_handle<odb>(ec, git_odb_open, get_path(path).c_str());
	}

	void odb::hash(git_oid* out,
	               bytes const& data,
	               git_object_t type) noexcept {
		git_odb_hash(out, data.data(), data.size(), type);
	}

	bool odb::exists(git_oid const& id) const noexcept {
		return !!git_odb_exists(get(), &id);
	}

	std::error_code odb::write(git_oid* out,
	                           bytes const& data,
	                           git_object_t type) const noexcept {
		return as_error(
		    git_odb_write(out, get(), data.data(), data.size(), type));
	}
}  // namespace git
