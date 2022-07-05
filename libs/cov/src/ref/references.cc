// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/io/file.hh>
#include <cov/io/safe_stream.hh>
#include "../path-utils.hh"
#include "internal.hh"

namespace cov {
	class references_impl
	    : public counted_impl<references>,
	      public enable_ref_from_this<references, references_impl> {
	public:
		references_impl(std::filesystem::path const& root) : root_{root} {}

		ref_ptr<reference> create(std::string_view name,
		                          git_oid const& target) override {
			if (!reference::is_valid_name(name)) return {};

			char buffer[GIT_OID_HEXSZ];
			git_oid_fmt(buffer, &target);

			if (!print(name, {buffer, GIT_OID_HEXSZ})) return {};
			auto const [kind, prefix_length] = prefix_info(name);
			return direct_reference_create(kind, {name.data(), name.size()},
			                               prefix_length, target);
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
			auto const [kind, prefix_length] = prefix_info(name);
			auto self = ref_from_this();
			return symbolic_reference_create(
			    kind, {name.data(), name.size()}, prefix_length,
			    {target.data(), target.size()}, self);
		}

		ref_ptr<reference> create_matching(std::string_view name,
		                                   git_oid const& target,
		                                   git_oid const& expected,
		                                   bool& modified) override {
			modified = false;
			auto prev = lookup(name);
			if (prev) {
				if (prev->reference_type() != cov::reference_type::direct ||
				    git_oid_cmp(prev->direct_target(), &expected) != 0) {
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
				if (candidate) return candidate;
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
				auto const [kind, prefix_length] = prefix_info(name);
				auto self = ref_from_this();
				return symbolic_reference_create(
				    kind, {name.data(), name.size()}, prefix_length,
				    {symlink->data(), symlink->size()}, self);
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
				auto const [kind, prefix_length] = prefix_info(name);
				auto self = ref_from_this();

				git_oid target{};
				if (git_oid_fromstr(&target, view.data())) return {};

				return direct_reference_create(kind, {name.data(), name.size()},
				                               prefix_length, target);
			}

			return {};
		}

		ref_ptr<reference_list> iterator() override {
			return iterator(names::refs_dir);
		}

		ref_ptr<reference_list> iterator(std::string_view prefix) override {
			return reference_list_create(root_, {prefix.data(), prefix.size()},
			                             ref_from_this());
		}

	private:
		static std::pair<ref_tgt, size_t> prefix_info(std::string_view name) {
			if (name.starts_with(names::heads_dir_prefix))
				return {ref_tgt::branch, names::heads_dir_prefix.size()};
			if (name.starts_with(names::tags_dir_prefix))
				return {ref_tgt::tag, names::tags_dir_prefix.size()};
			if (name.starts_with(names::refs_dir_prefix))
				return {ref_tgt::other, names::refs_dir_prefix.size()};
			return {ref_tgt::other, 0u};
		}

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
			if (result)
				out.commit();
			else
				out.rollback();  // GCOV_EXCL_LINE -- untestable
			return result;
		}
		std::filesystem::path root_;
	};

	ref_ptr<references> references::make_refs(
	    std::filesystem::path const& root) {
		return make_ref<references_impl>(root);
	}
}  // namespace cov
