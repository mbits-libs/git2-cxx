// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <memory>
#include <native/expat.hh>
#include <string>
#include <toolkit.hh>
#include <utility>

namespace cov::app::collect {
	struct msvc_context {
		config const& cfg;
		coverage& cvg;
		std::string source{};
		std::unique_ptr<coverage_file> sink{};
	};

	struct handler_interface {
		msvc_context* ctx{};
		handler_interface(msvc_context* ctx) : ctx{ctx} {}
		virtual ~handler_interface();
		virtual void onElement(char const** attrs);
		virtual std::unique_ptr<handler_interface> onChild(std::string_view);
		virtual void onCharacter(std::string_view);
		virtual void onStop();
	};

	template <typename... Children>
	struct find_helper;

	template <>
	struct find_helper<> {
		static std::unique_ptr<handler_interface> find(std::string_view,
		                                               msvc_context*) {
			return {};
		}
	};

	template <typename First, typename... Children>
	struct find_helper<First, Children...> {
		static std::unique_ptr<handler_interface> find(std::string_view name,
		                                               msvc_context* ctx) {
			if (name == First::name()) return std::make_unique<First>(ctx);
			return find_helper<Children...>::find(name, ctx);
		}
	};

	template <typename... Children>
	struct handler_impl : handler_interface {
		using handler_interface::handler_interface;
		std::unique_ptr<handler_interface> onChild(
		    std::string_view name) override {
			return find_helper<Children...>::find(name, this->ctx);
		}
	};

	bool occ_load(std::filesystem::path const& filename,
	              config const& cfg,
	              coverage& cvg);
}  // namespace cov::app::collect
