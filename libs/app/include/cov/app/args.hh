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
#include <cov/error.hh>
#include <git2/error.hh>
#include <string>
#include <utility>
#include <variant>

namespace cov::app {
	[[noreturn]] void simple_error(str::args::Strings const& tr,
	                               std::string_view tool,
	                               std::string_view message);

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
		                    Strings const& str) const {
			return message(ec, str, str);
		}

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
	    requires(std::is_enum_v<Enum> && ...)
	class translator_holder {
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
		constexpr opt(T payload) : payload(payload) {}
	};
	template <>
	struct opt<char const*>;

	template <typename T>
	struct multi {
		T payload;
		constexpr multi(T payload) : payload(payload) {}
	};
	template <>
	struct multi<char const*>;

	template <typename T>
	struct opt_multi {
		T payload;
		constexpr opt_multi(T payload) : payload(payload) {}
	};
	template <>
	struct opt_multi<char const*>;

	template <typename T>
	struct multi<opt<T>> : opt_multi<T> {};

	template <typename T>
	struct opt<multi<T>> : opt_multi<T> {};

	inline std::string to_string(std::string_view view) {
		return {view.data(), view.size()};
	}

	template <typename... Enum>
	    requires(std::is_enum_v<Enum> && ...)
	struct str_visitor {
		str::args_translator<Enum...> const& tr;
		str_visitor(str::args_translator<Enum...> const& tr) : tr{tr} {}
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
			if (meta1) {
				auto const m1 = meta1->with(visitor);
				if (meta2) {
					auto const m2 = meta2->with(visitor);
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
	    requires(std::is_enum_v<Enum> && ...)
	struct string : std::variant<std::string_view, str::args::lng, Enum...> {
		using str_visitor = app::str_visitor<Enum...>;
		using args_description = app::args_description<string>;

		using base = std::variant<std::string_view, str::args::lng, Enum...>;
		using base::base;
		string(const char*) = delete;
		string(std::string const&) = delete;

		template <typename Arg>
		    requires(std::same_as<Arg, Enum> || ... ||
		             (std::same_as<Arg, str::args::lng> ||
		              std::same_as<Arg, std::string_view>))
		constexpr string(opt<Arg> value)
		    : base{value.payload}, is_optional{true} {}

		template <typename Arg>
		    requires(std::same_as<Arg, Enum> || ... ||
		             (std::same_as<Arg, str::args::lng> ||
		              std::same_as<Arg, std::string_view>))
		constexpr string(multi<Arg> value)
		    : base{value.payload}, is_multi{true} {}

		template <typename Arg>
		    requires(std::same_as<Arg, Enum> || ... ||
		             (std::same_as<Arg, str::args::lng> ||
		              std::same_as<Arg, std::string_view>))
		constexpr string(opt_multi<Arg> value)
		    : base{value.payload}, is_optional{true}, is_multi{true} {}

		template <typename Arg>
		    requires(std::same_as<Arg, Enum> || ... ||
		             (std::same_as<Arg, str::args::lng> ||
		              std::same_as<Arg, std::string_view>))
		constexpr string(opt<multi<Arg>> value)
		    : base{value.payload.payload}, is_optional{true}, is_multi{true} {}

		bool is_optional{false};
		bool is_multi{false};

		std::string with(str_visitor const& visitor) const {
			std::string stg;
			stg.assign(visitor.visit(*this));
			if (is_multi) stg = fmt::format("{}...", stg);
			if (is_optional) stg = fmt::format("[{}]", stg);

			return stg;
		}  // GCOV_EXCL_LINE
	};

	template <typename... Enum>
	    requires(std::is_enum_v<Enum> && ...)
	class base_parser : public translator_holder<Enum...>,
	                    public parser_holder {
	public:
		using string = app::string<Enum...>;
		using args_description = typename string::args_description;

		base_parser(str::translator_open_info const& langs,
		            ::args::args_view const& arguments,
		            std::string const& description = {})
		    : translator_holder<Enum...>{langs}
		    , parser_holder{arguments, description, this->tr_.args()} {}
	};
}  // namespace cov::app
