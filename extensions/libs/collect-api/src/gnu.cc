// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <atomic>
#include <cov/app/path.hh>
#include <cov/app/run.hh>
#include <cov/git2/config.hh>
#include <ctre.hpp>
#include <json/json.hpp>
#include <native/str.hh>
#include <string_view>
#include <task_watcher.hh>
#include <thread>
#include <thread_pool.hh>
#include <toolkit.hh>
#include <unordered_set>
#include <walk.hh>

using namespace std::literals;

// Alpine: g++ (GCC) 13.1.1 20230429
// Jammy: g++ (Ubuntu 12.1.0-2ubuntu1~22.04) 12.1.0
// MINGW-w64: g++.exe (Rev7, Built by MSYS2 project) 13.1.0
#define GXX "g\\+\\+"
#define WORD "[a-zA-Z0-9_]"
#define COMMENT R"(\([^)]+\))"
#define VERSION "[0-9.]+"
static constexpr auto GNU =
    ctre::match<"^(?:" GXX "(?:.exe)?|" WORD "+-" WORD "+-" WORD "+-" GXX
                "(?:-" VERSION ")?) " COMMENT " (" VERSION ")(?: [0-9]+)?$">;

static constexpr auto TRIPLET = ctre::match<
    // the triplet
    "^(" WORD "+-" WORD "+-" WORD "+-)" GXX "(?:-" VERSION ")? \\(.*$">;

namespace cov::app::collect {
	static unsigned u(long long val) {
		static constexpr long long max_uint =
		    std::numeric_limits<unsigned>::max();
		return std::max(0u, static_cast<unsigned>(std::min(max_uint, val)));
	}

	struct gnu_toolkit : toolkit {
		gnu_toolkit(std::filesystem::path const& compiler,
		            std::string&& ver,
		            std::string&& triplet)
		    : ver_{std::move(ver)}
		    , triplet_{std::move(triplet)}
		    , compiler_{compiler} {}
		std::string_view label() const noexcept override { return "GNU"sv; }
		std::string_view version() const noexcept override { return ver_; }

		void hello(config const&) const override {
			fmt::print("  [gcov] {}\n", get_u8path(gcov_));
		}

		void clean(config const& cfg) const override {
			remove_all(cfg.bin_dir / "**/*.gcda");
		}

		int observe(config const&,
		            std::string_view command,
		            args::arglist arguments) const {
			return platform::call(make_u8path(command), arguments);
		}

		int report(config const& cfg, coverage& cvg) override {
			using namespace std::filesystem;
			std::unordered_map<path, std::unordered_set<path>> paths;
			walk(cfg.bin_dir / "**/*.gcno",
			     [&](std::filesystem::path const& path) {
				     paths[path.parent_path()].insert(path.filename());
			     });

			closure state{.cfg = cfg, .cvg = cvg};

			for (auto const& [dirname, filenames] : paths) {
				state.push([&, this]() {
					std::vector<json::node> nodes;

					if (state.set_return_code(
					        load_directory(dirname, filenames, nodes)))
						return;

					for (auto& node : nodes) {
						state.push([this, &state, node = std::move(node)] {
							auto const ret =
							    analyze_node(state.cfg, state.cvg, node);
							state.task_finished();
							if (!ret) state.set_return_code(1);
						});
					}
					state.task_finished();
				});
			}

			return state.wait();
		}

		bool find_tools() {
			auto const hint = compiler_.parent_path();
			auto const ver = split(ver_, '.');
			return find_tool(gcov_, hint, fmt::format("{}gcov", triplet_), ver);
		}

	private:
		int load_directory(
		    std::filesystem::path const& dirname,
		    std::unordered_set<std::filesystem::path> const& filenames,
		    std::vector<json::node>& nodes);

		bool analyze_node(config const& cfg,
		                  coverage& cvg,
		                  json::node const& root);

		std::string ver_;
		std::string triplet_;
		std::filesystem::path compiler_;
		std::filesystem::path gcov_;
	};

	std::unique_ptr<toolkit> gnu(config const& cfg,
	                             std::span<std::string_view const> lines) {
		if (auto matched = GNU(lines[0]); matched) {
			auto triplet_matched = TRIPLET(lines[0]);
			auto triplet = triplet_matched
			                   ? triplet_matched.get<1>().to_string()
			                   : std::string{};
			auto tk = std::make_unique<gnu_toolkit>(
			    cfg.compiler, matched.get<1>().to_string(), std::move(triplet));
			if (!tk->find_tools()) tk.reset();
			return tk;
		}
		return {};
	}

	int gnu_toolkit::load_directory(
	    std::filesystem::path const& dirname,
	    std::unordered_set<std::filesystem::path> const& filenames,
	    std::vector<json::node>& nodes) {
		std::vector args{"--branch-probabilities"s, "--branch-counts"s,
		                 "--json-format"s,          "--stdout"s,
		                 "--object-directory"s,     get_u8path(dirname)};
		args.reserve(args.size() + filenames.size());
		for (auto const& gcno : filenames)
			args.push_back(get_u8path(dirname / gcno));

		auto proc = platform::run(get_u8path(gcov_), arguments{args});
		if (proc.return_code) {
			std::fwrite(proc.error.data(), 1, proc.error.size(), stderr);
			return proc.return_code;
		}

		auto view = json::string_view{
		    reinterpret_cast<char8_t const*>(proc.output.data()),
		    proc.output.size()};
		while (!view.empty()) {
			size_t read{};
			auto root =
			    json::read_json(view, {}, json::read_mode::serialized, &read);
			if (!read || std::holds_alternative<std::monostate>(root)) break;
			view = view.substr(read);

			nodes.push_back(std::move(root));
		}

		return 0;
	}

	bool gnu_toolkit::analyze_node(config const& cfg,
	                               coverage& cvg,
	                               json::node const& json_root) {
		auto root = cast<json::array>(json_root, u8"files");
		if (!root) return false;
		for (auto const& json_file : *root) {
			auto json_name = cast<json::string>(json_file, u8"file");
			auto json_lines = cast<json::array>(json_file, u8"lines");
			auto json_functions = cast<json::array>(json_file, u8"functions");

			if (!json_name || !json_lines) continue;
			auto filename = std::filesystem::path{*json_name};
			if (!filename.is_absolute()) filename = cfg.bin_dir / filename;

			auto sink = cvg.get_file_mt(get_u8path(filename));
			if (!sink) {
				filename = std::filesystem::weakly_canonical(filename);
				sink = cvg.get_file_mt(get_u8path(filename));
				if (!sink) {
					continue;
				}
			}

			for (auto const& json_line : *json_lines) {
				auto json_line_number =
				    cast<long long>(json_line, u8"line_number");
				auto json_count = cast<long long>(json_line, u8"count");

				if (!json_line_number || !json_count) continue;
				sink->on_visited(u(*json_line_number), u(*json_count));
			}

			if (json_functions) {
				for (auto const& json_function : *json_functions) {
					auto json_start_line =
					    cast<long long>(json_function, u8"start_line");
					auto json_end_line =
					    cast<long long>(json_function, u8"end_line");
					auto json_start_column =
					    cast<long long>(json_function, u8"start_column");
					auto json_end_column =
					    cast<long long>(json_function, u8"end_column");
					auto json_execution_count =
					    cast<long long>(json_function, u8"execution_count");
					auto json_fn_name =
					    cast<json::string>(json_function, u8"name");
					auto json_demangled_name =
					    cast<json::string>(json_function, u8"demangled_name");

					if (!json_start_line || !json_execution_count ||
					    !(json_fn_name && json_demangled_name))
						continue;

					text_pos start{
					    .line = u(*json_start_line),
					    .col = u(json_start_column ? *json_start_column : 0)};
					text_pos stop{
					    .line = u(json_end_line ? *json_end_line
					                            : *json_start_line),
					    .col = u(json_end_column ? *json_end_column : 0)};
					unsigned count = u(*json_execution_count);
					std::string name = from_u8s(
					    *(json_fn_name ? json_fn_name : json_demangled_name));
					std::string demangled;
					if (json_fn_name && json_demangled_name)
						demangled = from_u8s(*json_demangled_name);

					sink->on_function(start, stop, count, name, demangled);
				}
			}

			cvg.handover_mt(std::move(sink));
		}
		return true;
	}
}  // namespace cov::app::collect
