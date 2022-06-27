// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/io/strings.hh>

namespace cov::io {
	strings_view::strings_view(strings_view const& other)
	    : strings_view_base{}  // def-init here, will be updated in resync
	    , data{other.data} {
		resync();
	}

	strings_view::strings_view(strings_view&& other) noexcept
	    : strings_view_base{}  // def-init here, will be updated in resync
	    , data{std::move(other.data)} {
		resync();
		other.resync();
	}

	strings_view& strings_view::operator=(strings_view const& other) {
		data = other.data;
		resync();
		return *this;
	}

	strings_view& strings_view::operator=(strings_view&& other) noexcept {
		data = std::move(other.data);
		resync();
		other.resync();
		return *this;
	}

	void strings_view::resync() noexcept { update(data.data(), data.size()); }

	void strings_block::update_() noexcept {
		update(data_.data(), data_.size());
	}

	bool strings_block::reserve_offsets(size_t length) noexcept {
		try {
			offsets_.reserve(length);
		} catch (std::bad_alloc&) {
			return false;
		}

		return offsets_.capacity() >= length;
	}

	bool strings_block::reserve_data(size_t length) noexcept {
		try {
			data_.reserve(length);
		} catch (std::bad_alloc&) {
			update_();  // pointer MIGHT have changed...
			return false;
		}

		update_();
		return data_.capacity() >= length;
	}

	bool strings_block::append(std::string_view value) noexcept {
		if (!reserve_offsets(1 + offsets_.size())) return false;
		if (!reserve_data(value.length() + 1 + data_.size())) return false;

		offsets_.push_back(data_.size());

		auto* str = value.data();
		data_.insert(data_.end(), str, str + value.length());
		data_.push_back(0);
		update_();
		return true;
	}

	bool strings_block::fill32() noexcept {
		auto const extra = data_.size() % 4;
		if (!extra) return true;

		if (!reserve_data(data_.size() - extra + 4)) return false;

		data_.insert(data_.end(), 4 - extra, 0);
		update_();
		return true;
	}

	bool strings_block::is_valid() const noexcept {
		for (size_t off : offsets_) {
			if (!is_valid(off)) return false;
		}
		return is_valid(size());
	}

	size_t strings_block::locate(std::string_view str) const noexcept {
		return locate_or(str, size() + 1);
	}
	size_t strings_block::locate_or(std::string_view str,
	                                size_t value_if_invalid) const noexcept {
		auto it = std::lower_bound(
		    offsets_.begin(), offsets_.end(), str,
		    [&](auto& lhs, auto& rhs) -> bool {
			    using lhs_type =
			        std::remove_cv_t<std::remove_reference_t<decltype(lhs)>>;
			    if constexpr (std::is_same_v<lhs_type, std::string_view>) {
				    auto const rhs_s = at(rhs);
				    return lhs < rhs_s;
			    } else {
				    auto const lhs_s = at(lhs);
				    return lhs_s < rhs;
			    }
		    });
		if (it == offsets_.end() || at(*it) != str) return value_if_invalid;
		return *it;
	}

	strings_builder& strings_builder::insert(std::string_view str) noexcept {
		auto it = strings_.lower_bound(str);
		if (it == strings_.end() || *it != str) {
			try {
				strings_.insert(it, std::string{str.data(), str.size()});
			} catch (...) {
			}
		}

		return *this;
	}

	strings_block strings_builder::build() const noexcept {
		strings_block out;

		auto lengths = strings_.size();
		for (auto const& str : strings_) {
			lengths += str.length();
		}
		lengths = ((lengths + 3) >> 2) << 2;

		if (out.reserve_offsets(strings_.size()) && out.reserve_data(lengths)) {
			for (auto const& str : strings_) {
				out.append(str);
			}
		}

		out.fill32();

		return out;
	}
}  // namespace cov::io
