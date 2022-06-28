// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <cov/counted.hh>
#include <cov/io/types.hh>
#include <memory>
#include <string_view>

namespace cov {
#define KNOWN_OBJECTS(X) \
	X(report)            \
	X(report_files)      \
	X(line_coverage)     \
	X(blob)

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
		// virtual std::error_code write() = 0;
		// virtual std::error_code read() = 0;
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

	struct report : object {
		obj_type type() const noexcept override { return obj_report; };
		bool is_report() const noexcept final { return true; }
		virtual git_oid const* parent_report() const noexcept = 0;
		virtual git_oid const* file_list() const noexcept = 0;
		virtual git_oid const* commit() const noexcept = 0;
		virtual std::string_view branch() const noexcept = 0;
		virtual std::string_view author_name() const noexcept = 0;
		virtual std::string_view author_email() const noexcept = 0;
		virtual std::string_view committer_name() const noexcept = 0;
		virtual std::string_view committer_email() const noexcept = 0;
		virtual std::string_view message() const noexcept = 0;
		virtual git_time_t commit_time_utc() const noexcept = 0;
		virtual git_time_t add_time_utc() const noexcept = 0;
		virtual io::v1::coverage_stats const& stats() const noexcept = 0;
	};

	ref<report> report_create(git_oid const& parent_report,
	                          git_oid const& file_list,
	                          git_oid const& commit,
	                          std::string const& branch,
	                          std::string const& author_name,
	                          std::string const& author_email,
	                          std::string const& committer_name,
	                          std::string const& committer_email,
	                          std::string const& message,
	                          git_time_t commit_time_utc,
	                          git_time_t add_time_utc,
	                          io::v1::coverage_stats const& stats);

	struct report_entry {
		virtual ~report_entry();
		virtual bool is_dirty() const noexcept = 0;
		bool in_workdir() const noexcept { return is_dirty(); }
		bool in_index() const noexcept { return !is_dirty(); }
		virtual bool is_modified() const noexcept = 0;
		virtual std::string_view path() const noexcept = 0;
		virtual io::v1::coverage_stats const& stats() const noexcept = 0;
		virtual git_oid const* contents() const noexcept = 0;
		virtual git_oid const* line_coverage() const noexcept = 0;
	};

	struct report_entry_builder {
		bool is_dirty{};
		bool is_modified{};
		std::string path{};
		io::v1::coverage_stats stats{};
		git_oid contents{};
		git_oid line_coverage{};

		report_entry_builder& set_dirty(bool value = true) {
			is_dirty = value;
			return *this;
		}

		report_entry_builder& set_modifed(bool value = true) {
			is_modified = value;
			return *this;
		}

		report_entry_builder& set_path(std::string_view value) {
			path.assign(value);
			return *this;
		}

		report_entry_builder& set_stats(io::v1::coverage_stats const& value) {
			stats = value;
			return *this;
		}

		report_entry_builder& set_stats(uint32_t total,
		                                uint32_t relevant,
		                                uint32_t covered) {
			return set_stats({total, relevant, covered});
		}

		report_entry_builder& set_contents(git_oid const& value) {
			contents = value;
			return *this;
		}

		report_entry_builder& set_line_coverage(git_oid const& value) {
			line_coverage = value;
			return *this;
		}

		std::unique_ptr<report_entry> create() &&;
	};

	struct report_files : object {
		obj_type type() const noexcept override { return obj_report_files; };
		bool is_report_files() const noexcept final { return true; }
		virtual std::vector<std::unique_ptr<report_entry>> const& entries()
		    const noexcept = 0;
	};
	ref<report_files> report_files_create(
	    std::vector<std::unique_ptr<report_entry>>&&);

	struct line_coverage : object {
		obj_type type() const noexcept override { return obj_line_coverage; };
		bool is_line_coverage() const noexcept final { return true; }
		virtual std::vector<io::v1::coverage> const& coverage()
		    const noexcept = 0;
	};
	ref<line_coverage> line_coverage_create(std::vector<io::v1::coverage>&&);
}  // namespace cov
