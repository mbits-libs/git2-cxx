// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <charconv>
#include <cov/repository.hh>
#include "internal_environment.hh"

using namespace std::literals;

namespace cov::placeholder {
	namespace {

		class file_facade : public object_facade {
		public:
			file_facade(cov::files::entry const* data,
			            cov::repository const* repo)
			    : data_{data}, repo_{repo} {}

			std::string_view name() const noexcept override { return "blob"sv; }
			std::string_view secondary_label() const noexcept override {
				return "lines"sv;
			}
			std::string_view tertiary_label() const noexcept override {
				return "functions"sv;
			}
			std::string_view quaternary_label() const noexcept override {
				return "branches"sv;
			}

			git::oid const* primary_id() noexcept override {
				return data_ ? &data_->contents() : nullptr;
			}
			git::oid const* secondary_id() noexcept override {
				return data_ && !data_->line_coverage().is_zero()
				           ? &data_->line_coverage()
				           : nullptr;
			}
			git::oid const* tertiary_id() noexcept override {
				return data_ && !data_->function_coverage().is_zero()
				           ? &data_->function_coverage()
				           : nullptr;
			}
			git::oid const* quaternary_id() noexcept override {
				return data_ && !data_->branch_coverage().is_zero()
				           ? &data_->branch_coverage()
				           : nullptr;
			}
			io::v1::coverage_stats const* stats() noexcept override {
				return data_ ? &data_->stats() : nullptr;
			}

		private:
			cov::files::entry const* data_{};
			cov::repository const* repo_{};
		};

		class files_facade : public object_facade {
		public:
			files_facade(ref_ptr<cov::files> const& data,
			             cov::repository const* repo)
			    : data_{data}, repo_{repo} {}

			std::string_view name() const noexcept override {
				return "files"sv;
			}

			git::oid const* primary_id() noexcept override {
				return data_ ? &data_->oid() : nullptr;
			}

		private:
			ref_ptr<cov::files> data_{};
			cov::repository const* repo_{};
		};

		class build_facade : public object_facade {
		public:
			build_facade(cov::report::build const* short_view,
			             ref_ptr<cov::build> const& full_view,
			             cov::repository const* repo)
			    : data_{full_view}, short_view_{short_view}, repo_{repo} {}

			std::string_view name() const noexcept override {
				return "build"sv;
			}
			std::string_view secondary_label() const noexcept override {
				return "files"sv;
			}
			std::unique_ptr<object_list> loop(std::string_view) override {
				return {};
			}

			bool condition(std::string_view ref) override {
				if (ref == "prop"sv || ref.empty()) {
					std::string_view props = props_json();
					auto const properties =
					    cov::report::build::properties(props);
					return !properties.empty();
				}
				return object_facade::condition(ref);
			}

			iterator props(iterator out,
			               bool wrapped,
			               bool magic_colors,
			               unsigned indent,
			               internal_environment& env) override {
				std::string_view props = props_json();
				auto const properties = cov::report::build::properties(props);
				return context{this}.format_properties(
				    out, env, wrapped, magic_colors, indent, properties);
			}
			git::oid const* primary_id() noexcept override {
				return data_         ? &data_->oid()
				       : short_view_ ? &short_view_->build_id()
				                     : nullptr;
			}
			git::oid const* secondary_id() noexcept override {
				ensure_data();
				return data_ ? &data_->file_list_id() : nullptr;
			}
			sys_seconds added() noexcept override {
				ensure_data();
				return data_ ? data_->add_time_utc() : sys_seconds{};
			}
			io::v1::coverage_stats const* stats() noexcept override {
				return data_         ? &data_->stats()
				       : short_view_ ? &short_view_->stats()
				                     : nullptr;
			}

		private:
			void ensure_data() noexcept {
				if (data_) return;
				if (!short_view_ || !repo_) return;
				std::error_code ec{};
				data_ = repo_->lookup<cov::build>(short_view_->build_id(), ec);
				if (ec) data_.reset();
			}

			std::string_view props_json() {
				if (data_)
					return data_->props_json();
				else if (short_view_)
					return short_view_->props_json();
				return {};
			}

			ref_ptr<cov::build> data_{};
			cov::report::build const* short_view_{};
			cov::repository const* repo_{};
		};

		class report_facade : public object_facade {
		public:
			report_facade(ref_ptr<cov::report> const& data,
			              cov::repository const* repo)
			    : data_{data}, repo_{repo} {}

			std::string_view name() const noexcept override {
				return "report"sv;
			}
			std::string_view secondary_label() const noexcept override {
				return "files"sv;
			}
			std::string_view tertiary_label() const noexcept override {
				return "parent"sv;
			}
			std::string_view quaternary_label() const noexcept override {
				return "commit"sv;
			}

			std::unique_ptr<object_list> loop(std::string_view ref) override {
				if (ref == "B"sv || ref == "build"sv) {
					return std::make_unique<build_list>(data_, repo_);
				}
				return {};
			}

			iterator refs(iterator out,
			              bool wrapped,
			              bool magic_colors,
			              unsigned indent,
			              internal_environment& env) override {
				return env.client->names.format(out, primary_id(), wrapped,
				                                magic_colors, indent, this,
				                                env);
			}
			git::oid const* primary_id() noexcept override {
				return data_ ? &data_->oid() : nullptr;
			}
			git::oid const* secondary_id() noexcept override {
				return data_ && !data_->file_list_id().is_zero()
				           ? &data_->file_list_id()
				           : nullptr;
			}
			git::oid const* tertiary_id() noexcept override {
				return data_ && !data_->parent_id().is_zero()
				           ? &data_->parent_id()
				           : nullptr;
			}
			git::oid const* quaternary_id() noexcept override {
				return data_ && !data_->commit_id().is_zero()
				           ? &data_->commit_id()
				           : nullptr;
			}
			git::oid const* parent_id() noexcept override {
				return data_ && !data_->parent_id().is_zero()
				           ? &data_->parent_id()
				           : nullptr;
			}

			sys_seconds added() noexcept override {
				return data_ ? data_->add_time_utc() : sys_seconds{};
			}
			io::v1::coverage_stats const* stats() noexcept override {
				return data_ ? &data_->stats() : nullptr;
			}
			git_commit_view const* git() noexcept override {
				return data_ ? &view_ : nullptr;
			}

		private:
			git_commit_view setup_view() const noexcept {
				if (!data_) return {};
				return git_commit_view::from(*data_);
			}

			class build_list : public object_list {
			public:
				build_list(ref_ptr<cov::report> const& data,
				           cov::repository const* repo)
				    : data_{data}, repo_{repo} {}
				std::unique_ptr<struct object_facade> next() override {
					auto const& entries = data_->entries();
					if (index_ >= entries.size()) return {};
					auto const& entry = entries[index_];
					++index_;
					return std::make_unique<build_facade>(entry.get(), nullptr,
					                                      repo_);
				}

			private:
				size_t index_{0};
				ref_ptr<cov::report> data_{};
				cov::repository const* repo_{};
			};

			ref_ptr<cov::report> data_{};
			cov::repository const* repo_{};
			git_commit_view view_{setup_view()};
		};
	}  // namespace

	object_list::~object_list() = default;

	object_facade::~object_facade() = default;

	std::string_view object_facade::secondary_label() const noexcept {
		return {};
	}

	std::string_view object_facade::tertiary_label() const noexcept {
		return {};
	}

	std::string_view object_facade::quaternary_label() const noexcept {
		return {};
	}

	std::unique_ptr<object_list> object_facade::loop(std::string_view) {
		return {};
	}

	bool object_facade::condition(std::string_view ref) {
#define STATS(REF, OBJ)                        \
	if (ref == REF) {                          \
		auto const* info = stats();            \
		return info && info->OBJ.relevant > 0; \
	}
#define HAS_HASH(REF, METHOD)             \
	if (ref == REF) {                     \
		auto const* hash = METHOD##_id(); \
		return hash && !hash->is_zero();  \
	}
		if (ref == "pT"sv) {
			return !!stats();
		}
		if (ref == "pL"sv) {
			auto const* info = stats();
			return info && info->lines_total > 0;
		}
		STATS("pTL"sv, lines);
		STATS("pTF"sv, functions);
		STATS("pTB"sv, branches);
		HAS_HASH("H1"sv, primary);
		HAS_HASH("H2"sv, secondary);
		HAS_HASH("H3"sv, tertiary);
		HAS_HASH("H4"sv, quaternary);
		if (ref == "rd"sv) {
			return added() != sys_seconds{};
		}
		if (ref == "git"sv) {
			return !!git();
		}
		auto const list = loop(ref);
		return list && list->next();
	}

	iterator object_facade::refs(iterator out,
	                             bool,
	                             bool,
	                             unsigned,
	                             internal_environment&) {
		return out;
	}

	iterator object_facade::props(iterator out,
	                              bool,
	                              bool,
	                              unsigned,
	                              internal_environment&) {
		return out;
	}

	git::oid const* object_facade::secondary_id() noexcept { return nullptr; }

	git::oid const* object_facade::tertiary_id() noexcept { return nullptr; }

	git::oid const* object_facade::quaternary_id() noexcept { return nullptr; }

	git::oid const* object_facade::parent_id() noexcept { return nullptr; }

	std::chrono::sys_seconds object_facade::added() noexcept { return {}; }

	io::v1::coverage_stats const* object_facade::stats() noexcept {
		return nullptr;
	}

	git_commit_view const* object_facade::git() noexcept { return nullptr; }

	std::unique_ptr<object_facade> object_facade::present_oid(
	    git::oid_view id,
	    cov::repository const& repo) {
		std::error_code ec{};
		auto generic = repo.lookup<cov::object>(id, ec);
		if (!generic || ec) return {};

		if (auto report = as_a<cov::report>(generic); report) {
			return present_report(report, &repo);
		}

		if (auto build = as_a<cov::build>(generic); build) {
			return present_build(build, &repo);
		}

		if (auto files = as_a<cov::files>(generic); files) {
			return present_files(files, &repo);
		}

		return {};
	}
	std::unique_ptr<object_facade> object_facade::present_report(
	    ref_ptr<cov::report> const& data,
	    cov::repository const* repo) {
		return std::make_unique<report_facade>(data, repo);
	}

	std::unique_ptr<object_facade> object_facade::present_build(
	    cov::report::build const* data,
	    cov::repository const* repo) {
		return std::make_unique<build_facade>(data, nullptr, repo);
	}

	std::unique_ptr<object_facade> object_facade::present_build(
	    ref_ptr<cov::build> const& data,
	    cov::repository const* repo) {
		return std::make_unique<build_facade>(nullptr, data, repo);
	}

	std::unique_ptr<object_facade> object_facade::present_files(
	    ref_ptr<cov::files> const& data,
	    cov::repository const* repo) {
		return std::make_unique<files_facade>(data, repo);
	}

	std::unique_ptr<object_facade> object_facade::present_file(
	    cov::files::entry const* data,
	    cov::repository const* repo) {
		return std::make_unique<file_facade>(data, repo);
	}
}  // namespace cov::placeholder
