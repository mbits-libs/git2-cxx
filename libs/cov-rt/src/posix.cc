// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <args/parser.hpp>
#include <cov/app/dirs.hh>
#include <cov/app/path.hh>
#include <cov/app/tools.hh>
#include <cstdlib>
#include <filesystem>
#include <thread>
#include "path_env.hh"

#ifdef RUNNING_GCOV
extern "C" {
#include <gcov.h>
}
#endif

#ifdef RUNNING_LLVM_COV
extern "C" int __llvm_profile_write_file(void);
#endif

namespace cov::app::platform {
	namespace {
		std::string env(char const* name) {
			auto value = getenv(name);
			return value ? value : std::string{};
		}

		bool executable(std::filesystem::path const& program) {
			return !std::filesystem::is_directory(program) &&
			       !access(program.c_str(), X_OK);
		}

		std::filesystem::path where(std::filesystem::path const& bin,
		                            char const* environment_variable,
		                            std::string const& program) {
			auto path_str = env(environment_variable);
			auto dirs = split(bin.native(), path_str);

			for (auto const& dir : dirs) {
				auto path = std::filesystem::path{dir} / program;
				if (executable(path)) {
					return path;
				}
			}

			return {};
		}

		[[noreturn]] void spawn(std::filesystem::path const& program_path,
		                        args::arglist args) {
			auto const fd_max = static_cast<int>(sysconf(_SC_OPEN_MAX));
			for (int fd = 3; fd < fd_max; fd++)
				close(fd);

			setpgid(0, 0);

			std::vector<char*> argv;
			argv.reserve(2 + args.size());  // arg0 and NULL
			argv.push_back(const_cast<char*>(program_path.filename().c_str()));
			for (unsigned i = 0; i < args.size(); ++i)
				argv.push_back(const_cast<char*>(args[i].data()));
			argv.push_back(nullptr);

			// since we plan to go away...
#ifdef RUNNING_GCOV
			__gcov_dump();
#endif
#ifdef RUNNING_LLVM_COV
			__llvm_profile_write_file();
#endif
			// GCOV_EXCL_START[POSIX]
			execv(program_path.c_str(), argv.data());
			// pass error result from execv:
			if (errno == ENOEXEC) errno = EACCES;
			_exit(-errno);
		}  // GCOV_EXCL_STOP

		static pid_t pid = -1;

		// GCOV_EXCL_START[POSIX]
		void forward_signal(int signo) { kill(pid, signo); }
		// GCOV_EXCL_STOP

		struct sigpipe_handler {
			sigset_t sig_block, sig_restore;
			sigpipe_handler() {
				sigemptyset(&sig_block);
				sigaddset(&sig_block, SIGPIPE);

				pthread_sigmask(SIG_BLOCK, &sig_block, &sig_restore);
			}
			// GCOV_EXCL_START[POSIX]
			bool wait_if_needed() {
				if (errno != EPIPE) {
					return false;
				}
				struct timespec ts;
				ts.tv_sec = 0;
				ts.tv_nsec = 0;

				int sig;
				while ((sig = sigtimedwait(&sig_block, 0, &ts)) == -1) {
					if (errno != EINTR) {
						break;
					}
				}
				return true;
			}
			// GCOV_EXCL_STOP
			~sigpipe_handler() {
				pthread_sigmask(SIG_SETMASK, &sig_restore, NULL);
			}
		};

		struct pipe_type {
			int read{-1};
			int write{-1};

			~pipe_type() {
				close_side(read);
				close_side(write);
			}

			void close_read() { close_side(read); }
			void close_write() { close_side(write); }

			void dup_read(int stdio) {
				close_write();
				if (read == -1) return;
				::dup2(read, stdio);
			}

			void dup_write(int stdio) {
				close_read();
				if (write == -1) return;
				::dup2(write, stdio);
			}

			bool open() {
				int fd[2];
				if (::pipe(fd) == -1) return false;
				read = fd[0];
				write = fd[1];
				return true;
			}

			static constexpr size_t BUFSIZE = 16384u;

			std::thread async_write(std::vector<std::byte> const& src) {
				return std::thread(
				    // GCOV_EXCL_START[POSIX]
				    [](pipe_type* self, std::vector<std::byte> const& bytes) {
					    // GCOV_EXCL_STOP
					    sigpipe_handler block_sigpipe{};

					    auto ptr = bytes.data();
					    auto size = bytes.size();

					    auto const fd = self->write;
					    while (size) {
						    auto chunk = size;
						    if (chunk > BUFSIZE) chunk = BUFSIZE;
						    auto actual = ::write(fd, ptr, chunk);
						    // GCOV_EXCL_START[POSIX]
						    if (actual < 0) {
							    block_sigpipe.wait_if_needed();
							    break;
						    }
						    // GCOV_EXCL_STOP
						    ptr += actual;
						    size -= static_cast<size_t>(actual);
					    }

					    self->close_write();
				    },
				    this, std::ref(src));
			}

			std::thread async_read(std::vector<std::byte>& dst) {
				return std::thread(
				    [](int fd, std::vector<std::byte>& bytes) {
					    std::byte buffer[BUFSIZE];

					    while (true) {
						    auto const actual =
						        ::read(fd, buffer, std::size(buffer));
						    if (actual <= 0) break;
						    bytes.insert(bytes.end(), buffer, buffer + actual);
					    }
				    },
				    read, std::ref(dst));
			}

		private:
			void close_side(int& side) {
				if (side >= 0) ::close(side);
				side = -1;
			}
		};

		struct pipes_type {
			pipe_type input{};
			pipe_type output{};
			pipe_type error{};

			bool open(bool with_input, bool with_output) {
				if (with_input) {
					if (!input.open()) return false;
				}
				if (with_output) {
					if (!output.open() || !error.open()) return false;
				}
				return true;
			}

			void io(std::vector<std::byte> const* input_data,
			        captured_output* output_data) {
				input.close_read();
				output.close_write();
				error.close_write();

				std::vector<std::thread> threads{};
				threads.reserve((input_data ? 1u : 0u) +
				                (output_data ? 2u : 0u));
				if (input_data) {
					threads.push_back(input.async_write(*input_data));
				}
				if (output_data) {
					threads.push_back(output.async_read(output_data->output));
					threads.push_back(error.async_read(output_data->error));
				}

				for (auto& thread : threads) {
					thread.join();
				}
			}

			void dup() {
				input.dup_read(0);
				output.dup_write(1);
				error.dup_write(2);
			}
		};

		captured_output execute(std::filesystem::path const& bin,
		                        char const* environment_variable,
		                        std::string const& program,
		                        args::arglist args,
		                        std::vector<std::byte> const* input,
		                        std::filesystem::path const* cwd,
		                        bool capture) {
			captured_output result{};

			auto const executable = where(bin, environment_variable, program);
			if (executable.empty()) {
				result.return_code = -ENOENT;
				return result;
			}

			setenv("COV_EXE_PATH",
			       get_u8path(app::platform::exec_path()).c_str(), 1);

			pipes_type pipes{};
			if (!pipes.open(input != nullptr, capture)) {
				// GCOV_EXCL_START[POSIX]
				[[unlikely]];
				result.return_code = 128;
				return result;
			}  // GCOV_EXCL_STOP

			pid = fork();
			if (pid < 0) {
				// GCOV_EXCL_START[POSIX]
				[[unlikely]];
				result.return_code = -errno;
				return result;
			}  // GCOV_EXCL_STOP
			if (!pid) {
				if (cwd) {
					std::error_code ignore{};
					std::filesystem::current_path(*cwd, ignore);
				}
				pipes.dup();
				spawn(executable, args);
			}

			signal(SIGINT, forward_signal);
			signal(SIGTERM, forward_signal);
#if defined(SIGQUIT)
			signal(SIGQUIT, forward_signal);
#endif  // defined(SIGQUIT)

			pipes.io(input, capture ? &result : nullptr);

			int status;
			waitpid(pid, &status, 0);
			if (WIFEXITED(status)) {
				result.return_code = static_cast<char>(WEXITSTATUS(status));
				return result;
			}
			// GCOV_EXCL_START[POSIX]
			[[unlikely]];
			result.return_code =
			    (WIFSIGNALED(status)) ? WIFSIGNALED(status) : 128;
			return result;
			// GCOV_EXCL_STOP
		}
	}  // namespace

	int run_tool(std::filesystem::path const& tooldir,
	             std::string_view tool,
	             args::arglist args) {
		return execute(tooldir, "COV_PATH",
		               "cov-" + std::string(tool.data(), tool.size()), args,
		               nullptr, nullptr, false)
		    .return_code;
	}

	captured_output run_filter(std::filesystem::path const& filter_dir,
	                           std::filesystem::path const& cwd,
	                           std::string_view filter,
	                           args::arglist args,
	                           std::vector<std::byte> const& input) {
		return execute(filter_dir, "COV_FILTER_PATH",
		               std::string(filter.data(), filter.size()), args, &input,
		               &cwd, true);
	}
}  // namespace cov::app::platform
