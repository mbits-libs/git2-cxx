// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/io/build.hh>
#include <cov/io/strings.hh>
#include <cov/io/types.hh>

namespace cov::io::handlers {
	namespace {
		struct impl : counted_impl<cov::build> {
			explicit impl(git::oid_view oid,
			              git::oid_view file_list,
			              sys_seconds add_time_utc,
			              std::string_view props_json,
			              io::v1::coverage_stats const& stats)
			    : oid_{oid.oid()}
			    , file_list_{file_list.oid()}
			    , add_time_utc_{add_time_utc}
			    , props_json_{cov::report::builder::normalize(props_json)}
			    , stats_{stats} {}

			git::oid const& oid() const noexcept override { return oid_; }
			git::oid const& file_list_id() const noexcept override {
				return file_list_;
			}
			sys_seconds add_time_utc() const noexcept override {
				return add_time_utc_;
			}
			std::string_view props_json() const noexcept override {
				return props_json_;
			}
			io::v1::coverage_stats const& stats() const noexcept override {
				return stats_;
			}

		private:
			git::oid oid_;
			git::oid file_list_;
			sys_seconds add_time_utc_;
			std::string props_json_;
			io::v1::coverage_stats stats_;
		};
	}  // namespace

	ref_ptr<counted> build::load(uint32_t,
	                             uint32_t,
	                             git::oid_view id,
	                             read_stream& in,
	                             std::error_code& ec) const {
		ec = make_error_code(errc::bad_syntax);
		v1::build build{};
		if (!in.load(build)) {
			return {};
		}
		if (build.strings.offset < (sizeof(build) / sizeof(uint32_t))) {
			return {};
		}

		if (!in.skip((build.strings.offset * sizeof(uint32_t)) -
		             sizeof(build))) {
			return {};
		}
		strings_view strings{};
		if (!strings.load_from(in, build.strings)) {
			return {};
		}

		if (!strings.is_valid(build.propset)) {
			return {};
		}
		auto propset = strings.at(build.propset);

		ec.clear();
		return cov::build::create(id, build.file_list, build.added.to_seconds(),
		                          propset, build.stats);
	}

#if defined(__GNUC__)
// The warning is legit, since as_a<> can return nullptr, if there is no
// cov::build in type tree branch, but this should be called from within
// db_object::store, which is guarded by report::recognized
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#endif
	bool build::store(ref_ptr<counted> const& value, write_stream& out) const {
		auto const obj =
		    as_a<cov::build>(static_cast<object const*>(value.get()));
		auto stg = [obj] {
			strings_builder strings{};
			strings.insert(obj->props_json());
			return strings.build();
		}();

		auto const locate = [&, size = stg.size()](std::string_view value) {
			auto const offset = stg.locate_or(value, size + 1);
			auto const offset32 = static_cast<uint32_t>(offset);
			if (offset != offset32) throw false;
			return static_cast<io::str>(offset32);
		};

		auto const time_stamp = [](sys_seconds time) {
			timestamp ts{};
			ts = time;
			return ts;
		};

		v1::build report{
		    .strings = stg.after<v1::build>(),
		    .file_list = obj->file_list_id().id,
		    .added = time_stamp(obj->add_time_utc()),
		    .propset = locate(obj->props_json()),
		    .stats = obj->stats(),
		};

		if (!out.store(report)) return false;
		if (!out.store({stg.data(), stg.size()})) return false;

		return true;
	}
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
}  // namespace cov::io::handlers

namespace cov {
	ref_ptr<build> build::create(git::oid_view oid,
	                             git::oid_view file_list,
	                             sys_seconds add_time_utc,
	                             std::string_view props_json,
	                             io::v1::coverage_stats const& stats) {
		return make_ref<io::handlers::impl>(oid, file_list, add_time_utc,
		                                    props_json, stats);
	}
}  // namespace cov
