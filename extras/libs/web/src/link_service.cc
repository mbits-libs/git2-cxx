// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <concepts>
#include <string>
#include <web/link_service.hh>

using namespace std::literals;

namespace cov::app::web {
	namespace {
		template <typename Char, Char OsSep>
		auto os_normalized(std::string_view path) {
			if constexpr (std::same_as<Char, char> && OsSep == '/') {
				return path;
			} else {
				std::string result{};
				result.reserve(path.size());
				for (auto const c : path) {
					if (c == OsSep)
						result.push_back('/');
					else
						result.push_back(c);
				}
				return result;
			}
		}

		auto normalized(std::string_view resource_path) {
			using std::filesystem::path;
			return os_normalized<path::value_type, path::preferred_separator>(
			    resource_path);
		}

		std::string path_of(projection::entry_type type,
		                    projection::label const& name,
		                    std::string_view ext) {
			std::string_view cat{};
			switch (type) {
				case projection::entry_type::module:
					cat = "mods"sv;
					break;
				case projection::entry_type::directory:
					cat = "dirs"sv;
					break;
				case projection::entry_type::standalone_file:
				case projection::entry_type::file:
					cat = "files"sv;
					break;
			}

			return fmt::format("{}/{}{}", cat, normalized(name.expanded), ext);
		}
	}  // namespace

	std::string link_service::link_for(projection::entry_type type,
	                                   projection::label const& name) const {
		if (type == projection::entry_type::standalone_file) return {};
		return resource_link(path_of(type, name));
	}

	std::string export_link_service::path_of(
	    projection::entry_type type,
	    projection::label const& name) const {
		return web::path_of(type, name, ".html"sv);
	}

	std::string export_link_service::resource_link(
	    std::string_view resource) const {
		return fmt::format("{}/{}", root_, normalized(resource));
	}

	void export_link_service::adjust_root(
	    std::filesystem::path const& current_file_path) {
		auto const dir_name = current_file_path.parent_path();
		root_.clear();
		if (dir_name != ".") {
			size_t counter = 0;
			for ([[maybe_unused]] auto const& _ : dir_name) {
				++counter;
			}

			static constexpr auto up_dir = "../"sv;
			root_.reserve(counter * up_dir.size());
			for (size_t index = 0; index < counter; ++index) {
				root_.append(up_dir);
			}
		}
		if (root_.empty())
			root_ = ".";
		else
			root_.pop_back();
	}

	std::string server_link_service::path_of(
	    projection::entry_type type,
	    projection::label const& name) const {
		return web::path_of(type, name, {});
	}

	std::string server_link_service::resource_link(
	    std::string_view resource) const {
		return fmt::format("{}/{}", root_, normalized(resource));
	}

	void server_link_service::set_app_path(std::string_view root_path) {
		if (!root_path.empty() && root_path.back() == '/')
			root_path = root_path.substr(0, root_path.length() - 1);
		root_.assign(root_path);
	}

}  // namespace cov::app::web
