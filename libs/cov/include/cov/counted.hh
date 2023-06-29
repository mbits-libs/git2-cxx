// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <atomic>
#include <concepts>
#include <utility>

namespace cov {
	struct counted {
		virtual void acquire() = 0;
		virtual void release() = 0;
		virtual bool is_object() const noexcept { return false; }
		virtual long counter() const noexcept = 0;

	protected:
		virtual ~counted();
	};

	template <typename Base = counted>
	class counted_impl : public Base {
	public:
		void acquire() override { ++counter_; }
		void release() override {
			if (!--counter_) delete this;
		}

		// GCOV_EXCL_START
		long counter() const noexcept override { return counter_; }
		// GCOV_EXCL_STOP

	private:
		std::atomic<long> counter_{1};
	};
	static_assert(std::atomic<long>::is_always_lock_free);

	template <typename T>
	concept Counted = requires(T& ref) {
		ref.acquire();
		ref.release();
	};  // NOLINT(readability/braces)

	template <typename Object>
	class ref_ptr {
	public:
		using pointer = Object*;
		using element_type = Object;

		constexpr ref_ptr() noexcept = default;
		constexpr ref_ptr(std::nullptr_t) noexcept {}
		explicit ref_ptr(pointer p) noexcept : ptr_{p} {}

		ref_ptr(ref_ptr&& other) noexcept : ptr_{other.ptr_} {
			other.ptr_ = nullptr;
		}
		ref_ptr(ref_ptr const& other) noexcept : ref_ptr{other.duplicate()} {}

		template <std::derived_from<Object> Other>
		ref_ptr(ref_ptr<Other>&& other) noexcept : ptr_{other.unlink()} {}
		template <std::derived_from<Object> Other>
		ref_ptr(ref_ptr<Other> const& other) noexcept
		    : ref_ptr{other.duplicate()} {}

		~ref_ptr() { release(); }

		ref_ptr& operator=(ref_ptr&& other) noexcept {
			if (this == &other) return *this;
			reset(other.unlink());
			return *this;
		}
		template <std::derived_from<Object> Other>
		ref_ptr& operator=(ref_ptr<Other>&& other) noexcept {
			if (this == &other) return *this;
			reset(other.unlink());
			return *this;
		}
		template <std::derived_from<Object> Other>
		ref_ptr& operator=(ref_ptr<Other> const& other) noexcept {
			return *this = other.duplicate();
		}
		ref_ptr& operator=(std::nullptr_t) noexcept {
			reset();
			return *this;
		}

		explicit operator bool() const noexcept { return !!ptr_; }
		pointer get() const noexcept { return ptr_; }
		element_type& operator*() const noexcept { return *ptr_; }
		pointer operator->() const noexcept { return ptr_; }
		ref_ptr<Object> duplicate() const noexcept {
			acquire();
			return ref_ptr{ptr_};
		}
		pointer unlink() noexcept {
			auto tmp = ptr_;
			ptr_ = nullptr;
			return tmp;
		}
		template <std::derived_from<Object> Other>
		bool operator==(ref_ptr<Other> const& other) const noexcept {
			return get() == other.get();
		}
		template <std::derived_from<Object> Other>
		auto operator<=>(ref_ptr<Other> const& other) const noexcept {
			return get() <=> other.get();
		}

		void reset(pointer ptr = {}) noexcept {
			release();
			ptr_ = ptr;
		}

		long counter() const noexcept { return ptr_ ? ptr_->counter() : -1; }

	private:
		void release() const {
			if (ptr_) ptr_->release();
		}

		void acquire() const {
			if (ptr_) ptr_->acquire();
		}

		pointer ptr_{};
	};

	template <Counted derived, Counted base>
	inline bool is_a(ref_ptr<base> const& var) {
		return is_a<derived>(var.get());
	}

	template <Counted derived, Counted base>
	inline bool is_a(ref_ptr<base>& var) {
		return is_a<derived>(var.get());
	}

	template <Counted derived, Counted base>
	inline ref_ptr<derived> as_a(ref_ptr<base> const& var) {
		auto ptr = as_a<derived>(var.get());
		if (ptr) ptr->acquire();
		return ref_ptr{ptr};
	}

	template <Counted derived, Counted base>
	inline ref_ptr<derived> as_a(ref_ptr<base>& var) {
		auto ptr = as_a<derived>(var.get());
		if (ptr) ptr->acquire();
		return ref_ptr{ptr};
	}

	template <Counted same>
	inline ref_ptr<same> as_a(ref_ptr<same>&& var) {
		return var;
	}

	template <Counted Object, typename... Args>
	inline ref_ptr<Object> make_ref(Args&&... args) {
		return ref_ptr<Object>{new Object(std::forward<Args>(args)...)};
	}

	template <class Counted, class Intermediate>
	struct enable_ref_from_this {
		ref_ptr<Counted> ref_from_this() {
			auto self = static_cast<Counted*>(static_cast<Intermediate*>(this));
			self->acquire();
			return ref_ptr{self};
		}
	};
}  // namespace cov
