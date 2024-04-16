// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <git2/errors.h>
#include <cov/app/args.hh>
#include <regex>
#include <span>

namespace cov::app {
	namespace {
		using namespace std::literals;
		using errlng = str::errors::lng;

		template <typename Enum>
		struct error_code_lookup {
			errlng id;
			Enum ec;
			std::string_view message{};
		};

		struct error_lookup {
			errlng id;
			std::string_view message;
		};

		struct error_category {
			git_error_t cat;
			const char* domain;
			std::span<error_lookup const> direct{};
			std::span<error_lookup const> regex{};
		};

		static constexpr error_code_lookup<git::errc> git_general_direct[] = {
		    {errlng::SHOW_EMPTY_HEAD, git::errc::unbornbranch},
		    {errlng::SHOW_NOT_FOUND, git::errc::notfound},
		};

		static constexpr error_code_lookup<git::errc> git_general_regex[] = {
		    {errlng::SHOW_INVALID_PATTERN, git::errc::invalidspec,
		     "Invalid pattern '(.*)'"sv},
		};

		static constexpr error_code_lookup<cov::errc> cov_general_direct[] = {
		    {errlng::SHOW_NOT_BRANCH, cov::errc::not_a_branch},
		};

		static constexpr error_lookup config_direct[] = {
		    {errlng::CONFIG_NON_UNIQUE_MULTIVAR,
		     "entry is not unique due to being a multivar"sv},
		    {errlng::CONFIG_NON_UNIQUE_INCLUDED,
		     "entry is not unique due to being included"sv},
		};

		static constexpr error_lookup config_regex[] = {
		    {errlng::CONFIG_CANNOT_DELETE,
		     "could not find key '(.*)' to delete"sv},
		    {errlng::CONFIG_INVALID_NAME, "invalid config item name '(.*)'"sv},
		};

		static constexpr error_category known_errors[] = {
		    {GIT_ERROR_CONFIG, nullptr, config_direct, config_regex},
		    {GIT_ERROR_REGEX, "regex"},
		};
	}  // namespace

	[[noreturn]] void simple_error(str::args::Strings const& tr,
	                               std::string_view tool,
	                               std::string_view message) {
		auto const fmt = tr(str::args::lng::ERROR_MSG);
		auto const msg = fmt::format(fmt::runtime(fmt), tool, message);
		std::fputs(msg.c_str(), stderr);
		std::fputc('\n', stderr);
		std::exit(2);
	}  // GCOV_EXCL_LINE

	[[noreturn]] void parser_holder::error(
	    std::error_code const& ec,
	    str::errors::Strings const& tr,
	    str::args::Strings const& args) const {
		auto const msg = message(ec, tr, args);
		std::fputs(msg.c_str(), stderr);
		std::fputc('\n', stderr);
		std::exit(2);
	}  // GCOV_EXCL_LINE

	std::string parser_holder::message(std::error_code const& ec,
	                                   str::errors::Strings const& tr,
	                                   str::args::Strings const& args) const {
		if (ec.category() == git::category()) {
			auto [domain, message] =
			    message_from_libgit(tr, static_cast<git::errc>(ec.value()));
			if (!message.empty()) {
				if (domain && *domain) {
					auto fmt = tr(str::errors::lng::DOMAIN_ERROR_MSG);
					return fmt::format(fmt::runtime(fmt), parser_.program(),
					                   domain, message);
				}
				auto fmt = args(str::args::lng::ERROR_MSG);
				return fmt::format(fmt::runtime(fmt), parser_.program(),
				                   message);
			}
		}  // GCOV_EXCL_LINE[WIN32]

		if (ec.category() == cov::category()) {
			auto message =
			    message_from_cov_api(tr, static_cast<cov::errc>(ec.value()));
			if (!message.empty()) {
				auto fmt = args(str::args::lng::ERROR_MSG);
				return fmt::format(fmt::runtime(fmt), parser_.program(),
				                   message);
			}
		}  // GCOV_EXCL_LINE[WIN32]

		auto cat_name = ec.category().name();
		auto fmt = tr(str::errors::lng::DOMAIN_ERROR_MSG);
		return fmt::format(fmt::runtime(fmt), parser_.program(), cat_name,
		                   platform::con_to_u8(ec));
	}

	static std::string from_regex(str::errors::Strings const& tr,
	                              errlng id,
	                              std::string_view regex,
	                              std::string const& message,
	                              bool& matches) {
		matches = false;
		auto re = std::regex{std::string{regex.data(), regex.size()}};
		std::smatch match;
		if (!std::regex_match(message, match, re)) return {};

		matches = true;
		auto const fmt = tr(id);
		std::vector<std::string> match_copies{};
		match_copies.reserve(match.size() - 1);
		for (size_t index = 1; index < match.size(); ++index)
			match_copies.push_back(match.str(index));

		std::vector<fmt::basic_format_arg<fmt::format_context>> vargs{};
		vargs.reserve(match_copies.size());
		std::transform(match_copies.begin(), match_copies.end(),
		               std::back_inserter(vargs),
		               [](std::string_view volatile_view) {
			               return fmt::detail::make_arg<fmt::format_context>(
			                   volatile_view);
		               });
		fmt::format_args args{vargs.data(), static_cast<int>(vargs.size())};
		return fmt::vformat(fmt, args);
	}

	template <typename Enum, size_t Length>
	std::string message_from_errc(
	    str::errors::Strings const& tr,
	    Enum ec,
	    error_code_lookup<Enum> const (&map)[Length]) {
		for (auto const& [id, err, _] : map) {
			if (err != ec) continue;
			auto const view = tr(id);
			return {view.data(), view.size()};
		}  // GCOV_EXCL_LINE[WIN32]

		return {};
	}

	std::pair<char const*, std::string> parser_holder::message_from_libgit(
	    str::errors::Strings const& tr,
	    git::errc ec) {
		auto const error = git_error_last();

		if (!error || !error->message || !*error->message) {
			return {nullptr, message_from_errc(tr, ec, git_general_direct)};
		}
		auto const cat = error->klass;
		auto message = std::string{error->message};

		for (auto const& group : known_errors) {
			if (group.cat != cat) continue;

			for (auto const& [id, msg] : group.direct) {
				if (msg != message) continue;
				auto const view = tr(id);
				return {group.domain, {view.data(), view.size()}};
			}  // GCOV_EXCL_LINE[WIN32]

			for (auto const& [id, regex] : group.regex) {
				bool matches = false;
				auto result = from_regex(tr, id, regex, message, matches);
				if (matches) return {group.domain, std::move(result)};
			}  // GCOV_EXCL_LINE[WIN32]

			return {group.domain, std::move(message)};
		}  // GCOV_EXCL_LINE[WIN32]

		for (auto const& [id, err, _] : git_general_direct) {
			if (err != ec) continue;
			auto const view = tr(id);
			return {nullptr, {view.data(), view.size()}};
		}  // GCOV_EXCL_LINE[WIN32]

		for (auto const& [id, err, regex] : git_general_regex) {
			if (err != ec) continue;
			bool matches = false;
			auto result = from_regex(tr, id, regex, message, matches);
			if (matches) return {nullptr, std::move(result)};
		}  // GCOV_EXCL_LINE[WIN32]

		return {nullptr, std::move(message)};
	}

	std::string parser_holder::message_from_cov_api(
	    str::errors::Strings const& tr,
	    cov::errc ec) {
		return message_from_errc(tr, ec, cov_general_direct);
	}
}  // namespace cov::app
