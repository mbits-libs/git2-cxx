// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <charconv>
#include <cov/repository.hh>
#include "internal_environment.hh"

using namespace std::literals;

namespace cov::placeholder {
	namespace {
		class build_facade : public object_facade {
		public:
			build_facade(cov::report::build const* short_view,
			             cov::repository const* repo)
			    : short_view_{short_view}, repo_{repo} {}

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

			iterator prop(iterator out,
			              bool wrapped,
			              bool magic_colors,
			              internal_environment& env) override {
				std::string_view props = props_json();
				auto const properties = cov::report::build::properties(props);
				return context{this}.format_properties(
				    out, env, wrapped, magic_colors, properties);
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
			git::oid const* tertiary_id() noexcept override { return nullptr; }
			git::oid const* quaternary_id() noexcept override {
				return nullptr;
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
			git_commit_view const* git() noexcept override { return nullptr; }

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
			report_facade(cov::report const* data, cov::repository const* repo)
			    : data_{data}, repo_{repo} {}

			std::unique_ptr<object_list> loop(std::string_view ref) override {
				if (ref == "B"sv || ref == "build"sv) {
					return std::make_unique<build_list>(data_, repo_);
				}
				return {};
			}

			iterator prop(iterator out,
			              bool wrapped,
			              bool magic_colors,
			              internal_environment& env) override {
				return env.client->names.format(out, primary_id(), wrapped,
				                                magic_colors, this, env);
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
				build_list(cov::report const* data, cov::repository const* repo)
				    : data_{data}, repo_{repo} {}
				std::unique_ptr<struct object_facade> next() override {
					auto const& entries = data_->entries();
					if (index_ >= entries.size()) return {};
					auto const& entry = entries[index_];
					++index_;
					return std::make_unique<build_facade>(entry.get(), repo_);
				}

			private:
				size_t index_{0};
				cov::report const* data_{};
				cov::repository const* repo_{};
			};

			cov::report const* data_{};
			cov::repository const* repo_{};
			git_commit_view view_{setup_view()};
		};
	}  // namespace

	object_list::~object_list() = default;

	object_facade::~object_facade() = default;

	bool object_facade::condition(std::string_view ref) {
#define STATS(REF, OBJ)                        \
	if (ref == REF) {                          \
		auto const* info = stats();            \
		return info && info->OBJ.relevant > 0; \
	}
		if (ref == "pL"sv) {
			auto const* info = stats();
			return info && info->lines_total > 0;
		}
		STATS("pTL"sv, lines);
		STATS("pTF"sv, functions);
		STATS("pTB"sv, branches);
		auto const list = loop(ref);
		return list && list->next();
	}

	std::unique_ptr<object_facade> object_facade::present_report(
	    cov::report const* data,
	    cov::repository const* repo) {
		return std::make_unique<report_facade>(data, repo);
	}

	std::unique_ptr<object_facade> object_facade::present_build(
	    cov::report::build const* data,
	    cov::repository const* repo) {
		return std::make_unique<build_facade>(data, repo);
	}
}  // namespace cov::placeholder
