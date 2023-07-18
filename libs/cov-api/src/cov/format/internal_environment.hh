// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4189)
#endif
#include <fmt/format.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"
#endif
#include <date/tz.h>
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
#include <cov/format.hh>
#include <optional>
#include <string>
#include <string_view>

namespace cov::placeholder {
	struct internal_environment {
		environment const* client;
		void* app;
		std::string (*tr)(long long count, translatable scale, void* app);
		date::time_zone const* tz;
		iterator formatted_output(iterator out, std::string_view view);
		std::optional<width> current_width{};

		std::string translate(long long count, translatable scale) const {
			return tr(count, scale, app);
		}

		std::string translate(translatable scale) const {
			return tr(0, scale, app);
		}
	};
}  // namespace cov::placeholder
