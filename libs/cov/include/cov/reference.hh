// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <git2/oid.h>
#include <cov/object.hh>
#include <filesystem>
#include <string_view>

namespace cov {
	enum class reference_type : bool { direct = true, symbolic = false };

	struct reference : object {
		obj_type type() const noexcept override { return obj_reference; };
		bool is_reference() const noexcept final { return true; }
		virtual cov::reference_type reference_type() const noexcept = 0;
		virtual bool references_branch() const noexcept = 0;
		virtual bool references_tag() const noexcept = 0;
		static bool is_valid_name(std::string_view);
		virtual std::string_view name() const noexcept = 0;
		virtual std::string_view shorthand() const noexcept = 0;
		virtual std::string_view symbolic_target() const noexcept = 0;
		virtual git_oid const* direct_target() const noexcept = 0;
		virtual ref_ptr<reference> peel_target() noexcept = 0;
	};

	template <typename ListType, typename ItemType>
	class iterator_t {
	public:
		using difference_type = void;
		using value_type = void;
		using pointer = void;
		using reference = void;
		using iterator_category = std::input_iterator_tag;

		iterator_t(ListType* parent = nullptr) : parent_{parent} {
			if (parent) parent->acquire();
			next();
		}

		bool operator==(iterator_t const&) const = default;
		iterator_t& operator++() {
			next();
			return *this;
		}

		ref_ptr<ItemType> operator*() noexcept { return current_; }

	private:
		void next() {
			if (!parent_) return;
			current_ = parent_->next();
			if (!current_) parent_.reset();
		}

		ref_ptr<ListType> parent_;
		ref_ptr<ItemType> current_;
	};

	struct reference_list : object {
		obj_type type() const noexcept override { return obj_reference_list; };
		bool is_reference_list() const noexcept final { return true; }
		virtual ref_ptr<reference> next() = 0;

		using iter_t = iterator_t<reference_list, reference>;
		iter_t begin() { return {this}; }
		iter_t end() { return {}; }
	};

	struct references : object {
		static ref_ptr<references> make_refs(std::filesystem::path const& root);
		obj_type type() const noexcept override { return obj_references; };
		bool is_references() const noexcept final { return true; }
		virtual ref_ptr<reference> create(std::string_view name,
		                                  git_oid const& target) = 0;
		virtual ref_ptr<reference> create(std::string_view name,
		                                  std::string_view target) = 0;
		virtual ref_ptr<reference> create_matching(std::string_view name,
		                                           git_oid const& target,
		                                           git_oid const& expected,
		                                           bool& modified) = 0;
		virtual ref_ptr<reference> create_matching(std::string_view name,
		                                           std::string_view target,
		                                           std::string_view expected,
		                                           bool& modified) = 0;
		virtual ref_ptr<reference> dwim(std::string_view) = 0;
		virtual ref_ptr<reference> lookup(std::string_view) = 0;
		virtual ref_ptr<reference_list> iterator() = 0;
		virtual ref_ptr<reference_list> iterator(std::string_view prefix) = 0;
	};
}  // namespace cov
