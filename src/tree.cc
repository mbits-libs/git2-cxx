#include "git2++/tree.hh"

namespace git {
	git_oid const& tree_entry::oid() const noexcept {
		return *git_tree_entry_id(get());
	}

	std::string tree_entry::strid() const noexcept {
		try {
			auto out = std::string(size_t{ GIT_OID_HEXSZ }, ' ');
			git_oid_fmt(out.data(), git_tree_entry_id(get()));
			return out;
		} catch (std::bad_alloc&) {
			return {};
		}
	}

	tree tree::lookup(repository_handle repo, std::string_view id) noexcept {
		return repo.lookup<tree>(id);
	}

	tree_entry tree::entry_bypath(const char* path) const noexcept {
		return git::create_handle<tree_entry>(git_tree_entry_bypath, get(), path);
	}
}