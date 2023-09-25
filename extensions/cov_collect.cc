// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <git2/errors.h>
#include <args/parser.hpp>
#include <concepts>
#include <cov/app/path.hh>
#include <cov/app/run.hh>
#include <cov/git2/global.hh>
#include <cov/git2/oid.hh>
#include <cov/git2/repository.hh>
#include <cov/hash/sha1.hh>
#include <cov/io/file.hh>
#include <cov/version.hh>
#include <json/json.hpp>
#include <map>
#include <memory>
#include <native/str.hh>
#include <report.hh>
#include <string>
#include <toolkit.hh>

using namespace std::literals;
using namespace cov::app;
using namespace cov;

git::repository open_here() {
	std::error_code ec{};
	auto const common = git::repository::discover("."sv, ec);
	if (ec) return {};
	auto repo = git::repository::open(common, ec);
	if (ec) return {};
	return repo;
}

bool repo_head(git::repository const& repo,
               json::string& branch,
               git::oid& ref) {
	std::error_code ec{};
	auto head = repo.head(ec);
	if (ec == git::errc::unbornbranch || ec == git::errc::notfound) {
		fmt::print(stderr, "[git] called on unborn branch\n");
		return false;
	}

	auto branch_name = head.shorthand();
	if (branch_name) branch = to_u8(branch_name);

	auto resolved = head.resolve(ec);
	if (!ec) {
		auto oid = git_reference_target(resolved.raw());
		if (oid) {
			ref.assign(*oid);
			return true;
		}
	}
	// GCOV_EXCL_START
	fmt::print(stderr, "[git] cannot resolve HEAD to a commit\n");
	return false;
	// GCOV_EXCL_STOP
}

enum class command { clean, observe, collect };

template <command Cmd>
using cmd_val = std::integral_constant<command, Cmd>;

struct params {
	std::optional<std::string> config{std::nullopt};
	command cmd{command::collect};
	args::args_view observed;
};

params parse_arguments(args::args_view const& arguments) {
	params result{};

	args::null_translator tr{};
	args::parser p{{}, arguments, &tr};
	p.arg(result.config, "c", "config").opt();
	p.set<cmd_val<command::clean>>(result.cmd, "clean").opt();
	p.set<cmd_val<command::observe>>(result.cmd, "observe").opt();

	auto rest = p.parse(args::parser::allow_subcommands);
	if (!rest.empty() && result.cmd != command::observe) {
		p.error(tr(args::lng::unrecognized, rest[0], {}));
	}

	if (!rest.empty()) {
		result.observed.progname = rest[0];
		result.observed.args = rest.shift();
	} else if (result.cmd == command::observe) {
		p.error("argument --observe: missing tool name and arguments");
	}

	return result;
}

std::filesystem::path workdir(git::repository const& repo) {
	auto const dir_view = repo.workdir();
	if (!dir_view) return std::filesystem::current_path();
	return make_u8path(*dir_view);
}

void find_config(git::repository const& repo,
                 std::optional<std::string>& config) {
	auto root = workdir(repo);

	for (auto const& dirname : {"build/debug"sv, "build"sv, "debug"sv, ""sv}) {
		auto path = root / dirname / ".covcollect"sv;
		if (std::filesystem::is_regular_file(path)) {
			config = get_u8path(path);
			return;
		}
	}
}

template <std::invocable<> Callback>
int measure(std::string_view label, Callback&& cb) {
	using namespace std::chrono;

	fmt::print(stderr, "[{}] start...\n", label);

	auto const then = steady_clock::now();
	auto const ret = cb();
	auto const now = steady_clock::now();
	auto const ms = duration_cast<milliseconds>(now - then);
	auto const cent_s = (ms.count() + 5) / 10;
	fmt::print(stderr, "[{}] {}.{:02} s\n", label, cent_s / 100, cent_s % 100);

	return ret;
}

int tool(args::args_view const& arguments) {
	git::init memory{};

	auto params = parse_arguments(arguments);

	auto repo = open_here();
	if (!repo) return 1;
	json::string branch{};
	git::oid ref{};
	if (!repo_head(repo, branch, ref)) return 1;

	if (!params.config) {
		find_config(repo, params.config);
		if (!params.config) return 1;
	}

	collect::config cfg{};
	cfg.load(*params.config);

	auto const toolkit = collect::recognize_toolkit(cfg);
	if (!toolkit) return 1;

	if (params.cmd == command::clean) {
		toolkit->clean(cfg);
		return 0;
	}

	if (params.cmd == command::observe) {
		return toolkit->observe(cfg, params.observed.progname,
		                        params.observed.args);
	}

	fmt::print(stderr, "[toolkit] {} {} (from {})\n", toolkit->label(),
	           toolkit->version(), get_u8path(*params.config));
	toolkit->hello(cfg);
	collect::report cvg{cfg};

	if (toolkit->needs_preprocess()) {
		auto const ret =
		    measure("preprocess"sv, [&] { return toolkit->preprocess(cfg); });
		if (ret) return ret;
	}

	auto ret = measure("report"sv, [&] { return toolkit->report(cfg, cvg); });
	if (ret) return ret;

	cvg.post_process();
	auto const summary = cvg.summary();

	fmt::print(stderr, "[files] {}\n", summary.lines_total);
	fmt::print(stderr, "[lines] {} / {}\n", summary.lines.visited,
	           summary.lines.relevant);
	fmt::print(stderr, "[functions] {} / {}\n", summary.functions.visited,
	           summary.functions.relevant);

	{
		struct output : json::output {
			output(std::filesystem::path path) : file{io::fopen(path, "w")} {}
			void write(json::string_view view) override {
				file.store(view.data(), view.size());
			}
			void write(byte_type ch) override { file.store(&ch, 1); }
			io::file file;
		};

		auto outname = cfg.bin_dir / cfg.output;

		{
			std::error_code ignore;
			std::filesystem::create_directories(outname.parent_path(), ignore);
		}

		output results{outname};
		if (!results.file) {
			fmt::print(stderr, "[i/o] cannot open {}\n", get_u8path(outname));
			return 1;
		}
		auto node = cvg.get_json(branch, ref);
		json::write_json(results, node);
		fmt::print(stderr, "[i/o] {}\n", get_u8path(outname));
	}
	return 0;
}
