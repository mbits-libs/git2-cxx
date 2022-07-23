// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)
// for the purpose of cpplint: @generated

#include "hilite/syntax/cxx.hh"
#include "hilite/hilite_impl.hh"

#include "cell/ascii.hh"
#include "cell/character.hh"
#include "cell/context.hh"
#include "cell/operators.hh"
#include "cell/parser.hh"
#include "cell/repeat_operators.hh"
#include "cell/special.hh"
#include "cell/string.hh"
#include "cell/tokens.hh"

#include <limits.h>
#include <cstring>
#include <functional>
#include <string>
#include <string_view>

namespace hl::cxx::parser::callbacks {
	template <typename Iterator>
	struct cxx_grammar_value : grammar_value<Iterator> {
		using grammar_value<Iterator>::grammar_value;
		bool is_raw{false};
	};

	template <typename Context>
	auto& _is_raw_string(Context& context) {
		return _val(context).is_raw;
	}

	RULE_EMIT(newline, token::newline)
	RULE_EMIT(deleted_eol, token::deleted_newline)
	RULE_EMIT(block_comment, token::block_comment)
	RULE_EMIT(line_comment, token::line_comment)
	RULE_EMIT(ws, token::whitespace)
	RULE_EMIT(character_literal, token::character)
	RULE_EMIT(string_literal,
	          _is_raw_string(context) ? token::raw_string : token::string)
	RULE_EMIT(punctuator, token::punctuator)
	RULE_EMIT(pp_identifier, token::preproc_identifier)
	RULE_EMIT(keyword, token::keyword)
	RULE_EMIT(module_name, token::module_name)
	RULE_EMIT(macro_name, token::macro_name)
	RULE_EMIT(macro_arg, token::macro_arg)
	RULE_EMIT(macro_va_args, token::macro_va_args)
	RULE_EMIT(replacement, token::macro_replacement)
	RULE_EMIT(control_line, token::preproc)
	RULE_EMIT(system_header, token::system_header_name)
	RULE_EMIT(local_header, token::local_header_name)
	RULE_EMIT(pp_number, token::number)
	RULE_EMIT(escaped, token::escape_sequence)
	RULE_EMIT(ucn, token::universal_character_name)
	RULE_EMIT(pp_define_arg_list, token::macro_arg_list)

	RULE_EMIT(char_encoding, token::char_encoding)
	RULE_EMIT(char_delim, token::char_delim)
	RULE_EMIT(char_udl, token::char_udl)

	RULE_EMIT(string_encoding, token::string_encoding)
	RULE_EMIT(string_delim, token::string_delim)
	RULE_EMIT(string_udl, token::string_udl)

	using namespace std::literals;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4307)  // C4307: '*': integral constant overflow
#endif

	constexpr static auto keywords = cell::token_set{
	    "alignas"sv,      "alignof"sv,      "asm"sv,
	    "auto"sv,         "bool"sv,         "break"sv,
	    "case"sv,         "catch"sv,        "char"sv,
	    "char16_t"sv,     "char32_t"sv,     "char8_t"sv,
	    "class"sv,        "co_await"sv,     "co_return"sv,
	    "co_yield"sv,     "concept"sv,      "const_cast"sv,
	    "const"sv,        "consteval"sv,    "constexpr"sv,
	    "constinit"sv,    "continue"sv,     "decltype"sv,
	    "default"sv,      "delete"sv,       "do"sv,
	    "double"sv,       "dynamic_cast"sv, "else"sv,
	    "enum"sv,         "explicit"sv,     "export"sv,
	    "extern"sv,       "false"sv,        "float"sv,
	    "for"sv,          "friend"sv,       "goto"sv,
	    "if"sv,           "inline"sv,       "int"sv,
	    "long"sv,         "mutable"sv,      "namespace"sv,
	    "new"sv,          "noexcept"sv,     "nullptr"sv,
	    "operator"sv,     "private"sv,      "protected"sv,
	    "public"sv,       "register"sv,     "reinterpret_cast"sv,
	    "requires"sv,     "return"sv,       "short"sv,
	    "signed"sv,       "sizeof"sv,       "static_assert"sv,
	    "static_cast"sv,  "static"sv,       "struct"sv,
	    "switch"sv,       "template"sv,     "this"sv,
	    "thread_local"sv, "throw"sv,        "true"sv,
	    "try"sv,          "typedef"sv,      "typeid"sv,
	    "typename"sv,     "union"sv,        "unsigned"sv,
	    "using"sv,        "virtual"sv,      "void"sv,
	    "volatile"sv,     "wchar_t"sv,      "while"sv};

	constexpr static auto replacements = cell::token_set{
	    "and"sv,   "and_eq"sv, "bitand"sv, "bitor"sv, "compl"sv, "not"sv,
	    "or_eq"sv, "xor_eq"sv, "not_eq"sv, "or"sv,    "xor"sv};

	constexpr static auto cstdint = cell::token_set{
	    "int8_t"sv,         "int16_t"sv,        "int32_t"sv,
	    "int64_t"sv,        "uint8_t"sv,        "uint16_t"sv,
	    "uint32_t"sv,       "uint64_t"sv,

	    "int_least8_t"sv,   "int_least16_t"sv,  "int_least32_t"sv,
	    "int_least64_t"sv,  "uint_least8_t"sv,  "uint_least16_t"sv,
	    "uint_least32_t"sv, "uint_least64_t"sv,

	    "int_fast8_t"sv,    "int_fast16_t"sv,   "int_fast32_t"sv,
	    "int_fast64_t"sv,   "uint_fast8_t"sv,   "uint_fast16_t"sv,
	    "uint_fast32_t"sv,  "uint_fast64_t"sv,

	    "intmax_t"sv,       "uintmax_t"sv,

	    "size_t"sv,         "ptrdiff_t"sv,      "nullptr_t"sv,
	    "max_align_t"sv,    "byte"sv,
	    "declval"sv  // to allow for bold inside decltype-type of expressions
	};

	constexpr static auto stl =
	    cell::token_set{"allocator"sv,
	                    "polymorphic_allocator"sv,
	                    "numeric_limits"sv,
	                    "type_info"sv,
	                    "initializer_list"sv,
	                    "path"sv,
	                    "directory_entry"sv,
	                    "directory_iterator"sv,
	                    "recursive_directory_iterator"sv,
	                    "unique_ptr"sv,
	                    "shared_ptr"sv,
	                    "weak_ptr"sv,
	                    "optional"sv,
	                    "nullopt_t"sv,
	                    "variant"sv,
	                    "monostate"sv,
	                    "in_place_type"sv,
	                    "any"sv,
	                    "pair"sv,
	                    "tuple"sv,
	                    "integer_sequence"sv,
	                    "index_sequence"sv,
	                    "type_index"sv,

	                    "bad_typeid"sv,
	                    "bad_cast"sv,
	                    "bad_optional_access"sv,
	                    "bad_variant_access"sv,
	                    "bad_any_cast"sv,
	                    "bad_week_ptr"sv,

	                    "is_void"sv,
	                    "is_null_pointer"sv,
	                    "is_integral"sv,
	                    "is_floating_point"sv,
	                    "is_array"sv,
	                    "is_enum"sv,
	                    "is_union"sv,
	                    "is_class"sv,
	                    "is_function"sv,
	                    "is_pointer"sv,
	                    "is_lvalue_reference"sv,
	                    "is_rvalue_reference"sv,
	                    "is_member_object_pointer"sv,
	                    "is_member_function_pointer"sv,
	                    "is_fundamental"sv,
	                    "is_arithmetic"sv,
	                    "is_scalar"sv,
	                    "is_object"sv,
	                    "is_compound"sv,
	                    "is_reference"sv,
	                    "is_member_pointer"sv,
	                    "is_const"sv,
	                    "is_volatile"sv,
	                    "is_trivial"sv,
	                    "is_trivially_copyable"sv,
	                    "is_standard_layout"sv,
	                    "is_pod"sv,
	                    "is_literal_type"sv,
	                    "has_unique_object_representations"sv,
	                    "is_empty"sv,
	                    "is_polymorphic"sv,
	                    "is_abstract"sv,
	                    "is_final"sv,
	                    "is_aggregate"sv,
	                    "is_signed"sv,
	                    "is_unsigned"sv,
	                    "is_constructible"sv,
	                    "is_trivially_constructible"sv,
	                    "is_nothrow_constructible"sv,
	                    "is_default_constructible"sv,
	                    "is_trivially_default_constructible"sv,
	                    "is_nothrow_default_constructible"sv,
	                    "is_copy_constructible"sv,
	                    "is_trivially_copy_constructible"sv,
	                    "is_nothrow_copy_constructible"sv,
	                    "is_move_constructible"sv,
	                    "is_trivially_move_constructible"sv,
	                    "is_nothrow_move_constructible"sv,
	                    "is_assignable"sv,
	                    "is_trivially_assignable"sv,
	                    "is_nothrow_assignable"sv,
	                    "is_copy_assignable"sv,
	                    "is_trivially_copy_assignable"sv,
	                    "is_nothrow_copy_assignable"sv,
	                    "is_move_assignable"sv,
	                    "is_trivially_move_assignable"sv,
	                    "is_nothrow_move_assignable"sv,
	                    "is_destructible"sv,
	                    "is_trivially_destructible"sv,
	                    "is_nothrow_destructible"sv,
	                    "has_virtual_destructor"sv,
	                    "is_swappable_with"sv,
	                    "is_swappable"sv,
	                    "is_nothrow_swappable_with"sv,
	                    "is_nothrow_swappable"sv,
	                    "alignment_of"sv,
	                    "rank"sv,
	                    "extent"sv,
	                    "is_same"sv,
	                    "is_base_of"sv,
	                    "is_convertible"sv,
	                    "is_nothrow_convertible"sv,
	                    "is_invocable"sv,
	                    "is_invocable_r"sv,
	                    "is_nothrow_invocable"sv,
	                    "is_nothrow_invocable_r"sv,

	                    "remove_cv"sv,
	                    "remove_cv_t"sv,
	                    "remove_const"sv,
	                    "remove_const_t"sv,
	                    "remove_volatile"sv,
	                    "remove_volatile_t"sv,
	                    "add_cv"sv,
	                    "add_cv_t"sv,
	                    "add_const"sv,
	                    "add_const_t"sv,
	                    "add_volatile"sv,
	                    "add_volatile_t"sv,
	                    "remove_reference"sv,
	                    "remove_reference_t"sv,
	                    "add_lvalue_reference"sv,
	                    "add_lvalue_reference_t"sv,
	                    "add_rvalue_reference"sv,
	                    "add_rvalue_reference_t"sv,
	                    "remove_pointer"sv,
	                    "remove_pointer_t"sv,
	                    "add_pointer"sv,
	                    "add_pointer_t"sv,
	                    "make_signed"sv,
	                    "make_signed_t"sv,
	                    "make_unsigned"sv,
	                    "make_unsigned_t"sv,
	                    "remove_extent"sv,
	                    "remove_extent_t"sv,
	                    "remove_all_extents"sv,
	                    "remove_all_extents_t"sv,
	                    "aligned_storage"sv,
	                    "aligned_storage_t"sv,
	                    "aligned_union"sv,
	                    "aligned_union_t"sv,
	                    "decay"sv,
	                    "decay_t"sv,
	                    "remove_cvref"sv,
	                    "remove_cvref_t"sv,
	                    "enable_if"sv,
	                    "enable_if_t"sv,
	                    "conditional"sv,
	                    "conditional_t"sv,
	                    "common_type"sv,
	                    "common_type_t"sv,
	                    "common_reference"sv,
	                    "common_reference_t"sv,
	                    "basic_common_reference"sv,
	                    "basic_common_reference_t"sv,
	                    "underlying_type"sv,
	                    "underlying_type_t"sv,
	                    "result_of"sv,
	                    "result_of_t"sv,
	                    "invoke_result"sv,
	                    "invoke_result_t"sv,
	                    "type_identity"sv,
	                    "type_identity_t"sv,
	                    "conjunction"sv,
	                    "conjunction_t"sv,
	                    "disjunction"sv,
	                    "disjunction_t"sv,
	                    "negation"sv,
	                    "negation_t"sv,

	                    "integral_constant"sv,
	                    "bool_constant"sv,
	                    "true_type"sv,
	                    "false_type"sv,
	                    "endian"sv,
	                    "void_t"sv,

	                    "array"sv,
	                    "vector"sv,
	                    "deque"sv,
	                    "forward_list"sv,
	                    "list"sv,
	                    "set"sv,
	                    "map"sv,
	                    "multiset"sv,
	                    "multimap"sv,
	                    "unordered_set"sv,
	                    "unordered_map"sv,
	                    "unordered_multiset"sv,
	                    "unordered_multimap"sv,
	                    "stack"sv,
	                    "queue"sv,
	                    "priority_queue"sv,
	                    "span"sv,
	                    "iterator"sv,
	                    "const_iterator"sv,
	                    "reverse_iterator"sv,
	                    "const_reverse_iterator"sv,

	                    "basic_string"sv,
	                    "basic_string_view"sv,
	                    "string"sv,
	                    "string_view"sv,
	                    "wstring"sv,
	                    "wstring_view"sv,
	                    "u16string"sv,
	                    "u16string_view"sv,
	                    "u32string"sv,
	                    "u32string_view"sv,
	                    "char_traits"sv,

	                    "system_clock"sv,
	                    "steady_clock"sv,
	                    "high_resolution_clock"sv,
	                    "utc_clock"sv,
	                    "tai_clock"sv,
	                    "gps_clock"sv,
	                    "file_clock"sv,
	                    "time_point"sv,
	                    "duration"sv,
	                    "nanoseconds"sv,
	                    "microseconds"sv,
	                    "milliseconds"sv,
	                    "seconds"sv,
	                    "minutes"sv,
	                    "hours"sv,

	                    "weak_equality"sv,
	                    "strong_equality"sv,
	                    "partial_ordering"sv,
	                    "weak_ordering"sv,
	                    "strong_ordering"sv,
	                    "function"sv,
	                    "complex"sv,

	                    "ios_base"sv,
	                    "basic_ios"sv,
	                    "basic_streambuf"sv,
	                    "basic_ostream"sv,
	                    "basic_istream"sv,
	                    "basic_iostream"sv,
	                    "basic_filebuf"sv,
	                    "basic_ifstream"sv,
	                    "basic_ofstream"sv,
	                    "basic_fstream"sv,
	                    "basic_stringbuf"sv,
	                    "basic_istringstream"sv,
	                    "basic_ostringstream"sv,
	                    "basic_stringstream"sv,
	                    "strstreambuf"sv,
	                    "istrstream"sv,
	                    "ostrstream"sv,
	                    "strstream"sv,
	                    "basic_syncbuf"sv,
	                    "basic_osyncstream"sv,
	                    "ios"sv,
	                    "wios"sv,
	                    "streambuf"sv,
	                    "wstreambuf"sv,
	                    "filebuf"sv,
	                    "wfilebuf"sv,
	                    "stringbuf"sv,
	                    "wstringbuf"sv,
	                    "syncbuf"sv,
	                    "wsyncbuf"sv,
	                    "istream"sv,
	                    "wistream"sv,
	                    "ostream"sv,
	                    "wostream"sv,
	                    "iostream"sv,
	                    "wiostream"sv,
	                    "ifstream"sv,
	                    "wifstream"sv,
	                    "ofstream"sv,
	                    "wofstream"sv,
	                    "fstream"sv,
	                    "wfstream"sv,
	                    "istringstream"sv,
	                    "wistringstream"sv,
	                    "ostringstream"sv,
	                    "wostringstream"sv,
	                    "stringstream"sv,
	                    "wstringstream"sv,
	                    "osyncstream"sv,
	                    "wosyncstream"sv,
	                    "cin"sv,
	                    "wcin"sv,
	                    "cout"sv,
	                    "cerr"sv,
	                    "wcerr"sv,
	                    "clog"sv,
	                    "wclog"sv,
	                    "streamoff"sv,
	                    "streamsize"sv,
	                    "fpos"sv,
	                    "streampos"sv,
	                    "wstreampos"sv};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

	auto find_eol_end(const std::string_view& sv, size_t pos) {
		// GCOV_EXCL_START[GCC] - matched was visited, len was not
		auto const len = sv.size();
		// GCOV_EXCL_STOP
		bool matched = false;
		if (pos < len && sv[pos] == '\r') {
			++pos;
			matched = true;
		}
		if (pos < len && sv[pos] == '\n') {
			++pos;
			matched = true;
		}
		return matched ? pos : std::string_view::npos;
	}

	size_t find_del_eol_in_ident(const std::string_view& sv, size_t pos = 0) {
		auto slash = sv.find('\\', pos);
		if (slash == std::string_view::npos) return std::string_view::npos;
		return find_eol_end(sv, slash + 1);
	}

	std::string remove_deleted_eols_in_ident(const std::string_view& sv) {
		auto pos = find_del_eol_in_ident(sv);
		auto prev = decltype(pos){};
		if (pos == std::string_view::npos) return {};

		std::string out;
		out.reserve(sv.length());

		while (pos != std::string_view::npos) {
			auto until = pos;
			while (sv[until] != '\\')
				--until;

			out.append(sv.data() + prev, sv.data() + until);

			prev = pos;
			pos = find_del_eol_in_ident(sv, pos);
		}

		out.append(sv.data() + prev, sv.data() + sv.length());
		return out;
	}

	RULE_MAP(identifier) {
		auto ident_view = _view(context);

		const auto ident_str = remove_deleted_eols_in_ident(ident_view);
		if (!ident_str.empty()) ident_view = ident_str;

		auto const ident = cell::set_entry{ident_view};
		if (keywords.has(ident)) return _emit(context, token::keyword);

		if (replacements.has(ident))
			return _emit(context, token::known_ident_op_replacement);

		if (cstdint.has(ident))
			return _emit(context, token::known_ident_cstdint);

		if (stl.has(ident)) return _emit(context, token::known_ident_stl);

		_emit(context, token::identifier);
	}
}  // namespace hl::cxx::parser::callbacks

namespace hl::cxx::parser {
	using namespace hl::cxx::parser::callbacks;
	using namespace cell;

	static constexpr auto operator""_ident(const char* str, size_t len) {
		return string_token{std::string_view(str, len)}[on_identifier];
	}

	static constexpr auto operator""_pp_ident(const char* str, size_t len) {
		return string_token{std::string_view(str, len)}[on_pp_identifier];
	}

	static constexpr auto operator""_kw(const char* str, size_t len) {
		return string_token{std::string_view(str, len)}[on_keyword];
	}

	// clang-format off
	// leave the rules in semi-readable state.
	constexpr auto line_comment =
	  ("//" >> *(ch - eol))                    [on_line_comment]
	  ;

	constexpr auto block_comment =
	  (
	  "/*" >> *(
	    (ch - '*' - eol)
	    | ('*' >> !ch('/'))
	    | eol                                  [on_newline]
	    ) >> "*/"
	  )                                        [on_block_comment]
	  ;

	constexpr auto SP_char =
	  inlspace                                 [on_ws]
	  | line_comment
	  | block_comment
	  ;

	constexpr auto SP =
	  *SP_char
	  ;

	constexpr auto mandatory_SP =
	  +SP_char
	  ;

	constexpr auto UCN_value =
	  ('u' >> repeat(4)(xdigit))
	  | ('U' >> repeat(8)(xdigit))
	  ;

	constexpr auto nondigit =
	  '_'
	  | alpha
	  ;

	constexpr auto ident_nondigit =
	  '_'
	  | alpha
	  | ('\\' >> UCN_value)                    [on_ucn]
	  ;

	constexpr auto sign = ch("+-");

	constexpr auto header_name =
	  ('<' >> +(ch - eol - '>') >> '>')        [on_system_header]
	  | ('"' >> +(ch - eol - '"') >> '"')      [on_local_header]
	  ;

	constexpr auto identifier =
	  ident_nondigit
	  >> *(ident_nondigit | digit)
	  ;

	constexpr auto pp_number =
	  -ch('.') >> digit >> *(
	    digit
	    | ident_nondigit
	    | ('\'' >> (digit | nondigit))
	    | (ch("eEpP") >> sign)
	    )
	  ;
	// clang-format on

	template <char C>
	struct cxx_char_parser : cell::character_parser<cxx_char_parser<C>> {
		constexpr cxx_char_parser() = default;

		template <typename Iterator, typename Context>
		bool parse(Iterator& first, const Iterator& last, Context& ctx) const {
			this->filter(first, last, ctx);

			if (first == last) return false;

			auto save = first;

			{
				cell::action_state disable_actions{};
				if (eol.parse(first, last, ctx)) {
					first = save;
					return false;
				}
			}

			if (*first == C) {
				first = save;
				return false;
			}

			bool escaped = *first == '\\';
			++first;

			if (!escaped) return true;

			this->filter(first, last, ctx);

			if (first == last) {
				first = save;
				return false;
			}

			auto const escaped_ch = *first;
			++first;

			switch (escaped_ch) {
				case 'u':
					if (repeat(4)(xdigit).parse(first, last, ctx)) {
						if (cell::action_state::enabled()) {
							_setrange(save, first, ctx);
							on_ucn(ctx);
						}
						return true;
					}
					break;
				case 'U':
					if (repeat(8)(xdigit).parse(first, last, ctx)) {
						if (cell::action_state::enabled()) {
							_setrange(save, first, ctx);
							on_ucn(ctx);
						}
						return true;
					}
					break;
				case 'x':
					if ((+xdigit).parse(first, last, ctx)) {
						if (cell::action_state::enabled()) {
							_setrange(save, first, ctx);
							on_escaped(ctx);
						}
						return true;
					}
					break;
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
					repeat(0, 2)(odigit).parse(first, last, ctx);
					if (cell::action_state::enabled()) {
						_setrange(save, first, ctx);
						on_escaped(ctx);
					}
					return true;
				default:
					if (cell::action_state::enabled()) {
						_setrange(save, first, ctx);
						on_escaped(ctx);
					}
					return true;
			}

			first = save;
			return false;
		}
	};

	// clang-format off
	constexpr auto c_char = cxx_char_parser<'\''>{};
	constexpr auto s_char = cxx_char_parser<'"'>{};

	constexpr auto d_char =
	  ch - ch(" ()\\\t\v\f\r\n\"")
	  ;

	constexpr auto encoding_prefix =
	  lit("u8")
	  | "u"
	  | "U"
	  | "L"
	  ;
	// clang-format on

	struct cxx_character_parser : cell::parser<cxx_character_parser> {
		constexpr cxx_character_parser() = default;

		template <typename Iterator, typename Context>
		bool parse(Iterator& first, const Iterator& last, Context& ctx) const {
			Iterator start_delim_end, finish_delim_begin;

			auto get_quot_end = [&start_delim_end](auto& ctx) {
				start_delim_end = _getrange(ctx).second;
			};
			auto get_quot_start = [&finish_delim_begin](auto& ctx) {
				finish_delim_begin = _getrange(ctx).first;
			};
			auto c_character = (ch('\'')[get_quot_end]) >> +c_char >>
			                   (ch('\'')[get_quot_start]);

			auto copy = first;
			(void)encoding_prefix.parse(first, last, ctx);
			auto prefix = first;

			auto result = c_character.parse(first, last, ctx);

			if (!result) {
				first = copy;
				return false;
			}

			if (action_state::enabled()) {
				if (copy != prefix) {
					_setrange(copy, prefix, ctx);
					_emit(ctx, token::char_encoding);
				}

				_setrange(prefix, start_delim_end, ctx);
				_emit(ctx, token::char_delim);

				_setrange(finish_delim_begin, first, ctx);
				_emit(ctx, token::char_delim);
			}

			(void)(identifier[on_char_udl]).parse(first, last, ctx);
			return true;
		}
	};

	struct cxx_string_parser : cell::parser<cxx_string_parser> {
		constexpr cxx_string_parser() = default;
		template <typename Iterator>
		using range_t = std::pair<Iterator, Iterator>;

		template <typename Iterator, typename Context>
		bool parse(Iterator& first, const Iterator& last, Context& ctx) const {
			Iterator start_delim_end, finish_delim_begin;

			_is_raw_string(ctx) = false;

			auto get_quot_end = [&start_delim_end](auto& ctx) {
				start_delim_end = _getrange(ctx).second;
			};
			auto get_quot_start = [&finish_delim_begin](auto& ctx) {
				finish_delim_begin = _getrange(ctx).first;
			};
			auto c_string =
			    (ch('"')[get_quot_end]) >> *s_char >> (ch('"')[get_quot_start]);
			auto copy = first;
			(void)encoding_prefix.parse(first, last, ctx);
			auto prefix = first;
			auto result = false;
			auto is_raw = ch('R').parse(first, last, ctx);
			if (is_raw)
				result = parse_raw(first, last, ctx, start_delim_end,
				                   finish_delim_begin);
			else
				result = c_string.parse(first, last, ctx);

			if (!result) {
				first = copy;
				return false;
			}

			if (action_state::enabled()) {
				if (copy != prefix) {
					_setrange(copy, prefix, ctx);
					_emit(ctx, token::string_encoding);
				}

				_setrange(prefix, start_delim_end, ctx);
				_emit(ctx, token::string_delim);

				_setrange(finish_delim_begin, first, ctx);
				_emit(ctx, token::string_delim);
			}

			(void)(identifier[on_string_udl]).parse(first, last, ctx);
			return true;
		}

		template <typename Iterator, typename Context>
		bool parse_raw(Iterator& first,
		               const Iterator& last,
		               Context& ctx,
		               Iterator& start_delim_end,
		               Iterator& finish_delim_begin) const {
			if (!ch('"').parse(first, last, ctx)) return false;

			_is_raw_string(ctx) = true;
			auto const front_delim_start = first;
			while (d_char.parse(first, last, ctx))
				;
			auto const front_delim_end = first;

			if (!ch('(').parse(first, last, ctx)) return false;

			start_delim_end = first;

			while (first != last) {
				while (first != last) {
					static constexpr auto signaling_eol = eol[on_newline];
					if (signaling_eol.parse(first, last, ctx)) continue;
					auto copy = first;
					if (ch(')').parse(first, last, ctx)) {
						finish_delim_begin = copy;
						break;
					}
					++first;
				}
				if (!parse_end_delim(front_delim_start, front_delim_end, first,
				                     last))
					continue;

				auto copy = first;
				if (ch('"').parse(copy, last, ctx)) {
					first = copy;
					return true;
				}
			}
			return false;
		}

		template <typename Iterator>
		bool parse_end_delim(Iterator delim_first,
		                     const Iterator& delim_last,
		                     Iterator& first,
		                     const Iterator& last) const {
			if (delim_first == delim_last) return true;

			while ((delim_first != delim_last) && (first != last)) {
				if (*delim_first != *first) return false;
				++delim_first;
				++first;
			}
			return true;
		}
	};

	// clang-format off
	constexpr auto character_literal = cxx_character_parser{};
	constexpr auto string_literal = cxx_string_parser{};

	constexpr auto operators =
	  lit("...") | "<=>" | "<<=" | ">>=" | "<<" | ">>" | "<:" | ":>" | "<%" |
	  "%>" | "%:%:" | "%:" | "::" | "->*" | "->" | ".*" | "+=" | "-=" | "*=" |
	  "/=" | "%=" | "^=" | "&=" | "|=" | "++" | "--" | "==" | "!=" | "<=" |
	  ">=" | "&&" | "||" | "##" | ch("<>{}[]#()=;:?.~!+-*/%^&|,");

	constexpr auto preprocessing_token =
	  character_literal                        [on_character_literal]
	  | string_literal                         [on_string_literal]
	  | identifier                             [on_identifier]
	  | pp_number                              [on_pp_number]
	  | operators                              [on_punctuator]
	  ;

	constexpr auto mSP = mandatory_SP;
	constexpr auto header_name_tokens = 
	  string_literal
	  | ('<'
	    >> SP
	    >> +((preprocessing_token - '>') >> SP)
	    >> SP
	    >> '>'
	    )
	  ;
	constexpr auto __has_include_token =
	  "__has_include"_ident
	  >> SP
	  >> '('
	  >> SP >> (header_name | header_name_tokens)
	  >> SP
	  >> ')'
	  ;

	constexpr auto constant_expression =
	  +(
	    (
	      __has_include_token
	      | preprocessing_token
	      )
	    >> SP
	    )
	  ;

	constexpr auto pp_tokens =
	  preprocessing_token
	  >> *(SP >> preprocessing_token)
	  ;
	constexpr auto opt_pp_tokens =
	  -(mSP >> pp_tokens)
	  >> SP
	  ;
	constexpr auto pp_define_arg_list =
	  ( '('
	    >> SP
	    >> (
	      lit("...")                           [on_macro_va_args]
	      | identifier                         [on_macro_arg]
	        >> *(
	          SP
	          >> ','
	          >> SP
	          >> identifier                    [on_macro_arg]
	          )
	        >> -(
	          SP
	          >> ','
	          >> SP
	          >> lit("...")                    [on_macro_va_args]
	          )
	      | eps
	      )
	    >> SP
	    >> ')'
	    )                                      [on_pp_define_arg_list]
	  ;
	// clang-format on

	template <typename Wrapped>
	struct wrapped : cell::parser<wrapped<Wrapped>>, Wrapped {
		constexpr wrapped(const Wrapped& inner) : Wrapped(inner) {}

		template <typename Iterator, typename Context>
		bool parse(Iterator& first, const Iterator& last, Context& ctx) const {
			return Wrapped::operator()().parse(first, last, ctx);
		}
	};

	template <typename Wrapped>
	constexpr wrapped<Wrapped> wrap(const Wrapped& inner) {
		return inner;
	}

	// clang-format off
	constexpr auto pp_include = wrap([] {
		static constexpr auto grammar =
		  "include"_pp_ident
		  >> SP
		  >> +((header_name | preprocessing_token) >> SP);
		return (grammar);
	});

	constexpr auto pp_define = wrap([] {
		static constexpr auto grammar =
		  "define"_pp_ident
		  >> mSP
		  >> identifier                      [on_macro_name]
		  >> -pp_define_arg_list
		  >> SP
		  >> (*(preprocessing_token >> SP))  [on_replacement]
		  ;
		return (grammar);
	});
	constexpr auto pp_undef = wrap([] {
		static constexpr auto grammar =
		  "undef"_pp_ident
		  >> mSP
		  >> identifier                      [on_macro_name]
		  >> SP
		  ;
		return (grammar);
	});
	constexpr auto pp_if = wrap([] {
		static constexpr auto grammar =
		  "if"_pp_ident
		  >> mSP
		  >> constant_expression
		  ;
		return (grammar);
	});
	constexpr auto pp_ifdef = wrap([] {
		static constexpr auto grammar =
		  "ifdef"_pp_ident
		  >> mSP
		  >> identifier                      [on_macro_name]
		  >> SP
		  ;
		return (grammar);
	});
	constexpr auto pp_ifndef = wrap([] {
		static constexpr auto grammar =
		  "ifndef"_pp_ident
		  >> mSP
		  >> identifier                      [on_macro_name]
		  >> SP
		  ;
		return (grammar);
	});
	constexpr auto pp_elifdef = wrap([] {
		static constexpr auto grammar =
		  "elifdef"_pp_ident
		  >> mSP
		  >> identifier                      [on_macro_name]
		  >> SP
		  ;
		return (grammar);
	});
	constexpr auto pp_elifndef = wrap([] {
		static constexpr auto grammar =
		  "elifndef"_pp_ident
		  >> mSP
		  >> identifier                      [on_macro_name]
		  >> SP
		  ;
		return (grammar);
	});

	// at this point, this is too much for MSVC, it will not treat whatever it sees as
	// constexpression anymore.

	struct shorter_typename : cell::parser<shorter_typename> {
		static constexpr auto parser =
		  pp_include
		  | pp_define
		  | pp_undef
		  | pp_ifdef
		  | pp_ifndef
		  | pp_if
		  | pp_elifdef
		  | pp_elifndef
		  | ("elif"_pp_ident >> mSP >> constant_expression)
		  | "else"_pp_ident >> SP
		  | "endif"_pp_ident >> SP
		  | ("line"_pp_ident >> mSP >> +(preprocessing_token >> SP))
		  | ("error"_pp_ident >> opt_pp_tokens)
		  | ("pragma"_pp_ident >> opt_pp_tokens)
		  | opt_pp_tokens
		  ;

		constexpr shorter_typename() = default;
		template <typename Iterator, typename Context>
		bool parse(Iterator& first, const Iterator& last, Context& ctx) const {
			return parser.parse(first, last, ctx);
		}
	};

	constexpr auto pp_control = shorter_typename{};

	constexpr auto not_a_semicolon = preprocessing_token - ';';
	constexpr auto module_name =
	  not_a_semicolon
	  >> *(SP >> not_a_semicolon)
	  ;
	constexpr auto pp_module_name =
	  module_name                              [on_module_name]
	  ;
	constexpr auto pp_semi = SP >> (ch(';')[on_punctuator]) >> SP;
	constexpr auto pp_module =
	  -"export"_kw
	  >> SP
	  >> "module"_kw
	  >> SP
	  >> (
	    (':' >> SP >> "private"_kw)
	    | -pp_module_name
	    )
	  >> pp_semi
	  ;
	constexpr auto pp_import =
	  -"export"_kw
	  >> SP
	  >> "import"_kw
	  >> SP
	  >> (
	    (
	      (header_name | header_name_tokens)
	      >> opt_pp_tokens
	      )
	    | pp_module_name
	    )
	  >> pp_semi
	  ;

	constexpr auto control_line =
	  ('#' >> SP >> pp_control)
	  | (ahead(pp_module) >> pp_module)
	  | (ahead(pp_import) >> pp_import)
	  ;

	constexpr auto text_line = *(preprocessing_token >> SP);
	constexpr auto line =
	  SP >> (
	    control_line                           [on_control_line]
	    | text_line
	    ) >> SP
	  ;
	constexpr auto preprocessing_file =
	  *(
	    ahead(line >> eol)
	    >> line
	    >> eol                                 [on_newline]
	    )
	  >> -line
	  ;
	// clang-format on

	struct deleted_eol_parser : cell::parser<deleted_eol_parser> {
		constexpr deleted_eol_parser() = default;

		template <typename Iterator, typename Context>
		bool parse(Iterator& first, const Iterator& last, Context& ctx) const {
			auto const emit0 = first;
			if (first != last && *first == '\\') {
				++first;
				auto const emit1 = first;
				if (eol.parse(first, last, ctx)) {
					if (cell::action_state::enabled()) {
						_setrange(emit0, emit1, ctx);
						on_deleted_eol(ctx);

						_setrange(emit1, first, ctx);
						on_newline(ctx);
					}
					return true;
				}
			}
			return false;
		}
	};

	constexpr auto deleted_eol = deleted_eol_parser{};
}  // namespace hl::cxx::parser

namespace hl::cxx {
	using namespace hl::cxx::parser;

	std::string_view token_to_string(unsigned tok) noexcept {
		using namespace std::literals;

#define STRINGIFY(x)        \
	case hl::cxx::token::x: \
		return #x##sv;
		switch (tok) {
			CXX_HILITE_TOKENS(STRINGIFY)
			default:
				break;
		}
#undef STRINGIFY

		return {};
	}

	void tokenize(const std::string_view& contents, callback& result) {
		auto begin = contents.begin();
		const auto end = contents.end();
		auto value = grammar_result{};

		parse_with_restart<cxx_grammar_value>(begin, end, preprocessing_file,
		                                      deleted_eol, value);
		value.produce_lines(result, contents.size());
	}
}  // namespace hl::cxx
