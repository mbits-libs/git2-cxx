// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <stdlib.h>
#include <cov/app/path.hh>
#include <cov/app/run.hh>
#include <cov/git2/config.hh>
#include <cov/io/file.hh>
#include <ctre.hpp>
#include <json/json.hpp>
#include <native/str.hh>
#include <set>
#include <string_view>
#include <task_watcher.hh>
#include <toolkit.hh>
#include <walk.hh>

#include <iostream>

using namespace std::literals;

// Ubuntu clang version <version> (<meta>)
// clang version <version> -- on windows
static constexpr auto CLANG =
    ctre::match<R"(^(?:\w+ )?(?:clang|LLVM) \w+ ([0-9.]+).*$)">;

#ifdef _WIN32
int setenv(const char* name, const char* value, int) {
	return _putenv_s(name, value);
}
#endif

namespace cov::app::collect {
	struct execution {
		unsigned line;
		std::optional<unsigned> count;
	};

	static unsigned u(long long val) {
		static constexpr long long max_uint =
		    std::numeric_limits<unsigned>::max();
		return std::max(0u, static_cast<unsigned>(std::min(max_uint, val)));
	}

	struct region_ref {
		text_pos start;
		text_pos end;

		static std::optional<region_ref> encompassing_function(
		    json::array const& regions) {
			std::optional<region_ref> result{std::nullopt};

			for (auto const& json_region : regions) {
				auto region = cast<json::array>(json_region);
				if (!region || region->size() < 8) return std::nullopt;
				auto start_line = cast<long long>((*region)[0]);
				auto start_col = cast<long long>((*region)[1]);
				auto end_line = cast<long long>((*region)[2]);
				auto end_col = cast<long long>((*region)[3]);
				auto file_id = cast<long long>((*region)[5]);
				auto type = cast<long long>((*region)[7]);

				if (!start_line || !start_col || !end_line || !end_col ||
				    !type || !file_id)
					return std::nullopt;
				if (*type || *file_id) continue;

				text_pos start{.line = u(*start_line), .col = u(*start_col)};
				text_pos end{.line = u(*end_line), .col = u(*end_col)};

				if (!result) {
					result = region_ref{.start = start, .end = end};
					continue;
				}

				if (result->start > start) result->start = start;
				if (result->end < end) result->end = end;
			}

			return result;
		}
	};

	struct cvg_segment {
		text_pos pos;
		unsigned count;
		bool has_count;
		bool is_entry;
		bool is_gap;

		bool is_start_of_region() const {
			return !is_gap && has_count && is_entry;
		}

		static std::optional<cvg_segment> from(json::array const& arr) {
			// [24, 37, 21, true, true, false]
			if (arr.size() < 6) return std::nullopt;
			auto line = cast<long long>(arr[0]);
			auto column = cast<long long>(arr[1]);
			auto count = cast<long long>(arr[2]);
			auto has_count = cast<bool>(arr[3]);
			auto is_entry = cast<bool>(arr[4]);
			auto is_gap = cast<bool>(arr[5]);

			if (!line || !column || !count || !has_count || !is_entry ||
			    !is_gap)
				return std::nullopt;

			return cvg_segment{.pos = {.line = u(*line), .col = u(*column)},
			                   .count = u(*count),
			                   .has_count = *has_count,
			                   .is_entry = *is_entry,
			                   .is_gap = *is_gap};
		}

		static execution make_stats(
		    std::span<cvg_segment const*> const& line_segments,
		    cvg_segment const* wrapped_segment,
		    unsigned line) {
			execution result{line, std::nullopt};

			unsigned min_region_count = 0;
			for (auto const& segment : line_segments) {
				if (min_region_count > 1) break;
				if (segment->is_start_of_region()) ++min_region_count;
			}

			auto const start_of_skipped_region =
			    (!line_segments.empty() && line_segments[0]->has_count &&
			     line_segments[0]->is_entry);

			auto mapped = !start_of_skipped_region &&
			              ((wrapped_segment && wrapped_segment->has_count) or
			               (min_region_count > 0));
			if (!mapped) return result;

			if (wrapped_segment) result.count = wrapped_segment->count;

			for (auto const& segment : line_segments) {
				if (segment->is_start_of_region())
					result.count = std::max(*result.count, segment->count);
			}

			return result;
		}

		static void run_coverage(std::span<cvg_segment const> const& coverage,
		                         coverage_file& sink) {
			cvg_segment const* wrapped = nullptr;
			auto line = coverage.empty() ? 0u : coverage[0].pos.line;

			std::vector<cvg_segment const*> segments{};
			segments.reserve(coverage.size());

			auto const end_index = coverage.size();
			decltype(coverage.size()) index = 0;

			while (index < end_index) {
				if (!segments.empty()) wrapped = segments.back();
				segments.clear();

				while (index < end_index && coverage[index].pos.line == line) {
					segments.push_back(&coverage[index]);
					++index;
				}

				auto const mark = make_stats(segments, wrapped, line);
				++line;

				if (mark.count) sink.on_visited(mark.line, *mark.count);
			}
		}
	};

	struct llvm_toolkit : toolkit {
		llvm_toolkit(std::filesystem::path const& compiler, std::string&& ver)
		    : ver_{std::move(ver)}, compiler_{compiler} {}

		bool load_config(config const& settings);
		bool find_tools() {
			auto const hint = compiler_.parent_path();
			auto const ver = split(ver_, '.');
			return find_tool(merger_, hint, "llvm-profdata"sv, ver) &&
			       find_tool(exporter_, hint, "llvm-cov"sv, ver);
		}

		std::string_view label() const noexcept override { return "LLVM"sv; }
		std::string_view version() const noexcept override { return ver_; }

		void hello(config const&) const override {
			fmt::print("  [merge] {}\n", get_u8path(merger_));
			fmt::print("  [cov]   {}\n", get_u8path(exporter_));
			if (!raw_ext_.empty()) fmt::print("  [raw]   {}\n", raw_ext_);
			if (!prof_data_dir_.empty())
				fmt::print("  [data]  {}\n", get_u8path(prof_data_dir_));
			if (!exec_.empty()) {
				fmt::print("  [exe]\n");
				for (auto const& exe : exec_) {
					fmt::print("   - {}\n", get_u8path(exe));
				}
			}
		}

		auto merged() const { return prof_data_dir_ / "merged.db"sv; }

		void clean(config const& cfg) const override {
			std::error_code ec{};
			remove_all(cfg.bin_dir / ("**/*" + raw_ext_));
			std::filesystem::remove_all(cfg.bin_dir / prof_data_dir_, ec);
			if (ec) return;
			std::filesystem::create_directories(cfg.bin_dir / prof_data_dir_,
			                                    ec);
		}

		int observe(config const& cfg,
		            std::string_view command,
		            args::arglist arguments) const {
			auto const file_mask = get_u8path(cfg.bin_dir / prof_data_dir_ /
			                                  "raw"sv / ("%p" + raw_ext_));
			setenv("LLVM_PROFILE_FILE", file_mask.c_str(), 1);
			return platform::call(make_u8path(command), arguments);
		}

		bool needs_preprocess() const override { return true; }
		int preprocess(config const& cfg) override;
		int report(config const& cfg, coverage& cvg) override {
			closure state{.cfg = cfg, .cvg = cvg, .pool{}};

			for (auto const& exe : exec_) {
				state.push([&, exe] {
					state.set_return_code(
					    report_exe(state.cfg, state.cvg, exe));
					state.task_finished();
				});
			}

			return state.wait();
		}

		int report_exe(config const& cfg,
		               coverage& cvg,
		               std::filesystem::path const& exe);

	private:
		std::string ver_;
		std::filesystem::path compiler_;
		std::filesystem::path merger_;
		std::filesystem::path exporter_;
		std::vector<std::filesystem::path> exec_;
		std::filesystem::path prof_data_dir_;
		std::string raw_ext_;
	};

	std::unique_ptr<toolkit> llvm(config const& cfg,
	                              std::span<std::string_view const> lines) {
		if (auto matched = CLANG(lines[0]); matched) {
			auto tk = std::make_unique<llvm_toolkit>(
			    cfg.compiler, matched.get<1>().to_string());
			if (!tk->load_config(cfg) || !tk->find_tools()) tk.reset();
			return tk;
		}
		return {};
	}

	bool llvm_toolkit::load_config(config const& settings) {
		std::set<std::filesystem::path> patterns;
		std::set<std::filesystem::path> execs;

		auto cfg = git::config::create();
		cfg.add_file_ondisk(settings.cfg_dir / ".covcollect"sv);
		cfg.foreach_entry(
		    [this, &patterns](git_config_entry const* entry) -> int {
			    auto name = std::string_view{entry->name};
			    static constexpr auto prefix = "clang."sv;
#ifdef _WIN32
			    auto const ext = ".exe"s;
#endif

			    if (!name.starts_with(prefix)) return 0;
			    auto key = name.substr(prefix.length());

			    if (key == "profdata"sv) {
				    prof_data_dir_ = make_u8path(entry->value);
			    } else if (key == "exec"sv) {
#ifdef _WIN32
				    patterns.insert(make_u8path(entry->value + ext));
#else
				    patterns.insert(make_u8path(entry->value));
#endif
			    } else if (key == "raw"sv) {
				    raw_ext_ = entry->value;
			    }
			    return 0;
		    });

		prof_data_dir_ = std::filesystem::weakly_canonical(
		    prof_data_dir_.empty() ? settings.bin_dir / "llvm"sv
		                           : settings.bin_dir / prof_data_dir_);
		for (auto& pattern : patterns) {
			auto const path =
			    std::filesystem::weakly_canonical(settings.bin_dir / pattern);
			walk(path, [&execs](auto const& found) {
				auto f = io::fopen(found, "rb");
				if (f) {
					char hash_bang[2];
					if (f.load(hash_bang, 2) == 2) {
						if (hash_bang[0] == '#' && hash_bang[1] == '!') {
							// it's a script!
							return;
						}
					}
				}
				execs.insert(found);
			});
		}

		exec_.clear();
		exec_.insert(exec_.begin(), execs.begin(), execs.end());

		return true;
	}

	int llvm_toolkit::preprocess(config const& cfg) {
		std::error_code ec{};
		using namespace std::filesystem;

		auto const db = merged();
		create_directories(prof_data_dir_, ec);
		if (ec) return 1;
		remove(db, ec);
		if (ec) {
			std::cerr << "error: " << ec.message() << '\n';
			return 1;
		}

		std::vector<std::string> stg{"merge"s, "-sparse"s};

		walk(cfg.bin_dir / ("**/*" + raw_ext_),
		     [&stg](auto const& path) { stg.push_back(get_u8path(path)); });

		stg.reserve(stg.size() + 2);
		stg.push_back("-o"s);
		stg.push_back(get_u8path(db));

		return platform::call(merger_, arguments{stg});
	}

	int llvm_toolkit::report_exe(config const&,
	                             coverage& cvg,
	                             std::filesystem::path const& exe) {
		std::array args{"export"s,         "-format"s,
		                "text"s,           "-skip-expansions"s,
		                "-instr-profile"s, get_u8path(merged()),
		                get_u8path(exe)};
		auto proc = platform::run(get_u8path(exporter_), arguments{args});
		if (proc.return_code) {
			std::fwrite(proc.error.data(), 1, proc.error.size(), stderr);
			return proc.return_code;
		}

		auto root = json::read_json(
		    {reinterpret_cast<char8_t const*>(proc.output.data()),
		     proc.output.size()});

		auto root_map = cast<json::map>(root);
		if (!root_map) return 1;

		{
			auto root_version = cast<json::string>(root_map, u8"version");
			auto version = split(from_u8(*root_version), '.');
			if (version.empty() || version[0] != "2"sv) return 1;
		}

		if (auto root_data = cast<json::array>(root_map, u8"data"); root_data) {
			for (auto const& json_data : *root_data) {
				auto files = cast<json::array>(json_data, u8"files");
				auto functions = cast<json::array>(json_data, u8"functions");
				if (files) {
					for (auto const& json_file : *files) {
						auto filename =
						    cast<json::string>(json_file, u8"filename");
						auto segments =
						    cast<json::array>(json_file, u8"segments");
						auto branches =
						    cast<json::array>(json_file, u8"branches");

						if (!filename || (!segments && !branches)) continue;

						auto sink = cvg.get_file(from_u8(*filename));
						if (!sink) continue;

						if (segments) {
							std::vector<cvg_segment> data{};
							data.reserve(segments->size());
							for (auto const& json_segment : *segments) {
								auto segment = cast<json::array>(json_segment);
								if (!segment) return 1;
								auto cvg_seg = cvg_segment::from(*segment);
								if (!cvg_seg) return 1;
								data.push_back(*cvg_seg);
							}

							cvg_segment::run_coverage(data, *sink);
						}
					}
				}
				if (functions) {
					for (auto const& json_function : *functions) {
						auto filenames =
						    cast<json::array>(json_function, u8"filenames");
						auto regions =
						    cast<json::array>(json_function, u8"regions");
						auto name = cast<json::string>(json_function, u8"name");
						auto count = cast<long long>(json_function, u8"count");

						if (!filenames || filenames->empty() || !regions ||
						    regions->empty() || !name || !count)
							continue;

						auto region =
						    region_ref::encompassing_function(*regions);
						if (!region) continue;

						auto filename = cast<json::string>((*filenames)[0]);
						auto sink = cvg.get_file(from_u8(*filename));
						if (sink)
							sink->on_function(region->start, region->end,
							                  u(*count), get_u8path(*name), {});
					}
				}
			}
		}
		return 0;
	}

}  // namespace cov::app::collect
