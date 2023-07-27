// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include "toolkit.hh"

#include <fmt/format.h>
#include <array>
#include <cov/app/path.hh>
#include <cov/app/run.hh>
#include <cov/git2/config.hh>
#include <native/str.hh>
#include <walk.hh>

using namespace std::literals;

namespace cov::app::collect {
	std::unique_ptr<toolkit> llvm(config const& cfg,
	                              std::span<std::string_view const> lines);
	std::unique_ptr<toolkit> gnu(config const& cfg,
	                             std::span<std::string_view const> lines);
	std::unique_ptr<toolkit> msvc(config const& cfg);

	static constexpr std::array toolkits = {gnu, llvm};

	coverage_file::~coverage_file() = default;
	coverage::~coverage() = default;
	toolkit::~toolkit() = default;

	bool toolkit::needs_preprocess() const { return false; }
	int toolkit::preprocess(config const&) { return 0; }

	bool toolkit::find_tool(std::filesystem::path& dst,
	                        std::filesystem::path const& hint,
	                        std::string_view tool,
	                        std::span<std::string_view const> version) {
		std::vector<std::string> names{};
		names.reserve(version.size() + 1);
		while (!version.empty()) {
			names.push_back(
			    fmt::format("{}-{}", tool, fmt::join(version, ".")));
			version = version.subspan(0, version.size() - 1);
		}
		names.push_back(fmt::format("{}", tool));
		auto result = platform::find_program(names, hint);
		if (!result) return false;
		dst = std::move(*result);
		return true;
	}

	void toolkit::remove_all(std::filesystem::path const& mask) {
		walk(mask, [&](std::filesystem::path const& path) {
			std::error_code ignore{};
			// fmt::print(stderr, "{}\n", get_u8path(path));
			std::filesystem::remove(path, ignore);
		});
	}

	std::unique_ptr<toolkit> recognize_toolkit(config const& cfg) {
#ifdef _WIN32
		if (cfg.compiler.filename() == L"cl.exe"sv) return msvc(cfg);
#endif
		char ver[] = "--version";
		char* v_args[] = {ver, nullptr};
		auto const proc = platform::run(cfg.compiler, {1, v_args});
		if (proc.return_code) return {};

		auto view = trim({reinterpret_cast<char const*>(proc.output.data()),
		                  proc.output.size()});
		auto lines = split(view, '\n');

		for (auto toolkit : toolkits) {
			auto result = toolkit(cfg, lines);
			if (result) return result;
		}

		fmt::print(
		    stderr,
		    "[toolkit] could not find toolkit compatible with the compiler;\n"
		    "[toolkit]   called: `{} --version`\n",
		    get_u8path(cfg.compiler));
		if (!lines.empty() && !lines.front().empty())
			fmt::print(stderr, "[toolkit]   first line: `{}`\n", lines.front());

		return {};
	}

	void config::load(std::filesystem::path const& filename) {
		cfg_dir = std::filesystem::weakly_canonical(filename.parent_path());
		compiler.clear();
		bin_dir.clear();
		src_dir.clear();
		include.clear();
		exclude.clear();
		output.clear();

		auto cfg = git::config::create();
		cfg.add_file_ondisk(filename);
		cfg.foreach_entry([this](git_config_entry const* entry) -> int {
			auto view = std::string_view{entry->name};
			if (view.starts_with("collect."sv)) {
				auto key = view.substr("collect."sv.length());

				if (key == "compiler"sv) {
					this->compiler = make_u8path(entry->value);
					return 0;
				}

				if (key == "output"sv) {
					this->output = make_u8path(entry->value);
					return 0;
				}

				if (key == "bin-dir"sv) {
					this->bin_dir = std::filesystem::weakly_canonical(
					    this->cfg_dir / make_u8path(entry->value));
					return 0;
				}

				if (key == "src-dir"sv) {
					this->src_dir = std::filesystem::weakly_canonical(
					    this->cfg_dir / make_u8path(entry->value));
					return 0;
				}

				if (key == "include"sv) {
					this->include.push_back(make_u8path(entry->value));
					return 0;
				}

				if (key == "exclude"sv) {
					this->exclude.push_back(make_u8path(entry->value));
					return 0;
				}
			}

			return 0;
		});

		if (bin_dir.empty()) bin_dir = cfg_dir;
		if (src_dir.empty()) src_dir = cfg_dir;
		if (output.empty()) output = "cov-collect.json";
	}
}  // namespace cov::app::collect
