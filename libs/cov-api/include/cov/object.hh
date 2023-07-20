// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <cov/counted.hh>

namespace cov {
#define KNOWN_OBJECTS(X) \
	X(report)            \
	X(build)             \
	X(files)             \
	X(line_coverage)     \
	X(function_coverage) \
	X(blob)              \
	X(reference)         \
	X(reference_list)    \
	X(references)        \
	X(tag)               \
	X(tag_list)          \
	X(branch)            \
	X(branch_list)       \
	X(modules)

	enum obj_type {
		obj_unknown,
#define OBJECT_TYPE(name) obj_##name,
		KNOWN_OBJECTS(OBJECT_TYPE)
#undef OBJECT_TYPE
	};

#define FORWARD(name) struct name;
	KNOWN_OBJECTS(FORWARD)
#undef FORWARD

	struct object : counted {
		bool is_object() const noexcept override { return true; }
		virtual obj_type type() const noexcept = 0;
		template <typename Derived>
		inline bool is_a() const noexcept;
#define IS_A(NAME) \
	virtual bool is_##NAME() const noexcept { return false; }
		KNOWN_OBJECTS(IS_A)
#undef IS_A
	};

#define IS_A(NAME)                                    \
	template <>                                       \
	inline bool object::is_a<NAME>() const noexcept { \
		return is_##NAME();                           \
	}
	KNOWN_OBJECTS(IS_A)
#undef IS_A

	template <typename derived, typename base>
	inline bool is_a(base const& var) {
		return var.template is_a<derived>();
	}

	template <typename derived, typename base>
	inline bool is_a(base& var) {
		return var.template is_a<derived>();
	}

	template <typename derived, typename base>
	inline bool is_a(base const* var) {
		return var && var->template is_a<derived>();
	}

	template <typename derived, typename base>
	inline bool is_a(base* var) {
		return var && var->template is_a<derived>();
	}

	template <typename derived, typename base>
	inline derived const* as_a(base const& var) {
		return is_a<derived>(var) ? static_cast<derived const*>(&var) : nullptr;
	}

	template <typename derived, typename base>
	inline derived* as_a(base& var) {
		return is_a<derived>(var) ? static_cast<derived*>(&var) : nullptr;
	}

	template <typename derived, typename base>
	inline derived const* as_a(base const* var) {
		return is_a<derived>(var) ? static_cast<derived const*>(var) : nullptr;
	}

	template <typename derived, typename base>
	inline derived* as_a(base* var) {
		return is_a<derived>(var) ? static_cast<derived*>(var) : nullptr;
	}
}  // namespace cov
