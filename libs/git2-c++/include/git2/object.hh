// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <git2/repository.hh>

#include <string>

namespace git {
	struct repository_handle;

	template <class Holder, git_object_t ObjectType>
	struct basic_object : Holder {
		using Holder::Holder;
		using Holder::operator bool;

		using pointer = typename Holder::pointer;
		using element_type = typename Holder::element_type;

		static constexpr git_object_t OBJECT_TYPE = ObjectType;

		inline repository_handle owner() const noexcept {
			return repository_handle{
			    git_object_owner(git::cast<git_object*>(this->get()))};
		}

		template <class Peeled>
		Peeled peel(std::error_code& ec) const noexcept {
			return git::create_object<Peeled>(ec, git_object_peel, get_object(),
			                                  Peeled::OBJECT_TYPE);
		}

		git_oid const& oid() const noexcept {
			return *git_object_id(get_object());
		}

		std::string strid() const noexcept {
			try {
				auto out = std::string(size_t{GIT_OID_HEXSZ}, ' ');
				git_oid_fmt(out.data(), git_object_id(get_object()));
				return out;
			} catch (std::bad_alloc&) {
				return {};
			}  // GCOV_EXCL_LINE[WIN32]
		}

	protected:
		constexpr auto get_object() const noexcept {
			return git::cast<git_object*>(this->get());
		}
	};

	template <class GitClass, git_object_t ObjectType>
	struct object_ptr : basic_object<ptr<GitClass>, ObjectType> {
		using basic_object<ptr<GitClass>, ObjectType>::basic_object;
	};
}  // namespace git
