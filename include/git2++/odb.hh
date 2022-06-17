#pragma once
#include "git2/odb.h"
#include "git2++/ptr.hh"
#include "git2++/bytes.hh"

#include <string>

namespace git {
	GIT_PTR_FREE(git_odb);

	struct odb : ptr<git_odb> {
		using ptr<git_odb>::ptr;

		static odb open(const char*) noexcept;
		static void hash(git_oid*, bytes const&, git_object_t) noexcept;
		bool exists(git_oid const&) const noexcept;
		bool write(git_oid*, bytes const&, git_object_t) const noexcept;
	private:
		friend struct repository;
	};
}