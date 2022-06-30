// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/io/db_object.hh>
#include <cov/report.hh>

namespace cov::io::handlers {
	struct line_coverage : db_handler_for<cov::line_coverage> {
		ref<counted> load(uint32_t magic,
		                  uint32_t version,
		                  read_stream& in,
		                  std::error_code& ec) const override;
		bool store(ref<counted> const& obj, write_stream& out) const override;
	};
}  // namespace cov::io::handlers
