// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include "git2-c++/odb.hh"
#include "git2-c++/ptr.hh"
#include "git2-c++/submodule.hh"
#include "git2/object.h"
#include "git2/repository.h"

#include <string>
#include <string_view>

namespace git {
	GIT_PTR_FREE(git_object);
	GIT_PTR_FREE(git_repository);

	template <class Holder>
	struct basic_repository : Holder {
		using Holder::Holder;

		std::string_view workdir() const {
			return git_repository_workdir(this->get());
		}
		std::string_view gitdir() const {
			return git_repository_gitdir(this->get());
		}
		std::string_view commondir() const {
			return git_repository_commondir(this->get());
		}

		template <typename SubmoduleCallback>
		void submodule_foreach(SubmoduleCallback cb) const {
			git_submodule_foreach(this->get(),
			                      submodule_callback<SubmoduleCallback>,
			                      reinterpret_cast<void*>(&cb));
		}

		template <typename ObjectType>
		ObjectType lookup(std::string_view id) const noexcept {
			git_oid oid;
			if (git_oid_fromstrn(&oid, id.data(), id.length())) return {};

			return lookup<ObjectType>(oid, (id.length() + 1) / 2);
		}

		template <typename ObjectType>
		ObjectType lookup(git_oid const& id,
		                  size_t prefix = GIT_OID_RAWSZ) const noexcept {
			return git::create_object<ObjectType>(git_object_lookup_prefix,
			                                      this->get(), &id, prefix,
			                                      ObjectType::OBJECT_TYPE);
		}

	protected:
		auto get() const { return Holder::get(); }

	private:
		template <typename SubmoduleCallback>
		static int submodule_callback(git_submodule* sm,
		                              const char* name,
		                              void* payload) {
			auto& cb = *reinterpret_cast<SubmoduleCallback*>(payload);
			cb(submodule_handle(sm), std::string_view(name));
			return 0;
		}
	};

	struct repository_handle : basic_repository<handle<git_repository>> {
		using basic_repository<handle<git_repository>>::basic_repository;
	};

	enum class Discover { WithinFs = false, AcrossFs = true };

	struct repository : basic_repository<ptr<git_repository>> {
		using basic_repository<ptr<git_repository>>::basic_repository;

		static repository open(char const* path);
		static repository wrap(const git::odb& odb);
		static std::string discover(char const* start_path,
		                            Discover accross_fs = Discover::WithinFs);

		operator repository_handle() const noexcept {
			return repository_handle{get()};
		}
		repository_handle handle_() const noexcept { return *this; }
	};
}  // namespace git
