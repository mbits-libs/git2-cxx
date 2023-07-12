// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/io/report.hh>
#include <cov/io/strings.hh>
#include <cov/io/types.hh>

namespace cov::io::handlers {
	namespace {
		std::string S(std::string_view value) {
			return {value.data(), value.size()};
		}

		struct signature {
			std::string name;
			std::string email;
		};

		signature S(cov::report::signature_view value) {
			return {.name = S(value.name), .email = S(value.email)};
		}

		struct impl : counted_impl<cov::report> {
			explicit impl(git_oid const& oid,
			              git_oid const& parent_report,
			              git_oid const& file_list,
			              git_oid const& commit,
			              std::string_view branch,
			              cov::report::signature_view author,
			              cov::report::signature_view committer,
			              std::string_view message,
			              sys_seconds commit_time_utc,
			              sys_seconds add_time_utc,
			              io::v1::coverage_stats const& stats)
			    : oid_{oid}
			    , parent_report_{parent_report}
			    , file_list_{file_list}
			    , commit_{commit}
			    , branch_{S(branch)}
			    , author_{S(author)}
			    , committer_{S(committer)}
			    , message_{S(message)}
			    , commit_time_utc_{commit_time_utc}
			    , add_time_utc_{add_time_utc}
			    , stats_{stats} {}

			git_oid const& oid() const noexcept override { return oid_; }
			git_oid const& parent_report() const noexcept override {
				return parent_report_;
			}
			git_oid const& file_list() const noexcept override {
				return file_list_;
			}
			git_oid const& commit() const noexcept override { return commit_; }
			std::string_view branch() const noexcept override {
				return branch_;
			}
			std::string_view author_name() const noexcept override {
				return author_.name;
			}
			std::string_view author_email() const noexcept override {
				return author_.email;
			}
			std::string_view committer_name() const noexcept override {
				return committer_.name;
			}
			std::string_view committer_email() const noexcept override {
				return committer_.email;
			}
			std::string_view message() const noexcept override {
				return message_;
			}
			sys_seconds commit_time_utc() const noexcept override {
				return commit_time_utc_;
			}
			sys_seconds add_time_utc() const noexcept override {
				return add_time_utc_;
			}
			io::v1::coverage_stats const& stats() const noexcept override {
				return stats_;
			}

		private:
			git_oid oid_;
			git_oid parent_report_;
			git_oid file_list_;
			git_oid commit_;
			std::string branch_;
			signature author_;
			signature committer_;
			std::string message_;
			sys_seconds commit_time_utc_;
			sys_seconds add_time_utc_;
			io::v1::coverage_stats stats_;
		};
	}  // namespace

	ref_ptr<counted> report::load(uint32_t,
	                              uint32_t,
	                              git_oid const& id,
	                              read_stream& in,
	                              std::error_code& ec) const {
		ec = make_error_code(errc::bad_syntax);
		v1::report header{};
		if (!in.load(header)) return {};

		if (!in.skip(header.strings_offset * sizeof(uint32_t))) return {};
		strings_view strings{};
		if (!in.load(strings.data, header.strings_size * sizeof(uint32_t)))
			return {};
		strings.resync();

		if (!strings.is_valid(header.commit.branch) ||
		    !strings.is_valid(header.commit.author.name) ||
		    !strings.is_valid(header.commit.author.email) ||
		    !strings.is_valid(header.commit.committer.name) ||
		    !strings.is_valid(header.commit.committer.email) ||
		    !strings.is_valid(header.commit.message))
			return {};

		auto const at = [&](size_t offset) { return strings.at(offset); };

		auto branch = at(header.commit.branch),
		     author_name = at(header.commit.author.name),
		     author_email = at(header.commit.author.email),
		     committer_name = at(header.commit.committer.name),
		     committer_email = at(header.commit.committer.email),
		     message = at(header.commit.message);

		ec.clear();
		return cov::report::create(
		    id, header.parent_report, header.file_list, header.commit.commit_id,
		    branch, {author_name, author_email},
		    {committer_name, committer_email}, message,
		    header.commit.committed.to_seconds(), header.added.to_seconds(),
		    v1::coverage_stats::from(header.stats));
	}

#if defined(__GNUC__)
// The warning is legit, since as_a<> can return nullptr, if there is no
// cov::report in type tree branch, but this should be called from within
// db_object::store, which is guarded by report::recognized
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#endif
	bool report::store(ref_ptr<counted> const& value, write_stream& out) const {
		auto const obj =
		    as_a<cov::report>(static_cast<object const*>(value.get()));
		auto stg = [obj] {
			strings_builder strings{};
			strings.insert(obj->branch());
			strings.insert(obj->author_name());
			strings.insert(obj->author_email());
			strings.insert(obj->committer_name());
			strings.insert(obj->committer_email());
			strings.insert(obj->message());
			return strings.build();
		}();

		auto const locate = [&, size = stg.size()](std::string_view value) {
			auto const offset = stg.locate_or(value, size + 1);
			auto const offset32 = static_cast<uint32_t>(offset);
			if (offset != offset32) throw false;
			return offset32;
		};

		auto const time_stamp = [](sys_seconds time) {
			timestamp ts{};
			ts = time;
			return ts;
		};

		v1::report hdr{
		    .parent_report = obj->parent_report(),
		    .file_list = obj->file_list(),
		    .added = time_stamp(obj->add_time_utc()),
		    .stats = obj->stats().to_short(),
		    .commit =
		        {
		            .branch = locate(obj->branch()),
		            .author = {.name = locate(obj->author_name()),
		                       .email = locate(obj->author_email())},
		            .committer = {.name = locate(obj->committer_name()),
		                          .email = locate(obj->committer_email())},
		            .message = locate(obj->message()),
		            .commit_id = obj->commit(),
		            .committed = time_stamp(obj->commit_time_utc()),
		        },
		    .strings_offset = 0u,
		    .strings_size =
		        static_cast<uint32_t>(stg.size() / sizeof(uint32_t)),
		};

		if (!out.store(hdr)) return false;
		if (!out.store({stg.data(), stg.size()})) return false;

		return true;
	}
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
}  // namespace cov::io::handlers

namespace cov {
	ref_ptr<report> report::create(git_oid const& oid,
	                               git_oid const& parent_report,
	                               git_oid const& file_list,
	                               git_oid const& commit,
	                               std::string_view branch,
	                               signature_view author,
	                               signature_view committer,
	                               std::string_view message,
	                               sys_seconds commit_time_utc,
	                               sys_seconds add_time_utc,
	                               io::v1::coverage_stats const& stats) {
		return make_ref<io::handlers::impl>(
		    oid, parent_report, file_list, commit, branch, author, committer,
		    message, commit_time_utc, add_time_utc, stats);
	}
}  // namespace cov
