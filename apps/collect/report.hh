// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/git2/oid.hh>
#include <cov/io/types.hh>
#include <filesystem>
#include <json/json.hpp>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <toolkit.hh>
#include <vector>

namespace cov::app::collect {
	struct report : coverage {
		report(config const& cfg) : cfg_{cfg} {}
		coverage_file* get_file(std::string_view path) override;
		std::unique_ptr<coverage_file> get_file_mt(
		    std::string_view path) override;
		void handover_mt(std::unique_ptr<coverage_file>&&) override;

		void post_process();
		io::v1::coverage_stats summary() const;
		json::node get_json(json::string const& branch, git::oid_view ref);

	private:
		std::string build_prefix() const;

		struct function_cvg {
			cov::app::collect::text_pos start;
			cov::app::collect::text_pos stop;
			unsigned count;
			std::string name;
			std::string demangled;

			json::node get_json() const;
			auto operator<=>(function_cvg const&) const noexcept = default;
		};

		struct file : coverage_file {
			file(std::string const& filename) : filename_{filename} {}

			void on_visited(unsigned line, unsigned count) override;
			void on_function(text_pos const& start,
			                 text_pos const& stop,
			                 unsigned count,
			                 std::string const& name,
			                 std::string const& demangled) override;

			void merge(file& src);

			std::string const& filename() const noexcept { return filename_; }

			std::map<unsigned, unsigned> const& lines() const noexcept {
				return lines_;
			}
			std::vector<function_cvg> const& functions() const noexcept {
				return functions_;
			}

			void post_process(std::filesystem::path const& path);
			json::node get_json(std::string_view filename) const;
			json::node get_lines() const;
			json::node get_functions() const;

		private:
			std::string filename_;
			std::map<unsigned, unsigned> lines_{};
			std::vector<function_cvg> functions_{};
			std::string hash_{};
		};

		std::mutex m_;
		config const& cfg_;
		std::string src_prefix_{build_prefix()};
		std::map<std::string, std::unique_ptr<file>> files_{};
	};
}  // namespace cov::app::collect
