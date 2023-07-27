// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <args/parser.hpp>
#include <filesystem>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace cov::app::collect {
	struct config {
		std::filesystem::path cfg_dir;
		std::filesystem::path compiler;
		std::filesystem::path bin_dir;
		std::filesystem::path src_dir;
		std::filesystem::path output;
		std::vector<std::filesystem::path> include;
		std::vector<std::filesystem::path> exclude;

		void load(std::filesystem::path const& filename);
	};

	struct text_pos {
		unsigned line{0};
		unsigned col{0};
		auto operator<=>(text_pos const&) const noexcept = default;
	};

	struct coverage_file {
		virtual ~coverage_file();
		virtual void on_visited(unsigned line, unsigned count) = 0;
		virtual void on_function(text_pos const& start,
		                         text_pos const& stop,
		                         unsigned count,
		                         std::string const& name,
		                         std::string const& demangled) = 0;
	};

	struct coverage {
		virtual ~coverage();
		virtual coverage_file* get_file(std::string_view path) = 0;
		virtual std::unique_ptr<coverage_file> get_file_mt(
		    std::string_view path) = 0;
		virtual void handover_mt(std::unique_ptr<coverage_file>&&) = 0;
	};

	struct toolkit {
		virtual ~toolkit();
		virtual std::string_view label() const noexcept = 0;
		virtual std::string_view version() const noexcept = 0;
		virtual void hello(config const&) const = 0;
		virtual void clean(config const& cfg) const = 0;
		virtual int observe(config const& cfg,
		                    std::string_view command,
		                    args::arglist arguments) const = 0;
		virtual bool needs_preprocess() const;
		virtual int preprocess(config const&);
		virtual int report(config const&, coverage&) = 0;

		static bool find_tool(std::filesystem::path& dst,
		                      std::filesystem::path const& hint,
		                      std::string_view tool,
		                      std::span<std::string_view const> version);
		static void remove_all(std::filesystem::path const& mask);
	};

	struct arguments {
		explicit arguments(std::span<std::string> stg) {
			mem_.reserve(stg.size());
			std::transform(stg.begin(), stg.end(), std::back_inserter(mem_),
			               [](auto& s) { return s.data(); });
		}

		operator ::args::arglist() noexcept {
			return {static_cast<unsigned>(mem_.size()), mem_.data()};
		}

	public:
		std::vector<char*> mem_;
	};

	std::unique_ptr<toolkit> recognize_toolkit(config const& cfg);
}  // namespace cov::app::collect
