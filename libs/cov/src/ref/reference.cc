// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cctype>
#include "internal.hh"

namespace cov {
	using namespace std::literals;
	// single-component is only valid, if it is UPPER_CASE; multicomps cannot
	// have: '.' at start of a component; '..' or '@{' anywhere; controls or any
	// of '*:?["^~', SPACE or TAB; a component ends with '.lock'; last (any?)
	// component is empty
	bool reference::is_valid_name(std::string_view name) {
		if (name.empty()) return false;

		size_t prev_component = 0;
		size_t position = 0;
		bool seen_at = false;
		bool seen_dot = false;
		bool seen_slash = false;
		for (auto const c : name) {
			++position;
			if (std::iscntrl(static_cast<unsigned char>(c))) return false;
			switch (c) {
				case '*':
				case ':':
				case '?':
				case '[':
				case '"':
				case '^':
				case '~':
				case ' ':
				case '\t':
					return false;
				case '.':
					if (seen_dot) return false;
					break;
				case '{':
					if (seen_at) return false;
					break;
				case '/':
					if (seen_slash) return false;
					{
						auto const chunk = name.substr(
						    prev_component, position - prev_component - 1);
						prev_component = position;
						if (chunk.front() == '.' || chunk.ends_with(".lock"sv))
							return false;
					}
					break;
			}

			seen_dot = c == '.';
			seen_at = c == '@';
			seen_slash = c == '/';
		}

		if (seen_slash) return false;
		auto const chunk = name.substr(prev_component);
		if (chunk.front() == '.' || chunk.ends_with(".lock"sv)) return false;

		if (!prev_component) {
			for (auto const c : name) {
				if (c == '_') continue;
				if (!std::isupper(static_cast<unsigned char>(c))) return false;
			}
		}

		return true;
	}

	class reference_base : public counted_impl<reference> {
	public:
		reference_base(reference_name&& name) : name_{std::move(name)} {}
		bool references_branch() const noexcept override {
			return name_.tgt_kind == ref_tgt::branch;
		}
		bool references_tag() const noexcept override {
			return name_.tgt_kind == ref_tgt::tag;
		}
		std::string_view name() const noexcept override { return name_.name; };
		std::string_view shorthand() const noexcept override {
			return std::string_view{name_.name}.substr(name_.shorthand_prefix);
		}
		std::string_view symbolic_target() const noexcept override {
			return {};
		}
		git_oid const* direct_target() const noexcept override {
			return nullptr;
		}

	private:
		reference_name name_;
	};

	class direct_reference
	    : public reference_base,
	      public enable_ref_from_this<reference, direct_reference> {
	public:
		direct_reference(reference_name&& name, git_oid const& target)
		    : reference_base{std::move(name)}, target_{target} {}
		cov::reference_type reference_type() const noexcept override {
			return cov::reference_type::direct;
		}
		git_oid const* direct_target() const noexcept override {
			return &target_;
		}
		ref_ptr<reference> peel_target() noexcept override {
			return ref_from_this();
		}

	private:
		git_oid target_;
	};

	ref_ptr<reference> reference::direct(reference_name&& name,
	                                     git_oid const& target) {
		return make_ref<direct_reference>(std::move(name), target);
	}

	class symbolic_reference
	    : public reference_base,
	      public enable_ref_from_this<reference, symbolic_reference> {
	public:
		symbolic_reference(reference_name&& name,
		                   std::string const& target,
		                   ref_ptr<references> const& peel_source)
		    : reference_base{std::move(name)}
		    , target_{target}
		    , peel_source_{peel_source} {}
		cov::reference_type reference_type() const noexcept override {
			return cov::reference_type::symbolic;
		}
		std::string_view symbolic_target() const noexcept override {
			return target_;
		}
		ref_ptr<reference> peel_target() noexcept override {
			auto bead = ref_from_this();
			while (bead &&
			       bead->reference_type() == cov::reference_type::symbolic) {
				auto name = bead->symbolic_target();
				auto next = peel_source_->lookup(name);
				if (!next) {
					next = reference::null(references::prefix_info(name));
				}

				bead = std::move(next);
			}
			return bead;
		}

	private:
		std::string target_;
		ref_ptr<references> peel_source_;
	};

	ref_ptr<reference> reference::symbolic(
	    reference_name&& name,
	    std::string const& target,
	    ref_ptr<references> const& peel_source) {
		return make_ref<symbolic_reference>(std::move(name), target,
		                                    peel_source);
	}

	class null_reference
	    : public reference_base,
	      public enable_ref_from_this<reference, null_reference> {
	public:
		using reference_base::reference_base;

		cov::reference_type reference_type() const noexcept override {
			return cov::reference_type::undetermined;
		}
		ref_ptr<reference> peel_target() noexcept override {
			return ref_from_this();
		}
	};

	ref_ptr<reference> reference::null(reference_name&& name) {
		return make_ref<null_reference>(std::move(name));
	}

}  // namespace cov
