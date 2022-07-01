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
		reference_base(ref_tgt tgt_kind,
		               std::string const& name,
		               size_t shorthand_prefix)
		    : tgt_kind_{tgt_kind}
		    , name_{name}
		    , shorthand_prefix_{shorthand_prefix} {}
		bool references_branch() const noexcept override {
			return tgt_kind_ == ref_tgt::branch;
		}
		bool references_tag() const noexcept override {
			return tgt_kind_ == ref_tgt::tag;
		}
		std::string_view name() const noexcept override { return name_; };
		std::string_view shorthand() const noexcept override {
			return std::string_view{name_}.substr(shorthand_prefix_);
		}
		std::string_view symbolic_target() const noexcept override {
			return {};
		}
		git_oid const* direct_target() const noexcept override {
			return nullptr;
		}

	private:
		ref_tgt tgt_kind_;
		std::string name_;
		size_t shorthand_prefix_;
	};

	class direct_reference
	    : public reference_base,
	      public enable_ref_from_this<reference, direct_reference> {
	public:
		direct_reference(ref_tgt tgt_kind,
		                 std::string const& name,
		                 size_t shorthand_prefix,
		                 git_oid const& target)
		    : reference_base{tgt_kind, name, shorthand_prefix}
		    , target_{target} {}
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

	ref_ptr<reference> direct_reference_create(ref_tgt tgt_kind,
	                                           std::string const& name,
	                                           size_t shorthand_prefix,
	                                           git_oid const& target) {
		return make_ref<direct_reference>(tgt_kind, name, shorthand_prefix,
		                                  target);
	}

	class symbolic_reference
	    : public reference_base,
	      public enable_ref_from_this<reference, symbolic_reference> {
	public:
		symbolic_reference(ref_tgt tgt_kind,
		                   std::string const& name,
		                   size_t shorthand_prefix,
		                   std::string const& target,
		                   ref_ptr<references> const& peel_source)
		    : reference_base{tgt_kind, name, shorthand_prefix}
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
			       bead->reference_type() != cov::reference_type::direct) {
				bead = peel_source_->lookup(bead->symbolic_target());
			}
			return bead;
		}

	private:
		std::string target_;
		ref_ptr<references> peel_source_;
	};

	ref_ptr<reference> symbolic_reference_create(
	    ref_tgt tgt_kind,
	    std::string const& name,
	    size_t shorthand_prefix,
	    std::string const& target,
	    ref_ptr<references> const& peel_source) {
		return make_ref<symbolic_reference>(tgt_kind, name, shorthand_prefix,
		                                    target, peel_source);
	}
}  // namespace cov
