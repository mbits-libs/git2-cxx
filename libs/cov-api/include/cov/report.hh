// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <cov/git2/oid.hh>
#include <cov/io/types.hh>
#include <cov/object.hh>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace cov {
	struct repository;

	struct object_with_id : object {
		bool is_object_with_id() const noexcept final { return true; }
		virtual git::oid const& oid() const noexcept = 0;
	};

	struct report : object_with_id {
		using property = std::variant<std::string, long long, bool>;
		struct build {
			virtual ~build();
			virtual git::oid const& build_id() const noexcept = 0;
			virtual std::string_view props_json() const noexcept = 0;
			virtual io::v1::coverage_stats const& stats() const noexcept = 0;
			static std::map<std::string, property> properties(
			    std::string_view props);
		};

		obj_type type() const noexcept override { return obj_report; };
		bool is_report() const noexcept final { return true; }
		virtual git::oid const& parent_id() const noexcept = 0;
		virtual git::oid const& file_list_id() const noexcept = 0;
		virtual git::oid const& commit_id() const noexcept = 0;
		virtual std::string_view branch() const noexcept = 0;
		virtual std::string_view author_name() const noexcept = 0;
		virtual std::string_view author_email() const noexcept = 0;
		virtual std::string_view committer_name() const noexcept = 0;
		virtual std::string_view committer_email() const noexcept = 0;
		virtual std::string_view message() const noexcept = 0;
		virtual sys_seconds commit_time_utc() const noexcept = 0;
		virtual sys_seconds add_time_utc() const noexcept = 0;
		virtual io::v1::coverage_stats const& stats() const noexcept = 0;
		virtual std::span<std::unique_ptr<build> const> entries()
		    const noexcept = 0;

		struct signature_view {
			std::string_view name;
			std::string_view email;
		};

		static ref_ptr<report> create(
		    git::oid_view parent,
		    git::oid_view file_list,
		    git::oid_view commit,
		    std::string_view branch,
		    signature_view author,
		    signature_view committer,
		    std::string_view message,
		    sys_seconds commit_time_utc,
		    sys_seconds add_time_utc,
		    io::v1::coverage_stats const& stats,
		    std::vector<std::unique_ptr<build>>&& builds) {
			return create(git::oid{}, parent, file_list, commit, branch, author,
			              committer, message, commit_time_utc, add_time_utc,
			              stats, std::move(builds));
		}

		static ref_ptr<report> create(
		    git::oid_view oid,
		    git::oid_view parent,
		    git::oid_view file_list,
		    git::oid_view commit,
		    std::string_view branch,
		    signature_view author,
		    signature_view committer,
		    std::string_view message,
		    sys_seconds commit_time_utc,
		    sys_seconds add_time_utc,
		    io::v1::coverage_stats const& stats,
		    std::vector<std::unique_ptr<build>>&& reports);

		class builder {
		public:
			struct info {
				git::oid build_id{};
				std::string_view props{};
				io::v1::coverage_stats stats{};
			};
			builder& add(std::unique_ptr<build>&&);
			builder& add(git::oid_view build_id,
			             std::string_view props,
			             io::v1::coverage_stats stats);
			builder& add_nfo(info const& nfo) {
				return add(nfo.build_id, nfo.props, nfo.stats);
			}
			bool remove(std::string_view props);
			std::vector<std::unique_ptr<build>> release();
			std::map<std::string, std::unique_ptr<build>> const& debug()
			    const noexcept {
				return entries_;
			}

			static std::string properties(
			    std::span<std::string const> const& propset);
			static std::string normalize(std::string_view props);

		private:
			std::map<std::string, std::unique_ptr<build>> entries_{};
		};
	};

	struct build : object_with_id {
		obj_type type() const noexcept override { return obj_build; };
		bool is_build() const noexcept final { return true; }
		virtual git::oid const& file_list_id() const noexcept = 0;
		virtual sys_seconds add_time_utc() const noexcept = 0;
		virtual std::string_view props_json() const noexcept = 0;
		virtual io::v1::coverage_stats const& stats() const noexcept = 0;

		static ref_ptr<build> create(git::oid_view file_list,
		                             sys_seconds add_time_utc,
		                             std::string_view props_json,
		                             io::v1::coverage_stats const& stats) {
			return create(git::oid{}, file_list, add_time_utc, props_json,
			              stats);
		}

		static ref_ptr<build> create(git::oid_view oid,
		                             git::oid_view file_list,
		                             sys_seconds add_time_utc,
		                             std::string_view props_json,
		                             io::v1::coverage_stats const& stats);
	};

	struct files : object_with_id {
		struct entry {
			virtual ~entry();
			virtual std::string_view path() const noexcept = 0;
			virtual io::v1::coverage_stats const& stats() const noexcept = 0;
			virtual git::oid const& contents() const noexcept = 0;
			virtual git::oid const& line_coverage() const noexcept = 0;
			virtual git::oid const& function_coverage() const noexcept = 0;
			virtual git::oid const& branch_coverage() const noexcept = 0;
			virtual std::vector<std::byte> get_contents(
			    repository const&,
			    std::error_code&) const noexcept = 0;
		};

		obj_type type() const noexcept override { return obj_files; };
		bool is_files() const noexcept final { return true; }
		virtual std::span<std::unique_ptr<entry> const> entries()
		    const noexcept = 0;
		virtual entry const* by_path(std::string_view path) const noexcept = 0;

		static ref_ptr<files> create(git::oid_view oid,
		                             std::vector<std::unique_ptr<entry>>&&);

		class builder {
		public:
			struct info {
				std::string_view path{};
				io::v1::coverage_stats stats{};
				git::oid contents{};
				git::oid line_coverage{};
				git::oid function_coverage{};
				git::oid branch_coverage{};
			};
			builder& add(std::unique_ptr<entry>&&);
			builder& add(std::string_view path,
			             io::v1::coverage_stats const& stats,
			             git::oid_view contents,
			             git::oid_view line_coverage,
			             git::oid_view function_coverage,
			             git::oid_view branch_coverage);
			builder& add_nfo(info const& nfo) {
				return add(nfo.path, nfo.stats, nfo.contents, nfo.line_coverage,
				           nfo.function_coverage, nfo.branch_coverage);
			}
			bool remove(std::string_view path);
			ref_ptr<files> extract(git::oid_view oid);
			std::vector<std::unique_ptr<entry>> release();

		private:
			std::map<std::string_view, std::unique_ptr<entry>> entries_{};
		};
	};

	struct line_coverage : object {
		obj_type type() const noexcept override { return obj_line_coverage; };
		bool is_line_coverage() const noexcept final { return true; }
		virtual std::span<io::v1::coverage const> coverage() const noexcept = 0;

		static ref_ptr<line_coverage> create(std::vector<io::v1::coverage>&&);
	};

	struct function_coverage : object {
		struct entry {
			virtual ~entry();
			virtual std::string_view name() const noexcept = 0;
			virtual std::string_view demangled_name() const noexcept = 0;
			virtual std::uint32_t count() const noexcept = 0;
			virtual io::v1::text_pos const& start() const noexcept = 0;
			virtual io::v1::text_pos const& end() const noexcept = 0;
		};

		struct function_label {
			io::v1::text_pos start{};
			io::v1::text_pos end{};
			std::string name{};

			auto operator<=>(function_label const&) const noexcept = default;
		};
		struct function {
			function_label label{};
			unsigned count{};

			auto operator<=>(function const&) const noexcept = default;
		};

		struct function_iterator {
			function_iterator() = delete;
			function_iterator(std::vector<function> const& functions)
			    : it_{functions.begin()}, end_{functions.end()} {}

			template <typename Callback>
			void before(std::uint32_t line, Callback&& cb) {
				while (it_ != end_ && it_->label.start.line < line)
					cb(*it_++);
			}

			template <typename Callback>
			void at(std::uint32_t line, Callback&& cb) {
				while (it_ != end_ && it_->label.start.line < line)
					++it_;
				while (it_ != end_ && it_->label.start.line == line) {
					cb(*it_++);
				}
			}

			template <typename Callback>
			void rest(Callback&& cb) {
				while (it_ != end_)
					cb(*it_++);
			}

		private:
			std::vector<function>::const_iterator it_;
			std::vector<function>::const_iterator end_;
		};

		obj_type type() const noexcept override {
			return obj_function_coverage;
		};
		bool is_function_coverage() const noexcept final { return true; }
		virtual std::span<std::unique_ptr<entry> const> entries()
		    const noexcept = 0;
		std::vector<function> merge_aliases() const;

		static ref_ptr<function_coverage> create(
		    std::vector<std::unique_ptr<entry>>&&);

		class builder {
		public:
			struct info {
				std::string_view name{};
				std::string_view demangled_name{};
				std::uint32_t count{};
				io::v1::text_pos start{};
				io::v1::text_pos end{};
			};
			builder& add(std::unique_ptr<entry>&&);
			builder& add(std::string_view name,
			             std::string_view demangled_name,
			             std::uint32_t count,
			             io::v1::text_pos const& start,
			             io::v1::text_pos const& end);
			builder& add_nfo(info const& nfo) {
				return add(nfo.name, nfo.demangled_name, nfo.count, nfo.start,
				           nfo.end);
			}
			ref_ptr<function_coverage> extract();
			std::vector<std::unique_ptr<entry>> release();

			void reserve(size_t count) { entries_.reserve(count); }

		private:
			std::vector<std::unique_ptr<entry>> entries_{};
		};
	};
}  // namespace cov
