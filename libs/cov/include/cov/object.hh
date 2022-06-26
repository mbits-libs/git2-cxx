// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <cov/counted.hh>
#include <cov/types.hh>
#include <memory>
#include <string_view>

namespace cov {
#define KNOWN_OBJECTS(X) \
	X(report)            \
	X(reported_file)     \
	X(line_coverage)     \
	X(blob)

	enum obj_type {
		obj_unknown,
#define OBJECT_TYPE(name) obj_##name,
		KNOWN_OBJECTS(OBJECT_TYPE)
#undef OBJECT_TYPE
	};

#define FORWARD(name) struct name##_object;
	KNOWN_OBJECTS(FORWARD)
#undef FORWARD

	struct object : counted {
		bool is_object() const noexcept override { return true; }
		virtual obj_type type() const noexcept = 0;
		// virtual std::error_code write() = 0;
		// virtual std::error_code read() = 0;
		template <typename Derived>
		inline bool is_a() const noexcept;
#define IS_A(NAME) \
	virtual bool is_##NAME() const noexcept { return false; }
		KNOWN_OBJECTS(IS_A)
#undef IS_A
	};

#define IS_A(NAME)                                             \
	template <>                                                \
	inline bool object::is_a<NAME##_object>() const noexcept { \
		return is_##NAME();                                    \
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
		return is_a<derived>(var) ? &static_cast<derived const&>(var) : nullptr;
	}

	template <typename derived, typename base>
	inline derived* as_a(base& var) {
		return is_a<derived>(var) ? &static_cast<derived&>(var) : nullptr;
	}

	template <typename derived, typename base>
	inline derived const* as_a(base const* var) {
		return is_a<derived>(var) ? static_cast<derived const*>(var) : nullptr;
	}

	template <typename derived, typename base>
	inline derived* as_a(base* var) {
		return is_a<derived>(var) ? static_cast<derived*>(var) : nullptr;
	}

	struct report_entry {
		virtual ~report_entry();
		virtual git_oid const* reported_file() const noexcept = 0;
		virtual v1::coverage_stats stats() const noexcept = 0;
		virtual std::string_view path() const noexcept = 0;
	};

	struct report_object : object {
		obj_type type() const noexcept override { return obj_report; };
		bool is_report() const noexcept final { return true; }
		virtual git_oid const* parent_report() const noexcept = 0;
		virtual git_oid const* commit() const noexcept = 0;
		virtual std::string_view branch() const noexcept = 0;
		virtual std::string_view author_name() const noexcept = 0;
		virtual std::string_view author_email() const noexcept = 0;
		virtual std::string_view committer_name() const noexcept = 0;
		virtual std::string_view committer_email() const noexcept = 0;
		virtual std::string_view message() const noexcept = 0;
		virtual git_time_t commit_time_utc() const noexcept = 0;
		virtual git_time_t add_time_utc() const noexcept = 0;
		virtual std::vector<std::unique_ptr<v1::report_entry>> const& entries()
		    const noexcept = 0;
	};

	struct reported_file_object : object {
		obj_type type() const noexcept override { return obj_reported_file; };
		bool is_reported_file() const noexcept final { return true; }
		virtual v1::coverage_stats stats() const noexcept = 0;
	};

	struct line_coverage_object : object {
		obj_type type() const noexcept override { return obj_line_coverage; };
		bool is_line_coverage() const noexcept final { return true; }
		virtual std::vector<v1::coverage> const& coverage() const noexcept = 0;
	};
	ref<line_coverage_object> line_coverage_create(std::vector<v1::coverage>&&);
}  // namespace cov
