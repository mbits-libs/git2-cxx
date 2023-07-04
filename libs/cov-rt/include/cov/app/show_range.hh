// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <cov/app/args.hh>
#include <cov/app/cov_log_tr.hh>
#include <cov/format_args.hh>
#include <cov/revparse.hh>
#include <string>

namespace cov::app {
	enum class known_format : unsigned {
		oneline,
		short_,
		medium,
		full,
		fuller,
		reference,
		raw,
		custom,
	};

	struct show_range {
		color_feature color_type{};
		decorate_feature decorate{};
		known_format selected_format{known_format::medium};
		std::string custom_format{};
		bool abbrev_hash{false};

		void add_args(::args::parser& p,
		              LogStrings const& tr,
		              bool for_log = true);

		auto select_format() {
			return [this](std::string_view arg) { select_format(arg); };
		}

		auto oneline() {
			return [this]() {
				selected_format = known_format::oneline;
				abbrev_hash = true;
			};
		}

		void select_format(std::string_view format);
		std::string_view format_str() const noexcept;
		decorate_feature selected_decorate() const noexcept;

		void print(cov::repository const& repo,
		           cov::revs const& range,
		           std::optional<unsigned> max_count) const;
	};
}  // namespace cov::app
