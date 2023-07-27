// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <cov/app/path.hh>
#include <cov/app/run.hh>
#include <cov/git2/config.hh>
#include <cov/io/file.hh>
#include <native/str.hh>
#include <string_view>
#include <task_watcher.hh>
#include <thread>
#include <thread_pool.hh>
#include <toolkit.hh>
#include <unordered_set>
#include <walk.hh>

using namespace std::literals;

namespace cov::app::collect {
	static unsigned u(long long val) {
		static constexpr long long max_uint =
		    std::numeric_limits<unsigned>::max();
		return std::max(0u, static_cast<unsigned>(std::min(max_uint, val)));
	}

	std::string dos_path(std::filesystem::path const& path) {
		auto copy = path;
		copy.make_preferred();
		return get_u8path(copy);
	}

	std::string dos_path(std::filesystem::path&& path) {
		path.make_preferred();
		return get_u8path(path);
	}

	struct msvc_toolkit : toolkit {
		msvc_toolkit(std::filesystem::path const& compiler)
		    : compiler_{compiler} {}

		bool load_config(config const& settings);
		bool get_version(config const& cfg);
		bool find_tools() {
			auto const hint = compiler_.parent_path();
			auto const ver = split(ver_, '.');
			if (!find_tool(occ_, LR"(C:/Program Files/OpenCppCoverage)"sv,
			               "OpenCppCoverage"sv, {}))
				return false;
			occ_.make_preferred();
			return true;
		}

		std::string_view label() const noexcept override { return "MSVC"sv; }
		std::string_view version() const noexcept override { return ver_; }

		void hello(config const&) const override {
			fmt::print("  [occ] {}\n", get_u8path(occ_));
			fmt::print("  [outfile] {}\n",
			           get_u8path("<bin>" / occ_dir_ / occ_output_));
		}

		void clean(config const&) const override {}

		int observe(config const& cfg,
		            std::string_view command,
		            ::args::arglist args) const override {
			if (occ_.empty()) {
				return platform::call(make_u8path(command), args);
			}

			/*
			OpenCppCoverage
			    -q
			    --working_dir <bin-dir>
			    --export_type <filter>:<bin-dir>\\<occ-dir>\\<output>.xml
			    --source <include>
			    --cover_children
			    --
			    ...*/
			std::vector occ_args{
			    "-q"s,
			    "--working_dir"s,
			    dos_path(cfg.src_dir),
			    "--export_type"s,
			    fmt::format("{}:{}", occ_filter_,
			                dos_path(cfg.bin_dir / occ_dir_ / occ_output_)),
			};
			// +2 for "--cover_children --"; +1 for command
			occ_args.reserve(occ_args.size() + 2 * cfg.include.size() + 2 + 1 +
			                 args.size());

			for (auto const& include : cfg.include) {
				std::error_code ec{};
				auto src = std::filesystem::absolute(cfg.src_dir / include, ec);
				if (ec) continue;
				occ_args.push_back("--source"s);
				occ_args.push_back(dos_path(std::move(src)));
			}
			occ_args.push_back("--cover_children"s);
			occ_args.push_back("--"s);

			std::filesystem::path cmd{};
			if (!find_tool(cmd, {}, command, {})) {
				cmd = make_u8path(command);
			}
			cmd.make_preferred();
			occ_args.push_back(get_u8path(cmd));
			for (auto index = 0u; index < args.size(); ++index) {
				auto arg = args[index];
				occ_args.push_back({arg.data(), arg.size()});
			}

			return platform::call(occ_, arguments{occ_args});
		}

		int report(config const&, coverage&) override { return 0; }

	private:
		std::string ver_;
		std::filesystem::path compiler_;
		std::filesystem::path occ_;
		std::filesystem::path occ_dir_{"OpenCppCoverage"sv};
		std::string occ_filter_{"cobertura"sv};
		std::filesystem::path occ_output_{"cobertura.xml"sv};
	};

	std::unique_ptr<toolkit> msvc(config const& cfg) {
		auto tk = std::make_unique<msvc_toolkit>(cfg.compiler);
		if (!tk->load_config(cfg) || !tk->get_version(cfg) || !tk->find_tools())
			tk.reset();
		return tk;
	}

	bool msvc_toolkit::load_config(config const& settings) {
		auto cfg = git::config::create();
		cfg.add_file_ondisk(settings.cfg_dir / ".covcollect"sv);
		cfg.foreach_entry([this](git_config_entry const* entry) -> int {
			auto name = std::string_view{entry->name};
			static constexpr auto prefix = "msvc."sv;
#ifdef _WIN32
			auto const ext = ".exe"s;
#endif

			if (!name.starts_with(prefix)) return 0;
			auto key = name.substr(prefix.length());

			if (key == "output-dir"sv) {
				occ_dir_ = make_u8path(entry->value);
			} else if (key == "filter"sv) {
				occ_filter_ = entry->value;
			} else if (key == "output"sv) {
				occ_output_ = entry->value;
			}
			return 0;
		});

		if (occ_dir_.empty()) occ_dir_ = "OpenCppCoverage"sv;
		if (occ_filter_.empty()) occ_filter_ = "cobertura"sv;
		if (occ_output_.empty()) occ_output_ = "cobertura.xml"sv;

		return true;
	}

	bool msvc_toolkit::get_version(config const& cfg) {
		static constexpr auto MSC_VER = "_MSC_VER\n_MSC_FULL_VER"sv;
		std::error_code ec{};
		auto const msc_ver = cfg.bin_dir / occ_dir_ / "msc_ver.c";
		std::filesystem::create_directories(msc_ver.parent_path(), ec);
		if (ec) return false;
		{
			auto file = io::fopen(msc_ver, "w");
			if (!file) return false;
			if (file.store(MSC_VER.data(), MSC_VER.size()) != MSC_VER.size())
				return false;
		}
		std::vector cl_args = {"/nologo"s, "/EP"s,
		                       get_u8path(msc_ver.filename())};
		auto proc = platform::run(cfg.compiler, arguments{cl_args},
		                          msc_ver.parent_path(), {});
		if (proc.return_code) {
			std::fwrite(proc.error.data(), 1, proc.error.size(), stderr);
			return false;
		}

		auto const ver_text =
		    trim({reinterpret_cast<char const*>(proc.output.data()),
		          proc.output.size()});
		auto const ver_lines = split(ver_text, '\n');
		auto const first_line = trim(ver_lines.front());

		auto major = first_line.substr(0, 2);
		auto minor = first_line.size() > 2 ? first_line.substr(2) : "00"sv;

		if (ver_lines.size() > 1) {
			auto const second_line = trim(ver_lines[1]);
			if (second_line.size() > first_line.size()) {
				auto const patch = second_line.substr(first_line.size());

				ver_ = fmt::format("{}.{}.{}", major, minor, patch);
				return true;
			}
		}
		ver_ = fmt::format("{}.{}", major, minor);
		return true;
	}
}  // namespace cov::app::collect
