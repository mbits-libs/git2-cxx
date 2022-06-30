// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/discover.hh>
#include <fstream>
#include <git2/repository.hh>
#include "path-utils.hh"

#ifdef WIN32
#include <Windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace cov {
	namespace {
		std::optional<path> read_covlink(path const& basename) {
			std::error_code ec{};
			if (!is_regular_file(basename, ec) || ec) return std::nullopt;
			std::ifstream in{basename};
			std::string line{};
			if (!std::getline(in, line)) return std::nullopt;
			std::string_view view{line};
			if (view.length() < names::covlink_prefix.length() ||
			    view.substr(0, names::covlink_prefix.length()) !=
			        names::covlink_prefix)
				return std::nullopt;
			view = view.substr(names::covlink_prefix.length());
			auto const isspace = [](char c) {
				return std::isspace(static_cast<unsigned char>(c));
			};
			while (!view.empty() && isspace(view.front()))
				view = view.substr(1);
			while (!view.empty() && isspace(view.back()))
				view = view.substr(0, view.length() - 1);
			return weakly_canonical(basename.parent_path() / make_path(view));
		}

		auto device_id(path const& name) {
#ifdef WIN32
#define STAT _wstat
			using stat_type = struct _stat;
#else
#define STAT stat
			using stat_type = struct stat;
#endif

			stat_type st{};
			using result_type = std::optional<decltype(st.st_dev)>;
			if (STAT(name.c_str(), &st)) return result_type{};
			return result_type{st.st_dev};
		}
	}  // namespace

#define TRY(DIRNAME)                                                  \
	do {                                                              \
		auto local = (DIRNAME);                                       \
		if (is_valid_path(local)) return weakly_canonical(local);     \
		auto covlink = read_covlink(local);                           \
		if (covlink) {                                                \
			local = std::move(*covlink);                              \
			if (is_valid_path(local)) return weakly_canonical(local); \
		}                                                             \
	} while (0)

	std::filesystem::path discover_repository(
	    std::filesystem::path const& current_dir,
	    discover across_fs) {
		if (is_valid_path(current_dir)) return weakly_canonical(current_dir);

		TRY(current_dir / names::covdata_dir);
		if (auto const git_dir = git::repository::discover(
		        current_dir, across_fs == discover::across_fs
		                         ? git::Discover::AcrossFs
		                         : git::Discover::WithinFs);
		    !git_dir.empty()) {
			TRY(git_dir / names::covdata_dir);
		}

		auto dirname = absolute(current_dir);
		decltype(device_id(dirname)) device = std::nullopt;

		while (true) {
			{
				auto parent = dirname.parent_path();
				if (parent == dirname) break;
				dirname = std::move(parent);
				if (across_fs == discover::within_fs) {
					auto current_device = device_id(dirname);
					if (current_device) {
						if (!device) {
							device = current_device;
						} else if (*device != *current_device) {
							return {};
						}
					}
				}
			}
			if (is_valid_path(dirname)) return weakly_canonical(dirname);
			TRY(dirname / names::covdata_dir);
		}

		return {};
	}

#undef TRY
}  // namespace cov
