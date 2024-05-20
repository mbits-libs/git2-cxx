// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <cov/module.hh>
#include <cov/projection.hh>

namespace cov::projection {
	namespace {
		using namespace std::literals;

		void update_prefixes(std::unordered_set<std::string>& dst,
		                     std::vector<std::string> const& prefixes) {
			for (auto const& prefix : prefixes) {
				if (!prefix.empty() && prefix.ends_with('/')) {
					dst.insert(prefix.substr(0, prefix.length() - 1));
				} else {
					dst.insert(prefix);
				}
			}
		}

		void clean_modules(
		    std::map<std::string, std::vector<file_stats const*>>& modules) {
			auto it = modules.begin();
			auto end = modules.end();
			while (it != end) {
				if (!it->second.empty()) {
					++it;
					continue;
				}

				// GCOV_EXCL_START
				it = modules.erase(it);
				end = modules.end();
				// GCOV_EXCL_STOP
			}
		}

		struct dir_entry;
		dir_entry make_entry(entry_type type, std::string_view display);
		dir_entry make_file(std::string_view display,
		                    file_stats const& file,
		                    repository const* repo);
		auto find_entry(std::vector<dir_entry>& children,
		                entry_type type,
		                std::string_view display) -> decltype(children.begin());

		struct dir_entry {
			entry result{};
			std::vector<dir_entry> children{};

			template <typename Callback>
			void add_level(entry_type type,
			               std::string_view display,
			               Callback&& recurse) {
				auto it = find_entry(children, type, display);

				if (it != children.end()) {
					recurse(*it);
					return;
				}
				children.push_back(make_entry(type, display));
				recurse(children.back());
			}

			void add_module(std::string_view module,
			                std::string_view sep,
			                std::vector<file_stats const*> const& files,
			                size_t file_prefix_len,
			                repository const* repo) {
				if (module.empty()) {
					for (auto file : files) {
						auto filename = std::string_view{file->filename};
						if (file_prefix_len < filename.size())
							filename = filename.substr(file_prefix_len);
						add_file(filename, *file, repo);
					}

					return;
				}

				auto const pos = module.find(sep);
				auto const display =
				    sep.empty() ? module : module.substr(0, pos);
				auto const rest = sep.empty() || pos == std::string_view::npos
				                      ? std::string_view{}
				                      : module.substr(pos + sep.size());

				add_level(entry_type::module, display,
				          [=, &files](dir_entry& child) {
					          child.add_module(rest, sep, files,
					                           file_prefix_len, repo);
				          });
			}

			void add_file(std::string_view filename,
			              file_stats const& file,
			              repository const* repo) {
				auto const pos = filename.find('/');
				if (pos == std::string_view::npos)
					return children.push_back(make_file(filename, file, repo));

				auto const display = filename.substr(0, pos);
				auto const rest = pos == std::string_view::npos
				                      ? std::string_view{}
				                      : filename.substr(pos + 1);

				add_level(entry_type::directory, display,
				          [=, &file](dir_entry& child) {
					          child.add_file(rest, file, repo);
				          });
			}

			void add_files(
			    std::map<std::string, std::unordered_set<std::string>> const&
			        modules,
			    std::unordered_set<std::string> const& filtered_out,
			    std::vector<file_stats const*>& files,
			    std::string_view mod_sep,
			    bool mod_filter_empty,
			    size_t file_prefix_len,
			    repository const* repo) {
				std::map<std::string, std::vector<file_stats const*>>
				    module_data{};

				for (auto& file : files) {
					size_t length{};
					std::string module_name{};
					bool is_filtered = false;
					bool found_something = false;

					for (auto const& [modname, dirs] : modules) {
						for (auto const& dir : dirs) {
							auto const prefix = prefixed{dir};
							if (prefix.prefixes(file->filename) &&
							    length < prefix.prefix.length()) {
								length = prefix.prefix.length();
								module_name = modname;
								found_something = true;
							}
						}
					}

					for (auto const& dir : filtered_out) {
						auto const prefix = prefixed{dir};

						if (prefix.prefixes(file->filename) &&
						    length < prefix.prefix.length()) {
							length = prefix.prefix.length();
							module_name.clear();
							is_filtered = true;
							found_something = true;
						}
					}

					if (found_something) {
						if (!is_filtered) {
							module_data[module_name].push_back(file);
						}
						file = nullptr;
					}
				}

				if (mod_filter_empty) {
					std::vector<file_stats const*> nul_module;
					for (auto& file : files) {
						if (!file) continue;
						nul_module.push_back(file);
					}

					if (!nul_module.empty())
						module_data[{}] = std::move(nul_module);
				}

				clean_modules(module_data);

				for (auto const& [modname, mod_files] : module_data)
					add_module(modname, mod_sep, mod_files, file_prefix_len,
					           repo);
			}

			void propagate(std::string_view sep) {
				if (result.type != entry_type::file) {
					result.stats.current = coverage_stats::init();
					result.stats.previous = coverage_stats::init();
				}
				for (auto& child : children) {
					child.propagate(sep);
					result.stats.extend(child.result.stats);
				}

				if (children.size() != 1 ||
				    result.type != children.front().result.type) {
					return;
				}

				if (result.type == entry_type::module) {
					if (!result.name.display.empty())
						result.name.display.append(sep);
				} else {
					result.name.display.push_back('/');
				}
				result.name.display.append(
				    children.front().result.name.display);
				swap_with_child();
			}

			void swap_with_child() {
				auto tmp = std::move(children);
				children = std::vector<dir_entry>{};
				children = std::move(tmp.front().children);
			}
		};

		label make_label(std::string_view display) {
			return {.display{display.data(), display.size()}};
		}

		dir_entry make_entry(entry_type type, std::string_view display) {
			return {.result{.type = type, .name = make_label(display)}};
		}

		io::v1::stats recalc_functions(cov::repository const& repo,
		                               git::oid_view id,
		                               io::v1::stats fallback) {
			std::error_code ec{};
			auto funcs = repo.lookup<cov::function_coverage>(id, ec);
			if (ec || !funcs) return fallback;

			auto result = io::v1::stats::init();
			for (auto const& ref : funcs->merge_aliases()) {
				++result.relevant;
				if (ref.count) ++result.visited;
			}

			return result;
		}

		dir_entry make_file(std::string_view display,
		                    file_stats const& file,
		                    repository const* repo) {
			auto current = file.current;
			auto previous = file.previous;
			if (repo) {
				current.functions = recalc_functions(
				    *repo, file.current_functions, current.functions);
				if (!file.previous_functions.is_zero() &&
				    file.previous_functions == file.current_functions) {
					previous.functions = current.functions;
				} else {
					previous.functions = recalc_functions(
					    *repo, file.previous_functions, previous.functions);
				}
			}
			return {.result{.type = entry_type::file,
			                .name = make_label(display),
			                .stats{
			                    .current{current},
			                    .previous{previous},
			                },
			                .previous_name{file.previous_name},
			                .diff_kind{file.diff_kind}}};
		}

		auto find_entry(std::vector<dir_entry>& children,
		                entry_type type,
		                std::string_view display)
		    -> decltype(children.begin()) {
			return std::find_if(children.begin(), children.end(),
			                    [=](auto const& entry) {
				                    return entry.result.type == type &&
				                           entry.result.name.display == display;
			                    });
		}
	}  // namespace

	std::vector<entry> report_filter::project(
	    std::vector<file_stats> const& report,
	    cov::repository const* repo) const {
		std::vector<entry> result{};
		dir_entry root{.result{.type = entry_type::module}};

		auto files = file_projection(report);

		auto const is_standalone =
		    files.size() == 1 && files.front()->filename == fname.filter;
		if (is_standalone) {
			root.add_file(fname.filter, *files.front(), repo);
		} else {
			auto [modules, filtered_out] = modules_projection();
			root.add_files(modules, filtered_out, files, sep,
			               module.filter.empty(), fname.prefix.size(), repo);
		}

		root.propagate(sep);
		auto fname_filter = fname;
		if (is_standalone) {
			if (root.children.front().result.type == entry_type::directory) {
				root.swap_with_child();
			}

			root.children.front().result.type = entry_type::standalone_file;

			auto pos = fname.filter.rfind('/');
			if (pos == std::string::npos)
				fname_filter = {{}};
			else
				fname_filter = {fname.filter.substr(0, pos)};
		}
		result.reserve(root.children.size());
		for (auto& child : root.children) {
			child.result.name.expanded =
			    (child.result.type == entry_type::module ? module
			                                             : fname_filter)
			        .apply_to(child.result.name.display);
			result.push_back(std::move(child.result));
		}

		std::sort(result.begin(), result.end(),
		          [](auto const& lhs, auto const& rhs) {
			          if (lhs.type != rhs.type) {
				          // module -> directory -> file (no standalone on this
				          // path...)
				          return rhs.type < lhs.type;
			          }
			          return lhs.name.display < rhs.name.display;
		          });
		return result;
	}  // namespace cov::projection

	std::string_view report_filter::module_separator() const noexcept {
		auto result = mods ? mods->separator() : std::string_view{};
		if (result.empty()) result = "/"sv;
		return result;
	}

	std::vector<file_stats const*> report_filter::file_projection(
	    std::vector<file_stats> const& report) const {
		std::vector<file_stats const*> result{};
		result.reserve(report.size());

		for (auto const& entry : report) {
			if (!fname.prefixes(entry.filename)) continue;
			result.push_back(&entry);
		}

		return result;
	}  // GCOV_EXCL_LINE[GCC]

	std::pair<std::map<std::string, std::unordered_set<std::string>>,
	          std::unordered_set<std::string>>
	report_filter::modules_projection() const {
		std::pair<std::map<std::string, std::unordered_set<std::string>>,
		          std::unordered_set<std::string>>
		    results{};
		if (!mods) return results;

		auto& [modules, filtered_out] = results;

		for (auto const& entry : mods->entries()) {
			if (!module.prefixes(entry.name)) {
				update_prefixes(filtered_out, entry.prefixes);
				continue;
			}

			auto const pos = entry.name.find(sep, module.prefix.length());
			auto const name = pos == std::string::npos
			                      ? entry.name
			                      : entry.name.substr(0, pos);
			update_prefixes(modules[name], entry.prefixes);
		}

		return results;
	}  // GCOV_EXCL_LINE[GCC]
}  // namespace cov::projection
