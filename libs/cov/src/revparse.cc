// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <git2/errors.h>
#include <cov/report.hh>
#include <cov/repository.hh>
#include <cov/revparse.hh>
#include <cstring>
#include <git2/error.hh>
#include <unordered_set>

namespace cov {
	namespace {
		using namespace std::literals;

		struct ref {
			git::oid sha{};
			bool flag{};

			explicit operator bool() const noexcept { return flag; }

			bool operator==(ref const& other) const noexcept {
				if (flag != other.flag) return false;
				if (!flag) return true;
				return sha == other.sha;
			}

			size_t hash() const noexcept {
				auto const view =
				    std::string_view{reinterpret_cast<char const*>(sha.id.id),
				                     sizeof(sha.id.id)};
				return std::hash<std::string_view>{}(view);
			}

			ref next(cov::repository const& repo) const {
				if (flag) {
					std::error_code ec{};
					// fs based, good candidate for WHY I will need cache of
					// object-already-looked-up
					auto me = repo.lookup<cov::report>(sha, ec);
					if (!ec && me) {
						auto const& parent = me->parent_id();
						if (!parent.is_zero()) {
							return {parent, true};
						}
					}
				}
				return {};
			}
		};

		struct ref_hash {
			size_t operator()(ref const& value) const noexcept {
				return value.hash();
			}
		};

		using ref_set = std::unordered_set<ref, ref_hash>;

		template <typename Iterator>
		static size_t get_number(Iterator& it, Iterator end) {
			if (it == end) return 1;
			if (!std::isdigit(static_cast<unsigned char>(*it))) return 1;
			size_t result{};
			if (it != end && std::isdigit(static_cast<unsigned char>(*it))) {
				result *= 10;
				result += static_cast<size_t>(*it++ - '0');
			}
			return result;
		}
	}  // namespace

	/*
	    A                   K
	     \                 /
	      B       I       L
	       \     /       /
	        C   J       M
	         \ /       /
	          D       N
	           \     /
	            E   O
	             \ /
	              F
	              |
	              G
	              |
	              H

	    locate_range(0..any) -> 0..any
	    locate_range(any..0) -> 0..0
	    locate_range(G..K) -> G..K
	        K -> L -> M -> N -> O -> F -> G!
	        G -> H -> zero
	    locate_range(K..G) -> G..G
	        G -> H -> zero
	        K -> L -> M -> N -> O -> F -> G!
	    locate_range(B..K) -> F..K
	        K -> L -> M -> N -> O -> F!
	        B -> C -> D -> E -> F
	*/
	void revs::locate_range(cov::repository const& repo) {
		if (git_oid_is_zero(&from) || git_oid_is_zero(&to)) {
			return;
		}

		ref_set allowed{};
		ref_set unallowed{};

		// from is inaccessible start
		// to is accessible start

		ref top{git::oid{to}, true};
		ref bottom{git::oid{from}, true};

		while (top || bottom) {
			if (top) {
				if (unallowed.contains(top)) {
					from = top.sha.id;
					return;
				}
				allowed.insert(top);
				top = top.next(repo);
			}
			if (bottom) {
				if (allowed.contains(bottom)) {
					from = bottom.sha.id;
					return;
				}
				unallowed.insert(bottom);
				bottom = bottom.next(repo);
			}
		}
		// by now, top.id is zero -- so everything will be included
		from = top.sha.id;
	}

	std::error_code revs::parse(cov::repository const& repo,
	                            std::string_view range,
	                            revs& out) {
		out = {};
		auto const pos = range.find(std::string_view{"..", 2});
		if (pos == std::string_view::npos) {
			out.single = true;
			return parse_single(repo, range, out.to);
		}

		auto const from_rev = range.substr(0, pos);
		auto const to_rev = range.substr(pos + 2);

		if (from_rev.empty() && to_rev.empty()) {
			git_error_set(GIT_ERROR_INVALID, "Invalid pattern '..'");
			return git::make_error_code(git::errc::invalidspec);
		}

		if (!to_rev.empty() && to_rev.front() == '.') {
			// ".." was part of "..." and we do not support symmetric difference
			git_error_set(GIT_ERROR_INVALID, "Invalid pattern '...'");
			return git::make_error_code(git::errc::invalidspec);
		}

		git_oid HEAD{};
		if (from_rev.empty() || to_rev.empty()) {
			auto const ec = parse_single(repo, "HEAD", HEAD);
			if (ec) return ec;
		}

		if (from_rev.empty()) {
			out.from = HEAD;
		} else {
			auto const ec = parse_single(repo, from_rev, out.from);
			if (ec) return ec;
		}

		if (to_rev.empty()) {
			out.to = HEAD;
		} else {
			auto const ec = parse_single(repo, to_rev, out.to);
			if (ec) {
				out.from = {};
				return ec;
			}
		}

		out.locate_range(repo);
		return {};
	}

	std::error_code revs::parse_single(cov::repository const& repo,
	                                   std::string_view rev,
	                                   git_oid& out) {
		size_t parent_count = 0;

		auto it = rev.begin();
		auto const end = rev.end();

		while (it != end && *it != '^' && *it != '~')
			++it;

		std::string_view base_rev{rev.begin(), it};

		while (it != end) {
			switch (*it) {
				case '^': {
					++it;
					auto const count = get_number(it, end);
					if (count > 1) {
						// at any point, looking into more, than first parent
						// will jump outside of the tree
						memset(out.id, 0, sizeof(out.id));
						return git::make_error_code(git::errc::notfound);
					}
					parent_count += count;
					break;
				}
				case '~':
					++it;
					parent_count += get_number(it, end);
					break;
				default: {
					git_error_set(GIT_ERROR_INVALID, "Invalid pattern '%c'",
					              *it);
					return git::make_error_code(git::errc::invalidspec);
				}
			}
		}

		auto ref = repo.refs()->dwim(base_rev);
		if (ref) {
			while (ref->reference_type() == reference_type::symbolic) {
				ref = ref->peel_target();
			}

			if (ref->reference_type() != reference_type::direct) {
				memset(out.id, 0, sizeof(out.id));
				auto const HEAD = repo.current_head();
				if ((base_rev == "HEAD"sv || base_rev == HEAD.branch) &&
				    !HEAD.tip)
					return git::make_error_code(git::errc::unbornbranch);
				return git::make_error_code(git::errc::notfound);
			}

			out = *ref->direct_target();
		} else {
			auto report = as_a<cov::report>(repo.find_partial(base_rev));
			if (!report) {
				memset(out.id, 0, sizeof(out.id));
				return git::make_error_code(git::errc::notfound);
			}

			out = report->oid().id;
		}

		if (parent_count) {
			auto node = cov::ref{git::oid{out}, true};
			while (node && parent_count) {
				--parent_count;
				node = node.next(repo);
			}
			out = node.sha.id;
		}

		return {};
	}
}  // namespace cov
