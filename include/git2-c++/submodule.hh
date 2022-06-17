#pragma once
#include "git2/submodule.h"
#include "git2-c++/ptr.hh"

#include <string>

namespace git {
	GIT_PTR_FREE(git_submodule);

	template <class Holder>
	struct basic_submodule : Holder {
		using Holder::Holder;
		using Holder::operator bool;

		std::string_view path() const { return git_submodule_path(this->get()); }
	private:
	};

	using submodule = basic<basic_submodule, git_submodule>::ptr;
	using submodule_handle = basic<basic_submodule, git_submodule>::handle;
}