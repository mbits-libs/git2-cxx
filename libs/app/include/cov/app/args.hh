// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <git2/errors.h>
#include <args/parser.hpp>
#include <concepts>
#include <cov/app/dirs.hh>
#include <cov/app/strings/args.hh>
#include <cov/app/strings/errors.hh>
#include <cov/app/tr.hh>
#include <git2/error.hh>
#include <string>
#include <utility>
#include <variant>

namespace cov::app {
	class parser_holder {
	public:
		parser_holder(::args::args_view const& arguments,
		              std::string const& description,
		              ::args::base_translator const* tr)
		    : parser_{description, arguments, tr} {}

		void parse() { parser_.parse(); }

		[[noreturn]] void error(std::string const& msg) const {
			parser_.error(msg);
		}  // GCOV_EXCL_LINE[WIN32]

		template <typename Strings>
		requires std::derived_from<Strings, str::errors::Strings> &&
		    std::derived_from<Strings, str::args::Strings>
		[[noreturn]] void error(std::error_code const& ec,
		                        Strings const& str) const {
			error(ec, str, str);
		}  // GCOV_EXCL_LINE[WIN32]
		[[noreturn]] void error(std::error_code const& ec,
		                        str::errors::Strings const&,
		                        str::args::Strings const&) const;

		template <typename Strings>
		requires std::derived_from<Strings, str::errors::Strings> &&
		    std::derived_from<Strings, str::args::Strings>
		        std::string message(std::error_code const& ec,
		                            Strings const& str)
		const { return message(ec, str, str); }

		static std::pair<char const*, std::string> message_from_libgit(
		    str::errors::Strings const&,
		    git::errc = git::errc{});

	protected:
		::args::parser parser_;

	private:
		std::string message(std::error_code const& ec,
		                    str::errors::Strings const&,
		                    str::args::Strings const&) const;
	};

	template <typename... Enum>
	requires(std::is_enum_v<Enum>&&...) class translator_holder {
	public:
		translator_holder(str::translator_open_info const& langs)
		    : tr_{langs} {}

		static str::args_translator<Enum...> const& tr(
		    ::args::parser const& p) noexcept {
			return str::args_translator<Enum...>::from(p.tr());
		}
		str::args_translator<Enum...> const& tr() const noexcept { return tr_; }

	protected:
		str::args_translator<Enum...> tr_;
	};

	template <typename T>
	struct opt {
		T payload;
	};
	template <>
	struct opt<char const*>;

	inline std::string to_string(std::string_view view) {
		return {view.data(), view.size()};
	}

	template <typename... Enum>
	requires(std::is_enum_v<Enum>&&...) struct str_visitor {
		str::args_translator<Enum...> const& tr;
		template <typename String>
		std::string_view visit(String const& s) const {
			using base = typename String::base;
			return std::visit(*this, static_cast<base const&>(s));
		}
		std::string_view operator()(std::string_view view) const noexcept {
			return view;
		}
		std::string_view operator()(std::string const& str) const noexcept {
			return str;
		}
		std::string_view operator()(auto id) const noexcept { return tr(id); }
	};

	template <typename String>
	struct args_description {
		String args;
		String description;
		std::optional<String> meta1{};
		std::optional<String> meta2{};

		using str_visitor = typename String::str_visitor;

		std::pair<std::string, std::string> with(
		    str_visitor const& visitor) const {
			auto const arguments = visitor.visit(args);
			auto const descr = visitor.visit(description);
			std::string stg_m1, stg_m2;
			if (meta1) {
				auto m1 = visitor.visit(*meta1);
				if (meta1->is_optional) {
					stg_m1 = fmt::format("[{}]", m1);
					m1 = stg_m1;
				}
				if (meta2) {
					auto m2 = visitor.visit(*meta2);
					if (meta2->is_optional) {
						stg_m2 = fmt::format("[{}]", m2);
						m2 = stg_m2;
					}
					return {
					    fmt::format("{} {} {}", arguments, m1, m2),
					    to_string(descr),
					};
				}
				return {fmt::format("{} {}", arguments, m1), to_string(descr)};
			}
			return {to_string(arguments), to_string(descr)};
		}

		template <typename ArgsTranslator, size_t Length>
		static void group(ArgsTranslator const& tr,
		                  String const& title,
		                  args_description<String> const (&items)[Length],
		                  ::args::chunk& out) {
			str_visitor visitor{tr};
			out.title = to_string(visitor.visit(title));
			out.items.reserve(Length);
			for (auto const& item : items) {
				out.items.push_back(item.with(visitor));
			}
		}
	};

	template <typename... Enum>
	requires(std::is_enum_v<Enum>&&...) struct string
	    : std::variant<std::string_view, str::args::lng, Enum...> {
		using str_visitor = app::str_visitor<Enum...>;
		using args_description = app::args_description<string>;

		using base = std::variant<std::string_view, str::args::lng, Enum...>;
		using base::base;
		string(const char*) = delete;
		string(std::string const&) = delete;

		template <typename Arg>
		requires(
		    std::same_as<Arg, Enum> || ... ||
		    (std::same_as<Arg, str::args::lng> ||
		     std::same_as<Arg, std::string_view>)) constexpr string(opt<Arg>
		                                                                value)
		    : base{value.payload}, is_optional{true} {}

		bool is_optional{false};
	};

	template <typename... Enum>
	requires(std::is_enum_v<Enum>&&...) struct rt_string
	    : std::variant<std::string, std::string_view, str::args::lng, Enum...> {
		using str_visitor = app::str_visitor<Enum...>;
		using args_description = app::args_description<rt_string>;

		using base = std::
		    variant<std::string, std::string_view, str::args::lng, Enum...>;
		using base::base;
		rt_string(const char*) = delete;

		template <typename Arg>
		requires(
		    std::same_as<Arg, Enum> || ... ||
		    (std::same_as<Arg, str::args::lng> ||
		     std::same_as<Arg,
		                  std::string_view>)) constexpr rt_string(opt<Arg>
		                                                              value)
		    : base{value.payload}, is_optional{true} {}
		rt_string(opt<std::string> value)
		    : base{std::move(value.payload)}, is_optional{true} {}

		bool is_optional{false};
	};

	template <typename... Enum>
	requires(std::is_enum_v<Enum>&&...) class base_parser
	    : public translator_holder<Enum...>,
	      public parser_holder {
	public:
		using string = app::string<Enum...>;
		using rt_string = app::rt_string<Enum...>;
		using args_description = string::args_description;
		using rt_args_description = rt_string::args_description;

		base_parser(str::translator_open_info const& langs,
		            ::args::args_view const& arguments,
		            std::string const& description = {})
		    : translator_holder<Enum...>{langs}
		    , parser_holder{arguments, description, this->tr_.args()} {}
	};
}  // namespace cov::app
