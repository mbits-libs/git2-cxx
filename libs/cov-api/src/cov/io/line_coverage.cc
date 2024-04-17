// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/io/line_coverage.hh>
#include <cov/io/types.hh>

namespace cov::io::handlers {
	namespace {
		struct impl : counted_impl<cov::line_coverage> {
			explicit impl(std::vector<v1::coverage>&& lines)
			    : lines_{std::move(lines)} {}

			std::span<io::v1::coverage const> coverage()
			    const noexcept override {
				return lines_;
			}

		private:
			std::vector<v1::coverage> lines_{};
		};
	}  // namespace

	ref_ptr<counted> line_coverage::load(uint32_t,
	                                     uint32_t,
	                                     git::oid_view,
	                                     read_stream& in,
	                                     std::error_code& ec) const {
		ec = make_error_code(errc::bad_syntax);
		uint32_t count{0};
		if (!in.load(count)) return {};
		std::vector<v1::coverage> result{};
		if (!in.load(result, count)) return {};
		ec.clear();
		return cov::line_coverage::create(std::move(result));
	}

	bool line_coverage::store(ref_ptr<counted> const& value,
	                          write_stream& out) const {
		auto const obj =
		    as_a<cov::line_coverage>(static_cast<object const*>(value.get()));
		if (!obj) return false;
		auto const& items = obj->coverage();
		v1::line_coverage hdr{.line_count =
		                          static_cast<uint32_t>(items.size())};
		if (!out.store(hdr)) return false;
		if (!out.store(items)) return false;
		return true;
	}
}  // namespace cov::io::handlers

namespace cov {
	ref_ptr<line_coverage> line_coverage::create(
	    std::vector<io::v1::coverage>&& lines) {
		return make_ref<io::handlers::impl>(std::move(lines));
	}
}  // namespace cov
