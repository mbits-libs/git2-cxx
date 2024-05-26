// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <filesystem>
#include <map>
#include <mstch/mstch.hpp>
#include <string>
#include <vector>

namespace cov::app::web {
	class dir_cache : public mstch::cache {
	public:
		explicit dir_cache(std::vector<std::filesystem::path> const& roots,
		                   std::filesystem::path const& resdir)
		    : roots_{roots}, res_{resdir} {}
		std::pair<std::filesystem::file_time_type, std::string> load_ex(
		    std::string const& partial);
		std::string load(std::string const& partial) override;
		bool need_update(std::string const& partial) const override;
		bool is_valid(std::string const& partial) const override;

	private:
		std::vector<std::filesystem::path> roots_;
		std::filesystem::path res_;
		std::map<std::string, std::filesystem::file_time_type> cache_;
	};

#define MSTCH_LNG(X)     \
	X(COVERAGE_BRANDING) \
	X(COVERAGE_BUILDS)   \
	X(COMMIT_AUTHOR)     \
	X(COMMIT_COMMITTER)  \
	X(COMMIT_ID)         \
	X(COMMIT_BRANCH)     \
	X(FILE_LINES)        \
	X(FILE_NAME)         \
	X(FILE_COVERAGE)     \
	X(FILE_LINES_TOTAL)  \
	X(FILE_RELEVANT)     \
	X(FILE_COVERED)      \
	X(FILE_MISSED)       \
	X(ROW_TOTAL)

	enum class lng {
#define X(LNG) LNG,
		MSTCH_LNG(X)
#undef X
	};

	struct lng_provider {
		virtual ~lng_provider();
		virtual std::string_view get(lng) const noexcept = 0;
	};

	template <typename T>
	using non_owning_ptr = T const*;

	template <typename T, typename... A>
	    requires requires(A&&... args) { new T(std::forward<A>(args)...); }
	inline std::shared_ptr<T> mk_shared(A&&... args) {
		return std::make_shared<T>(std::forward<A>(args)...);
	}

	class lng_callback : public mstch::callback {
		mutable std::unordered_map<lng, mstch::node> storage_;
		mutable std::unordered_map<std::string, mstch::node> str_storage_;
		non_owning_ptr<lng_provider> provider_;

	public:
		lng_callback(non_owning_ptr<lng_provider> provider)
		    : provider_{provider} {}
		mstch::node const& at(std::string const& name) const override;
		bool has(std::string const& name) const override;

		std::vector<std::string> debug_all_keys() const override {
			return {
#define X(LNG) "LNG_" #LNG,
			    MSTCH_LNG(X)
#undef X
			};
		}
		static auto create(non_owning_ptr<lng_provider> provider) {
			return mk_shared<lng_callback>(provider);
		}
	};

	class octicon_callback : public mstch::callback {
		std::filesystem::path const json_path_;
		mutable std::map<std::string, mstch::node> storage_;

	public:
		octicon_callback(std::filesystem::path const& json_path)
		    : json_path_{json_path} {}
		mstch::node const& at(std::string const& name) const override;
		bool has(std::string const& name) const override;
		std::vector<std::string> debug_all_keys() const override;
		static auto create(std::filesystem::path const& json_path) {
			return mk_shared<octicon_callback>(json_path);
		}
	};
}  // namespace cov::app::web
