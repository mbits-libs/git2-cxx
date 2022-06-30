// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <atomic>
#include <concepts>

namespace cov {
	struct counted {
		virtual void acquire() = 0;
		virtual void release() = 0;
		virtual bool is_object() const noexcept { return false; }

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

	private:
		std::atomic<long> counter_{1};
	};
	static_assert(std::atomic<long>::is_always_lock_free);

	template <typename T>
	concept Counted = requires(T& ref) {
		ref.acquire();
		ref.release();
	};

	template <typename Object>
	class ref {
	public:
		using pointer = Object*;
		using element_type = Object;

		constexpr ref() noexcept = default;
		constexpr ref(std::nullptr_t) noexcept {}
		explicit ref(pointer p) noexcept : ptr_{p} {}

		ref(ref&& other) noexcept : ptr_{other.ptr_} { other.ptr_ = nullptr; }
		ref(ref const& other) noexcept : ref{other.duplicate()} {}

		template <std::derived_from<Object> Other>
		ref(ref<Other>&& other) noexcept : ptr_{other.unlink()} {}
		template <std::derived_from<Object> Other>
		ref(ref<Other> const& other) noexcept : ref{other.duplicate()} {}

		~ref() { release(); }

		ref& operator=(ref&& other) noexcept {
			reset(other.unlink());
			return *this;
		}
		template <std::derived_from<Object> Other>
		ref& operator=(ref<Other>&& other) noexcept {
			reset(other.unlink());
			return *this;
		}
		template <std::derived_from<Object> Other>
		ref& operator=(ref<Other> const& other) noexcept {
			return *this = other.duplicate();
		}
		ref& operator=(std::nullptr_t) noexcept {
			reset();
			return *this;
		}

		explicit operator bool() const noexcept { return !!ptr_; }
		pointer get() const noexcept { return ptr_; }
		element_type& operator*() const noexcept { return &ptr_; }
		pointer operator->() const noexcept { return ptr_; }
		ref<Object> duplicate() const noexcept {
			acquire();
			return ref{ptr_};
		}
		pointer unlink() noexcept {
			auto tmp = ptr_;
			ptr_ = nullptr;
			return tmp;
		}
		template <std::derived_from<Object> Other>
		bool operator==(ref<Other> const& other) const noexcept {
			return get() == other.get();
		}
		template <std::derived_from<Object> Other>
		auto operator<=>(ref<Other> const& other) const noexcept {
			return get() <=> other.get();
		}

		void reset(pointer ptr = {}) noexcept {
			if (ptr == ptr_) return;
			release();
			ptr_ = ptr;
		}

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
	inline ref<derived> as_a(ref<base> const& var) {
		auto ptr = as_a<derived>(var.get());
		if (ptr) ptr->acquire();
		return ref{ptr};
	}

	template <Counted Object, typename... Args>
	inline ref<Object> make_ref(Args&&... args) {
		return ref<Object>{new Object(std::forward<Args>(args)...)};
	};

	template <class Counted, class Intermediate>
	struct enable_ref_from_this {
		ref<Counted> ref_from_this() {
			auto self = static_cast<Counted*>(static_cast<Intermediate*>(this));
			self->acquire();
			return ref{self};
		}
	};
}  // namespace cov
