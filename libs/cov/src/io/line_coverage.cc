// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/io/line_coverage.hh>
#include <cov/io/types.hh>

namespace cov::io::handlers {
	namespace {
		struct impl : counted_impl<cov::line_coverage> {
			explicit impl(std::vector<v1::coverage>&& lines)
			    : lines_{std::move(lines)} {}

			std::vector<v1::coverage> const& coverage()
			    const noexcept override {
				return lines_;
			}

		private:
			std::vector<v1::coverage> lines_{};
		};
	}  // namespace

	ref_ptr<counted> line_coverage::load(uint32_t,
	                                     uint32_t,
	                                     git_oid const&,
	                                     read_stream& in,
	                                     std::error_code& ec) const {
		ec = make_error_code(errc::bad_syntax);
		uint32_t count{0};
		if (!in.load(count)) return {};
		std::vector<v1::coverage> result{};
		if (!in.load(result, count)) return {};
		ec.clear();
		return line_coverage_create(std::move(result));
	}

#if defined(__GNUC__)
// The warning is legit, since as_a<> can return nullptr, if there is no
// cov::line_coverage in type tree branch, but this should be called from within
// db_object::store, which is guarded by line_coverage::recognized
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#endif
	bool line_coverage::store(ref_ptr<counted> const& value,
	                          write_stream& out) const {
		auto const obj =
		    as_a<cov::line_coverage>(static_cast<object const*>(value.get()));
		auto const& items = obj->coverage();
		v1::line_coverage hdr{.line_count =
		                          static_cast<uint32_t>(items.size())};
		if (!out.store(hdr)) return false;
		if (!out.store(items)) return false;
		return true;
	}
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
}  // namespace cov::io::handlers

namespace cov {
	ref_ptr<line_coverage> line_coverage_create(
	    std::vector<io::v1::coverage>&& lines) {
		return make_ref<io::handlers::impl>(std::move(lines));
	}
}  // namespace cov
