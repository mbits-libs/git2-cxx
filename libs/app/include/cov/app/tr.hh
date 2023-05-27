// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <fmt/format.h>
#include <args/translator.hpp>
#include <cov/app/strings/args.hh>
#include <span>
#include <string>
#include <string_view>

namespace cov::app::str {
	template <std::size_t n>
	struct fixed_string {
		constexpr fixed_string() = default;
		constexpr fixed_string(const char (&str)[n + 1]) noexcept {
			auto i = std::size_t{0};
			for (char const c : str) {
				_data[i++] = c;
			}
			_data[n] = static_cast<char>(0);
		}

		friend constexpr auto operator<=>(fixed_string const&,
		                                  fixed_string const&) = default;
		friend constexpr auto operator==(fixed_string const&,
		                                 fixed_string const&) -> bool = default;

		[[nodiscard]] static constexpr auto size() noexcept -> std::size_t {
			return n;
		}

		[[nodiscard]] static constexpr auto empty() noexcept -> bool {
			return n == 0;
		}

		constexpr auto data() const& noexcept -> char const* { return _data; }

		constexpr auto data() & noexcept -> char* { return _data; }

		constexpr auto begin() const& noexcept -> char const* { return _data; }

		constexpr auto end() const& noexcept -> char const* {
			return _data + n;
		}

		constexpr auto begin() & noexcept -> char* { return _data; }

		constexpr auto end() & noexcept -> char* { return _data + n; }

		constexpr auto operator[](std::size_t index) noexcept {
			return _data[index];
		}

		char _data[n + 1];
	};

	// template argument deduction
	template <std::size_t n>
	fixed_string(char const (&)[n]) -> fixed_string<n - 1>;

	struct args_strings : public str::args::Strings {
		std::string args(::args::lng id,
		                 std::string_view arg1,
		                 std::string_view arg2) const;
	};

}  // namespace cov::app::str

namespace cov::app {
	template <typename Enum>
	struct lngs_traits;

	template <typename Enum, str::fixed_string FileName, typename Strings>
	struct base_lngs_traits {
		using type = Enum;
		using strings = Strings;
		static constexpr std::string_view file_name() noexcept {
			return {FileName.data(), FileName.size()};
		}
	};

	template <>
	struct lngs_traits<str::args::lng>
	    : base_lngs_traits<str::args::lng, "args", str::args_strings> {};
}  // namespace cov::app

namespace cov::app::str {
	template <typename Enum>
	requires std::is_enum_v<Enum>
	struct strings : lngs_traits<Enum>::strings {
		using base = typename lngs_traits<Enum>::strings;
		using base::operator();

		void setup_path_manager(std::filesystem::path const& base) {
			auto const file_name = lngs_traits<Enum>::file_name();
			base::init_builtin();
			base::template path_manager<lngs::manager::SubdirPath>(
			    base, std::string{file_name.data(), file_name.size()});
		}
	};

	template <typename... Enum>
	requires(std::is_enum_v<Enum>&&...) struct with : strings<Enum>... {
		using strings<Enum>::operator()...;

		void setup_path_manager(std::filesystem::path const& base) {
			(strings<Enum>::setup_path_manager(base), ...);
		}

		void open_first_of(std::span<std::string const> const& langs) {
			(strings<Enum>::open_first_of(langs), ...);
		}
	};

	struct translator_open_info {
		std::filesystem::path locale_dir;
		std::span<std::string const> langs;
	};

	template <typename Strings>
	class translator : public Strings, private ::args::base_translator {
	public:
		explicit translator(str::translator_open_info const& langs) {
			Strings::setup_path_manager(langs.locale_dir);
			Strings::open_first_of(langs.langs);
		}

		using Strings::operator();
		::args::base_translator const* args() const noexcept { return this; }

		static translator const& from(
		    ::args::base_translator const& args_transaltor) noexcept {
			return static_cast<translator const&>(args_transaltor);
		}

		template <typename Enum, typename... Args>
		std::string format(Enum id,
		                   Args... args) const requires std::is_enum_v<Enum> {
			return fmt::vformat((*this)(id), fmt::make_format_args(args...));
		}

		template <typename Enum, typename... Args>
		void print(Enum id, Args... args) const requires std::is_enum_v<Enum> {
			fmt::vprint((*this)(id), fmt::make_format_args(args...));
		}

		template <typename Enum, typename... Args>
		void print(FILE* out,
		           Enum id,
		           Args... args) const requires std::is_enum_v<Enum> {
			fmt::vprint(out, (*this)(id), fmt::make_format_args(args...));
		}

	private:
		std::string operator()(::args::lng id,
		                       std::string_view arg1,
		                       std::string_view arg2) const override {
			return Strings::args(id, arg1, arg2);
		}
	};

	template <typename... Enum>
	requires(std::is_enum_v<Enum>&&...) using args_translator =
	    translator<with<str::args::lng, Enum...>>;
}  // namespace cov::app::str
