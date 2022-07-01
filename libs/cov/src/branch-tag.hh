// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/reference.hh>
#include <string_view>

namespace cov {
	namespace detail {
		static std::string full_name(std::string_view prefix,
		                             std::string_view name) {
			std::string result;
			result.reserve(prefix.size() + name.size());
			result.append(prefix);
			result.append(name);
			return result;
		}

		template <typename Policy>
		struct link : counted_impl<typename Policy::type> {
			using type = typename Policy::type;
			link(std::string const& name, git_oid const& id)
			    : name_{name}, id_{id} {}
			std::string_view name() const noexcept override { return name_; }
			git_oid const* id() const noexcept override { return &id_; }

			static std::string full_name(std::string_view name) {
				return detail::full_name(Policy::prefix(), name);
			}

			static bool is_valid_name(std::string_view name) {
				return reference::is_valid_name(full_name(name));
			}

			static ref_ptr<type> create(std::string_view name,
			                            git_oid const& id,
			                            references& refs) {
				return from(refs.create(full_name(name), id));
			}

			static ref_ptr<type> lookup(std::string_view name,
			                            references& refs) {
				return from(refs.lookup(full_name(name)));
			}

			static ref_ptr<type> from(ref_ptr<reference>&& ref) {
				std::string name{};
				if (ref) name.assign(ref->shorthand());
				if (ref) ref = ref->peel_target();
				if (!ref || ref->reference_type() != reference_type::direct)
					return {};
				return make_ref<typename Policy::type_impl>(
				    name, *ref->direct_target());
			}

			static ref_ptr<typename Policy::list_type> iterator(
			    references& refs) {
				return make_ref<typename Policy::list_type_impl>(refs);
			}

		private:
			std::string name_;
			git_oid id_;
		};

		template <typename Policy>
		struct link_list : counted_impl<typename Policy::list_type> {
			using type = typename Policy::type;
			link_list(references& refs)
			    : list_{refs.iterator(Policy::subdir())} {}
			ref_ptr<type> next() override {
				while (true) {
					auto ref = list_->next();
					if (!ref) return {};
					if (!Policy::right_reference(ref)) continue;
					return type::from(std::move(ref));
				}
			}

		private:
			ref_ptr<reference_list> list_;
		};

	}  // namespace detail
}  // namespace cov