// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/projection.hh>
#include <string>

namespace cov::app::web {
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4265)
#endif

	struct link_service {
		virtual std::string path_of(projection::entry_type type,
		                            projection::label const& name) const = 0;
		virtual std::string link_for(projection::entry_type type,
		                             projection::label const& name) const;
		virtual std::string resource_link(std::string_view resource) const = 0;

	protected:
		~link_service() = default;
	};

	struct export_link_service : link_service {
		std::string path_of(projection::entry_type type,
		                    projection::label const& name) const override;
		std::string resource_link(std::string_view resource) const override;

		void adjust_root(std::filesystem::path const& current_file_path);

	private:
		std::string root_;
	};

	struct server_link_service : link_service {
		std::string path_of(projection::entry_type type,
		                    projection::label const& name) const override;
		std::string resource_link(std::string_view resource) const override;

		void set_app_path(std::string_view root_path);

	private:
		std::string root_;
	};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
}  // namespace cov::app::web
