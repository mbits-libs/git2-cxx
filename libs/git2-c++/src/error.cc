// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <git2/errors.h>
#include <git2/error.hh>
#include <string>

namespace git {
	namespace {
		class git_category : public std::error_category {
		public:
			const char* name() const noexcept override {
				return "git::category";
			}

			std::string message(int value) const override {
				switch (value) {
					case GIT_ERROR:
						return "Generic error";
					case GIT_ENOTFOUND:
						return "Requested object could not be found";
					case GIT_EEXISTS:
						return "Object exists preventing operation";
					case GIT_EAMBIGUOUS:
						return "More than one object matches";
					case GIT_EBUFS:
						return "Output buffer too short to hold data";
					case GIT_EUSER:
						return "User-produced error";
					case GIT_EBAREREPO:
						return "Operation not allowed on bare repository";
					case GIT_EUNBORNBRANCH:
						return "HEAD refers to branch with no commits";
					case GIT_EUNMERGED:
						return "Merge in progress prevented operation";
					case GIT_ENONFASTFORWARD:
						return "Reference was not fast-forwardable";
					case GIT_EINVALIDSPEC:
						return "Name/ref spec was not in a valid format";
					case GIT_ECONFLICT:
						return "Checkout conflicts prevented operation";
					case GIT_ELOCKED:
						return "Lock file prevented operation";
					case GIT_EMODIFIED:
						return "Reference value does not match expected";
					case GIT_EAUTH:
						return "Authentication error";
					case GIT_ECERTIFICATE:
						return "Server certificate is invalid";
					case GIT_EAPPLIED:
						return "Patch/merge has already been applied";
					case GIT_EPEEL:
						return "The requested peel operation is not possible";
					case GIT_EEOF:
						return "Unexpected EOF";
					case GIT_EINVALID:
						return "Invalid operation or input";
					case GIT_EUNCOMMITTED:
						return "Uncommitted changes in index prevented "
						       "operation";
					case GIT_EDIRECTORY:
						return "The operation is not valid for a directory";
					case GIT_EMERGECONFLICT:
						return "A merge conflict exists and cannot continue";
					case GIT_PASSTHROUGH:
						return "A user-configured callback refused to act";
					case GIT_ITEROVER:
						return "Signals end of iteration with iterator";
					case GIT_RETRY:
						return "Internal error escaped from library";
					case GIT_EMISMATCH:
						return "Hashsum mismatch in object";
					case GIT_EINDEXDIRTY:
						return "Unsaved changes in the index would be "
						       "overwritten";
					case GIT_EAPPLYFAIL:
						return "Patch application failed";
				}
				return "git::errc{" + std::to_string(value) + "}";
			}

			bool equivalent(int err, const std::error_condition& cond)
			    const noexcept override {
				return cond ==
				       git::make_error_condition(static_cast<errc>(err));
			}
		};
	}  // namespace

	std::error_category const& category() {
		static git_category cat;
		return cat;
	}
}  // namespace git
