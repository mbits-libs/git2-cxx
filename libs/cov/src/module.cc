// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/module.hh>
#include <git2/blob.hh>
#include <git2/commit.hh>
#include <map>
#include <unordered_set>

#define CHECK(EXPR)            \
	do {                       \
		if (ec = (EXPR); ec) { \
			[[unlikely]];      \
			return ec;         \
		}                      \
	} while (0)

#define UNLIKELY(TEST)    \
	do {                  \
		if (TEST) {       \
			[[unlikely]]; \
			return {};    \
		}                 \
	} while (0)

#define CHECK_OBJ(OBJ, EXPR) \
	auto const OBJ = EXPR;   \
	UNLIKELY(ec || !(OBJ))

namespace cov {
	using namespace std::literals;

	namespace {
		namespace names {
			static constexpr auto prefix = "module."sv;
			static constexpr auto path = ".path"sv;
			static constexpr char module_sep[] = "module.sep";
		};  // namespace names

		class modules_impl final : public counted_impl<modules> {
		public:
			modules_impl(std::string const& separator,
			             std::vector<module_info> const& mods)
			    : separator_{separator}, entries_{mods} {}

			std::string_view separator() const noexcept override {
				return separator_;
			}

			std::vector<module_info> const& entries() const noexcept override {
				return entries_;
			}

			template <typename Span, typename Conv>
			std::vector<module_view> filter_impl(Span const& files,
			                                     Conv conv) const {
				std::vector<module_view> result{};
				result.resize(entries_.size() + 1);

				for (auto it = result.begin(); auto const& mod : entries_) {
					(*it++).name = mod.name;
				}

				for (auto const& file : files) {
					auto const ptr = conv(file);
					auto const filename = ptr->path();
					auto found = false;
					for (auto it = result.begin(); auto const& mod : entries_) {
						auto& view = *it++;
						if (!contains(mod.prefixes, filename)) continue;
						view.items.push_back(ptr);
						found = true;
					}
					if (!found) result.back().items.push_back(ptr);
				}

				std::erase_if(result, [](auto const& view) {
					return view.items.empty();
				});

				return result;
			}  // GCOV_EXCL_LINE[GCC]

			std::vector<module_view> filter(
			    std::span<report_entry const*> const& files) const override {
				return filter_impl(files,
				                   [](report_entry const* ptr) { return ptr; });
			}

			std::vector<module_view> filter(
			    std::vector<std::unique_ptr<report_entry>> const& files)
			    const override {
				return filter_impl(
				    files, [](std::unique_ptr<report_entry> const& ptr) {
					    return ptr.get();
				    });
			}

			mod_errc set_separator(std::string const& sep) override {
				if (sep == separator_) return mod_errc::unmodified;
				separator_ = sep;
				return mod_errc::needs_update;
			}

			mod_errc add(std::string const& mod,
			             std::string const& path) override {
				for (auto& entry : entries_) {
					if (entry.name != mod) continue;
					for (auto const& prefix : entry.prefixes) {
						if (prefix == path) return mod_errc::duplicate;
					}
					entry.prefixes.push_back(path);
					return mod_errc::needs_update;
				}  // GCOV_EXCL_LINE[WIN32]
				entries_.push_back({mod, {path}});
				return mod_errc::needs_update;
			}

			mod_errc remove(std::string const& mod,
			                std::string const& path) override {
				auto mod_it = std::find_if(
				    entries_.begin(), entries_.end(),
				    [&](auto const& entry) { return entry.name == mod; });
				if (mod_it == entries_.end()) return mod_errc::no_module;

				auto result = mod_errc::unmodified;
				auto path_id = std::find(mod_it->prefixes.begin(),
				                         mod_it->prefixes.end(), path);
				if (path_id != mod_it->prefixes.end()) {
					mod_it->prefixes.erase(path_id);
					result = mod_errc::needs_update;
				}

				if (mod_it->prefixes.empty()) {
					entries_.erase(mod_it);
					result = mod_errc::needs_update;
				}
				return result;
			}

			mod_errc remove_all(std::string const& mod) override {
				auto it = std::find_if(
				    entries_.begin(), entries_.end(),
				    [&](auto const& entry) { return entry.name == mod; });
				if (it == entries_.end()) return mod_errc::unmodified;
				entries_.erase(it);
				return mod_errc::needs_update;
			}

			std::error_code dump(
			    git::config const& cfg) const noexcept override {
				std::error_code ec{};

				// Before lock, because most likely the config already contains
				// exactly the same separator; this would remove the separator
				// on transaction commit, but the set_string("module.sep") would
				// see there is no need to change anything and not even register
				// operation in the transaction.
				ec = cfg.delete_multivar(names::module_sep, ".*");
				if (ec == git::errc::notfound) ec.clear();
				UNLIKELY(ec);

				auto tx = cfg.lock(ec);
				if (ec) {
					[[unlikely]];  // GCOV_EXCL_LINE
					return ec;     // GCOV_EXCL_LINE
				}

				std::unordered_set<std::string> to_remove{};
				CHECK(cfg.foreach_entry([&](git_config_entry const* entry) {
					auto view = std::string_view{entry->name};
					if (view.starts_with(names::prefix)) {
						auto key = view.substr(names::prefix.size());
						if (key.contains('.'))
							to_remove.insert({view.data(), view.size()});
					}
					return 0;
				}));

				for (auto const& key : to_remove) {
					CHECK(cfg.delete_multivar(key.c_str(), ".*"));
				}

				if (!separator_.empty())
					CHECK(cfg.set_string(names::module_sep, separator_));

				for (auto const& mod : entries_) {
					std::string key{};
					key.reserve(names::prefix.size() + names::path.size() +
					            mod.name.size());
					key.append(names::prefix);
					key.append(mod.name);
					key.append(names::path);
					for (auto const& prefix : mod.prefixes)
						CHECK(cfg.set_multivar(key.c_str(), "^$",
						                       prefix.c_str()));
				}

				return tx.commit();
			}

		private:
			static bool contains(std::vector<std::string> const& prefixes,
			                     std::string_view path) {
				for (auto const& pre : prefixes) {
					if (contains(pre, path)) return true;
				}
				return false;
			}

			static bool contains(std::string_view prefix,
			                     std::string_view path) {
				if (prefix.size() > path.size()) return false;
				if (prefix.size() == path.size()) return prefix == path;
				if (!path.starts_with(prefix)) return false;
				return (!prefix.empty() && prefix.back() == '/') ||
				       (path[prefix.size()] == '/');
			}

			std::string separator_;
			std::vector<module_info> entries_;
		};
	}  // namespace

	ref_ptr<modules> modules::make_modules(
	    std::string const& separator,
	    std::vector<module_info> const& mods) {
		return make_ref<modules_impl>(separator, mods);
	}

	ref_ptr<modules> modules::from_config(git::config const& cfg,
	                                      std::error_code& ec) {
		std::vector<module_info> result{};
		std::map<std::string, size_t, std::less<>> positions{};

		std::string sep;
		ec = cfg.foreach_entry([&](git_config_entry const* entry) {
			auto view = std::string_view{entry->name};
			if (view == names::module_sep) {
				sep = entry->value;
				return 0;
			}
			if (view.starts_with(names::prefix)) {
				auto key = view.substr(names::prefix.size());
				if (key.ends_with(names::path)) {
					auto mod_name =
					    key.substr(0, key.size() - names::path.size());
					if (!mod_name.empty()) {
						auto it = positions.lower_bound(mod_name);
						if (it == positions.end() || it->first != mod_name) {
							auto key_str =
							    std::string{mod_name.data(), mod_name.size()};
							it = positions.insert(it, {key_str, result.size()});
							result.push_back({key_str, {}});
						}
						result[it->second].prefixes.push_back(entry->value);
					}
				}
			}
			return 0;
		});
		UNLIKELY(ec);

		return make_modules(sep, result);
	}

	ref_ptr<modules> modules::from_commit(git_oid const& ref,
	                                      git::repository_handle const& repo,
	                                      std::error_code& ec) {
		CHECK_OBJ(commit, repo.lookup<git::commit>(ref, ec));
		CHECK_OBJ(tree, commit.tree(ec));
		CHECK_OBJ(entry, tree.entry_bypath(".covmodule", ec));
		CHECK_OBJ(blob, repo.lookup<git::blob>(entry.oid(), ec));

		auto const bytes = blob.raw();

		CHECK_OBJ(cfg, git::config::create(ec));

		ec = cfg.add_memory(
		    {reinterpret_cast<char const*>(bytes.data()), bytes.size()},
		    GIT_CONFIG_LEVEL_APP);
		UNLIKELY(ec);

		return from_config(cfg, ec);
	}

#undef UNLIKELY
#undef CHECK_OBJ
#undef CHECK

}  // namespace cov
