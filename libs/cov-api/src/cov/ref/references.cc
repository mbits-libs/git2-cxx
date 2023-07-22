// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <cov/io/file.hh>
#include <cov/io/safe_stream.hh>
#include "../path-utils.hh"
#include "internal.hh"

namespace cov {
	namespace {
		static constexpr auto heads = "refs/heads"sv;
		static constexpr auto tags = "refs/tags"sv;

		std::string name_for(std::string_view name, bool as_branch) {
			return fmt::format("{}/{}", as_branch ? heads : tags, name);
		}
	}  // namespace

	class references_impl
	    : public counted_impl<references>,
	      public enable_ref_from_this<references, references_impl> {
	public:
		references_impl(std::filesystem::path const& root) : root_{root} {}

		ref_ptr<reference> create(std::string_view name,
		                          git::oid_view target) override {
			if (!reference::is_valid_name(name)) return {};
			if (!print(name, target.str())) return {};
			return reference::direct(prefix_info(name), target);
		}

		ref_ptr<reference> create(std::string_view name,
		                          std::string_view target) override {
			if (!reference::is_valid_name(name)) return {};

			static constexpr auto REF = "ref: "sv;

			std::string contents{};
			contents.reserve(REF.size() + target.size());
			contents.append(REF);
			contents.append(target);

			if (!print(name, contents)) return {};
			auto self = ref_from_this();
			return reference::symbolic(prefix_info(name),
			                           {target.data(), target.size()}, self);
		}

		ref_ptr<reference> create_matching(std::string_view name,
		                                   git::oid_view target,
		                                   git::oid_view expected,
		                                   bool& modified) override {
			modified = false;
			auto prev = lookup(name);
			if (prev) {
				if (prev->reference_type() != cov::reference_type::direct ||
				    *prev->direct_target() != expected) {
					modified = true;
					return {};
				}
			}
			return create(name, target);
		}

		ref_ptr<reference> create_matching(std::string_view name,
		                                   std::string_view target,
		                                   std::string_view expected,
		                                   bool& modified) override {
			modified = false;
			auto prev = lookup(name);
			if (prev) {
				if (prev->reference_type() != cov::reference_type::symbolic ||
				    prev->symbolic_target() != expected) {
					modified = true;
					return {};
				}
			}
			return create(name, target);
		}

		ref_ptr<reference> dwim(std::string_view shorthand) override {
			static constexpr std::string_view prefixes[] = {
			    ""sv,
			    names::refs_dir_prefix,
			    names::tags_dir_prefix,
			    names::heads_dir_prefix,
			};

			for (auto prefix : prefixes) {
				std::string full_name{};
				full_name.reserve(prefix.size() + shorthand.size());
				full_name.append(prefix);
				full_name.append(shorthand);

				auto candidate = lookup(full_name);
				if (candidate &&
				    candidate->reference_type() != reference_type::undetermined)
					return candidate;
			}

			return {};
		}

		ref_ptr<reference> lookup(std::string_view name) override {
			if (!reference::is_valid_name(name)) return {};

			auto in = io::fopen(root_ / make_path(name));
			if (!in) return {};

			auto line = in.read_line();

			std::string_view view{line};
			auto symlink = prefixed(names::ref_prefix, view);
			if (symlink) {
				auto self = ref_from_this();
				return reference::symbolic(prefix_info(name),
				                           {symlink->data(), symlink->size()},
				                           self);
			}

			auto const onlyhex = [](std::string_view oid) {
				for (auto c : oid) {
					if (!std::isxdigit(static_cast<unsigned char>(c)))
						return false;
				}
				return true;
			};

			view = strip(view);
			if (view.length() == GIT_OID_HEXSZ && onlyhex(view)) {
				auto self = ref_from_this();  // TODO: ?

				return reference::direct(prefix_info(name),
				                         git::oid::from(view));
			}

			return {};
		}

		ref_ptr<reference_list> iterator() override {
			return iterator(names::refs_dir);
		}

		ref_ptr<reference_list> iterator(std::string_view prefix) override {
			return reference_list::create(root_, {prefix.data(), prefix.size()},
			                              ref_from_this());
		}

		std::error_code remove_ref(ref_ptr<reference> const& ref) override {
			auto const full_name = ref->name();
			if (full_name == names::HEAD) {
				// never HEAD!
				return git::make_error_code(git::errc::error);
			}

			if (!reference::is_valid_name(full_name)) {
				// clean an invalid name, if it exists...
				std::error_code ec{};
				std::filesystem::remove(root_ / make_path(full_name), ec);
				if (ec) return ec;
				return git::make_error_code(git::errc::invalidspec);
			}

			auto const curr = lookup(full_name);

			if (!curr) {
				return git::make_error_code(git::errc::notfound);
			}

			auto const type = ref->reference_type();
			if (curr->reference_type() != type) {
				return git::make_error_code(git::errc::modified);
			}

			if (type == reference_type::direct &&
			    *curr->direct_target() != *ref->direct_target()) {
				return git::make_error_code(git::errc::modified);
			}

			if (type == reference_type::symbolic &&
			    curr->symbolic_target() != ref->symbolic_target()) {
				return git::make_error_code(git::errc::modified);
			}

			std::error_code ec{};
			std::filesystem::remove(root_ / make_path(full_name), ec);
			return ec;
		}

		inline ref_ptr<reference> error(git::errc code, std::error_code& ec) {
			ec = git::make_error_code(code);
			return {};
		}

		ref_ptr<reference> copy_ref(ref_ptr<reference> const& ref,
		                            std::string_view new_name,
		                            bool as_branch,
		                            bool force,
		                            std::error_code& ec) override {
			ec.clear();
			auto const full_name = name_for(new_name, as_branch);
			if (!force && lookup(full_name))
				return error(git::errc::exists, ec);

			auto peeled = ref->peel_target();
			if (!peeled->direct_target())
				return error(git::errc::unbornbranch, ec);

			auto result = create(full_name, *peeled->direct_target());
			if (!result) return error(git::errc::invalidspec, ec);

			return result;
		}

	private:
		bool print(std::string_view name, std::string const& line) {
			auto filename = root_ / make_path(name);

			std::error_code ec{};
			std::filesystem::create_directories(filename.parent_path(), ec);
			if (ec) return false;
			auto out = io::safe_stream{filename};
			if (!out.opened()) return false;
			auto endl = '\n';
			auto result = out.store({line.data(), line.size()});
			if (result) result = out.store({&endl, 1});
			if (result) result = out.commit() == std::error_code{};
			if (!result) {
				out.rollback();  // GCOV_EXCL_LINE[GCC, Clang] -- untestable
			}                    // GCOV_EXCL_LINE[Clang]
			return result;
		}
		std::filesystem::path root_;
	};

	ref_ptr<references> references::make_refs(
	    std::filesystem::path const& root) {
		return make_ref<references_impl>(root);
	}

	static std::string S(std::string_view value) {
		return {value.data(), value.size()};
	}

	reference_name references::prefix_info(std::string_view name) {
		if (name.starts_with(names::heads_dir_prefix))
			return {ref_tgt::branch, S(name), names::heads_dir_prefix.size()};
		if (name.starts_with(names::tags_dir_prefix))
			return {ref_tgt::tag, S(name), names::tags_dir_prefix.size()};
		if (name.starts_with(names::refs_dir_prefix))
			return {ref_tgt::other, S(name), names::refs_dir_prefix.size()};
		return {ref_tgt::other, S(name), 0u};
	}
}  // namespace cov
