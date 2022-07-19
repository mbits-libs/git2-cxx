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

		template <typename Strings>
		requires std::derived_from<Strings, str::errors::Strings> &&
		    std::derived_from<Strings, str::args::Strings>
		        std::string message(std::error_code const& ec,
		                            Strings const& str)
		const { return message(ec, str, str); }

		static std::pair<char const*, std::string> message_from_libgit(
		    str::errors::Strings const&);

	protected:
		::args::parser parser_;

	private:
		[[noreturn]] void error(std::error_code const& ec,
		                        str::errors::Strings const&,
		                        str::args::Strings const&) const;
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

	template <typename... Enum>
	requires(std::is_enum_v<Enum>&&...) class base_parser
	    : public translator_holder<Enum...>,
	      public parser_holder {
	public:
		base_parser(str::translator_open_info const& langs,
		            ::args::args_view const& arguments,
		            std::string const& description = {})
		    : translator_holder<Enum...>{langs}
		    , parser_holder{arguments, description, this->tr_.args()} {}
	};
}  // namespace cov::app
