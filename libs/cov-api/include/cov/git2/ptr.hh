// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <git2/object.h>
#include <git2/types.h>
#include <cov/git2/error.hh>
#include <memory>
#include <utility>

namespace git {
	template <typename Object>
	struct ptr_closer;

	template <>
	struct ptr_closer<git_object> {
		void operator()(git_object* ptr) { git_object_free(ptr); }
	};

	template <typename Object>
	struct ptr : protected std::unique_ptr<Object, ptr_closer<Object>> {
		using std::unique_ptr<Object, ptr_closer<Object>>::unique_ptr;
		using std::unique_ptr<Object, ptr_closer<Object>>::operator bool;
		using pointer =
		    typename std::unique_ptr<Object, ptr_closer<Object>>::pointer;
		using element_type =
		    typename std::unique_ptr<Object, ptr_closer<Object>>::element_type;

		pointer raw() const noexcept { return this->get(); }
	};

	template <typename Object>
	class handle {
	public:
		using pointer = Object*;
		using element_type = std::remove_cvref_t<Object>;

		explicit handle(pointer ptr) noexcept : ptr_{ptr} {}
		handle() = default;
		handle(const handle&) = default;
		handle& operator=(const handle&) = default;
		handle(handle&&) = default;
		handle& operator=(handle&&) = default;

		explicit operator bool() const noexcept { return !!ptr_; }

		pointer raw() const noexcept { return ptr_; }

	protected:
		pointer get() const noexcept { return ptr_; }

	private:
		pointer ptr_{nullptr};
	};

	template <template <class> class Impl, class Object>
	struct basic {
		using ptr = Impl<git::ptr<Object>>;
		using handle = Impl<git::handle<Object>>;
	};

	template <typename Type>
	struct is_git_object : std::false_type {};
	template <>
	struct is_git_object<git_commit*> : std::true_type {};
	template <>
	struct is_git_object<git_tree*> : std::true_type {};
	template <>
	struct is_git_object<git_blob*> : std::true_type {};
	template <>
	struct is_git_object<git_tag*> : std::true_type {};
	template <>
	struct is_git_object<git_tree_entry*> : std::true_type {};

	template <class A, class B>
	struct can_cast
	    : std::conjunction<std::is_same<A, git_object*>, is_git_object<B>> {};
	template <class A, class B>
	struct cast_allowed
	    : std::disjunction<std::is_same<A, B>, can_cast<A, B>, can_cast<B, A>> {
	};

	template <class To, class From>
	inline std::enable_if_t<cast_allowed<To, From>::value, To> cast(From ptr) {
		return reinterpret_cast<To>(ptr);
	}

	template <typename GitObject,
	          typename GitStructPtr,
	          typename OpenFunction,
	          typename... Args>
	inline GitObject create_handle_lowlevel(std::error_code& ec,
	                                        OpenFunction&& open_function,
	                                        Args&&... args) {
		using pointer = typename GitObject::pointer;

		GitStructPtr out{nullptr};
		ec = as_error(open_function(&out, std::forward<Args>(args)...));

		if (ec) {
			GitObject{git::cast<pointer>(out)};
			out = nullptr;
		}

		return GitObject{git::cast<pointer>(out)};
	}

	template <typename GitObject, typename OpenFunction, typename... Args>
	inline GitObject create_object(std::error_code& ec,
	                               OpenFunction&& open_function,
	                               Args&&... args) {
		return git::create_handle_lowlevel<GitObject, git_object*>(
		    ec, std::forward<OpenFunction>(open_function),
		    std::forward<Args>(args)...);
	}

	template <typename GitObject, typename OpenFunction, typename... Args>
	inline GitObject create_handle(std::error_code& ec,
	                               OpenFunction&& open_function,
	                               Args&&... args) {
		return git::create_handle_lowlevel<GitObject,
		                                   typename GitObject::pointer>(
		    ec, std::forward<OpenFunction>(open_function),
		    std::forward<Args>(args)...);
	}
}  // namespace git

#define GIT_PTR(ObjectType, CloserFunction)                       \
	template <>                                                   \
	struct ptr_closer<ObjectType> {                               \
		void operator()(ObjectType* ptr) { CloserFunction(ptr); } \
	}

#define GIT_PTR_FREE(ObjectType) GIT_PTR(ObjectType, ObjectType##_free)
