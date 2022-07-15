// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <sys/wait.h>
#include <unistd.h>
#include <args/parser.hpp>
#include <cstdlib>
#include <filesystem>

#ifdef RUNNING_GCOV
extern "C" {
#include <gcov.h>
}
#endif

namespace cov::app::platform {
	namespace {
		using namespace std::literals;
		[[noreturn]] void spawn(std::filesystem::path const& tooldir,
		                        std::string_view tool,
		                        args::arglist args) {
			auto const fd_max = static_cast<int>(sysconf(_SC_OPEN_MAX));
			for (int fd = 3; fd < fd_max; fd++)
				close(fd);

			setpgid(0, 0);

			auto const program = "cov-" + std::string(tool.data(), tool.size());
			auto const program_path = (tooldir / program);

			std::vector<char*> argv;
			argv.reserve(2 + args.size());  // arg0 and NULL
			argv.push_back(const_cast<char*>(program.c_str()));
			for (unsigned i = 0; i < args.size(); ++i)
				argv.push_back(const_cast<char*>(args[i].data()));
			argv.push_back(nullptr);

#ifdef RUNNING_GCOV
			// since we plan to go away...
			__gcov_dump();
#endif
			// GCOV_EXCL_START[POSIX]
			execv(program_path.c_str(), argv.data());
			// pass error result from execv:
			_exit(-errno);
		}  // GCOV_EXCL_STOP
	}      // namespace

	std::filesystem::path exec_path() {
		std::error_code ec;
		static constexpr std::string_view self_links[] = {
		    "/proc/self/exe"sv,
		    "/proc/curproc/file"sv,
		    "/proc/curproc/exe"sv,
		    "/proc/self/path/a.out"sv,
		};
		for (auto path : self_links) {
			auto link = std::filesystem::read_symlink(path, ec);
			if (!ec) return link;
		}
		[[unlikely]];  // GCOV_EXCL_LINE[POSIX]
		return {};     // GCOV_EXCL_LINE[POSIX]
	}

	static pid_t pid = -1;

	// GCOV_EXCL_START[POSIX]
	void forward_signal(int signo) { kill(pid, signo); }
	// GCOV_EXCL_STOP

	int run_tool(std::filesystem::path const& tooldir,
	             std::string_view tool,
	             args::arglist args) {
		pid = fork();
		if (pid < 0) return -errno;
		if (!pid) spawn(tooldir, tool, args);

		signal(SIGINT, forward_signal);
		signal(SIGTERM, forward_signal);
#if defined(SIGQUIT)
		signal(SIGQUIT, forward_signal);
#endif  // defined(SIGQUIT)

		int status;
		waitpid(pid, &status, 0);
		if (WIFEXITED(status)) return static_cast<char>(WEXITSTATUS(status));
		// GCOV_EXCL_START[POSIX]
		[[unlikely]];
		if (WIFSIGNALED(status)) return WIFSIGNALED(status);
		return 128;
		// GCOV_EXCL_STOP
	}
}  // namespace cov::app::platform
