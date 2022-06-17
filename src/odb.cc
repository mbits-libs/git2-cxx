#include "git2++/odb.hh"

namespace git {
	odb odb::open(const char* path) noexcept
	{
		return create_handle<odb>(git_odb_open, path);
	}
	
	void odb::hash(git_oid* out, bytes const& data, git_object_t type) noexcept
	{
		git_odb_hash(out, data.data(), data.size(), type);
	}

	bool odb::exists(git_oid const& id) const noexcept
	{
		return !!git_odb_exists(get(), &id);
	}

	bool odb::write(git_oid* out, bytes const& data, git_object_t type) const noexcept
	{
		return !git_odb_write(out, get(), data.data(), data.size(), type);
	}
}