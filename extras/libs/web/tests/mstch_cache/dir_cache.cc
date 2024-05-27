// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)
//
// IT IS NATURAL FOR TEST IN HERE TO FAIL FROM TIME TO TIME. IF THEY SWITCH FROM
// SUCCEEDING TO FAILING, A TEST NEEDS TO BE UPDATED TO REFLECT CHANGE IN THE
// REST OF THE CODE.

#include <fmt/chrono.h>
#include <cov/io/file.hh>
#include <optional>
#include "../setup.hh"
#include "common.hh"

#if __cpp_lib_chrono >= 201907L
#define HAS_CXX20_FILE_CLOCK
#elif defined(__GNUC__)
#define GCC_VER (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#if GCC_VER >= 100100
#define HAS_CXX20_FILE_CLOCK
#endif
#endif

namespace cov::app::web::testing {
	using sys_time = std::chrono::system_clock::time_point;
	using sys_duration = std::chrono::system_clock::duration;

#if defined(HAS_CXX20_FILE_CLOCK)
	sys_time to_system_clock(std::filesystem::file_time_type last_write_time) {
		return std::chrono::file_clock::to_sys(last_write_time);
	}

	std::filesystem::file_time_type to_file_clock(sys_time last_write_time) {
		return std::chrono::file_clock::from_sys(last_write_time);
	}
#else
#ifdef _WIN32
	// on windows, both sys clock and file clock use the same duration, but the
	// epochs are moved apart, by the value below (taken from xfilesystem_abi.h)
	inline static constexpr long long __std_fs_file_time_epoch_adjustment =
	    0x19DB1DED53E8000LL;

	sys_time to_system_clock(std::filesystem::file_time_type last_write_time) {
		using tp = sys_time;
		auto const ticks = last_write_time.time_since_epoch().count() -
		                   __std_fs_file_time_epoch_adjustment;
		auto const since_epoch = tp::duration{ticks};
		return tp{since_epoch};
	}

	std::filesystem::file_time_type to_file_clock(sys_time last_write_time) {
		auto const ticks = last_write_time.time_since_epoch().count() +
		                   __std_fs_file_time_epoch_adjustment;
		auto const since_epoch =
		    std::filesystem::file_time_type::duration{ticks};
		return std::filesystem::file_time_type{since_epoch};
	}
#else
	sys_time to_system_clock(std::filesystem::file_time_type last_write_time) {}

	std::filesystem::file_time_type to_file_clock(sys_time last_write_time) {}
#endif
#endif

	auto now() {
		static auto time_point = std::chrono::system_clock::now();
		return time_point;
	}

	struct touch_options {
		std::string_view path{};
		std::optional<std::string_view> contents{std::nullopt};
		std::optional<sys_duration> timestamp{-10 * 24h};
	};

	static constexpr touch_options files[] = {
	    {"pool1/templates/template1.mustache"sv, "{{> template2}}"sv},
	    {"pool1/templates/template2.mustache"sv, R"(template 2.A)"sv},
	    {"pool2/templates/template1.mustache"sv, "({{> template2}})"sv},
	    {"pool2/templates/template2.mustache"sv, R"(template 2.B)"sv},
	    {"pool3/templates/template1.mustache"sv, "({{> template3}})"sv},
	    {"pool3/templates/template2.mustache"sv, R"(template 2.C)"sv},
	};

	class dir_cache : public ::testing::Test {
	protected:
		static auto const& cache_root() {
			static auto const result = setup::test_dir() / "cache"sv;
			return result;
		}

		void touch(touch_options const& options) {
			auto const path = cache_root() / setup::make_u8path(options.path);
			std::filesystem::create_directories(path.parent_path());

			if (options.contents) {
				auto f = io::fopen(path, "wb");
				if (f)
					f.store(options.contents->data(), options.contents->size());
			} else if (!options.timestamp) {
				if (!io::fopen(path, "w")) return;
			}

			if (options.timestamp) {
				auto const timestamp =
				    to_file_clock(now() + *options.timestamp);
				std::filesystem::last_write_time(path, timestamp);
			}
		}

		void SetUp() override {
			std::filesystem::remove_all(cache_root());
			for (auto const& file : files) {
				touch(file);
			}
		}

		void TearDown() override { std::filesystem::remove_all(cache_root()); }
	};

	TEST_F(dir_cache, base) {
		web::dir_cache cache(
		    {cache_root() / "pool1"sv, cache_root() / "pool2"sv,
		     cache_root() / "pool3"sv},
		    "templates"sv);
		auto const actual = cache.render("template1", mstch::map{});
		ASSERT_EQ("template 2.A"sv, actual);
	}

	TEST_F(dir_cache, updated_file) {
		web::dir_cache cache(
		    {cache_root() / "pool1"sv, cache_root() / "pool2"sv,
		     cache_root() / "pool3"sv},
		    "templates"sv);
		touch(
		    {.path = "pool2/templates/template1.mustache", .timestamp = -24h});
		auto const actual = cache.render("template1", mstch::map{});
		ASSERT_EQ("(template 2.A)"sv, actual);
	}

	TEST_F(dir_cache, missing_partial) {
		web::dir_cache cache(
		    {cache_root() / "pool1"sv, cache_root() / "pool2"sv,
		     cache_root() / "pool3"sv},
		    "templates"sv);
		touch(
		    {.path = "pool3/templates/template1.mustache", .timestamp = -24h});
		auto const actual = cache.render("template1", mstch::map{});
		ASSERT_EQ("()"sv, actual);
	}

	TEST_F(dir_cache, switch_at_runtime) {
		web::dir_cache cache(
		    {cache_root() / "pool1"sv, cache_root() / "pool2"sv,
		     cache_root() / "pool3"sv},
		    "templates"sv);
		ASSERT_EQ("template 2.A"sv, cache.render("template1", mstch::map{}));
		touch(
		    {.path = "pool3/templates/template1.mustache", .timestamp = -24h});
		auto const actual = cache.render("template1", mstch::map{});
		ASSERT_EQ("()"sv, actual);
	}
}  // namespace cov::app::web::testing
