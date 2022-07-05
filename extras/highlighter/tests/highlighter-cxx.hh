// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include "hilite/cxx.hh"

#define HL_CXX(TOKEN) \
	{ hl::cxx::TOKEN, #TOKEN##s }

namespace highlighter::testing {
	static hilite_ctx const cxx_ctx{"HL_CXX"sv, "cxx"sv};

#define TOK_(x) using tok_cxx_##x = tok_<hl::cxx::token::x>;
	CXX_HILITE_TOKENS(TOK_)
#undef TOK_

	static hilite_test const cxx[] = {
	    {
	        "hello-world.cc"sv,
	        R"(#include <iostream>

int main() {
    std::cout << "Hello, World!\n";
})"sv,
	        cxx_ctx,
	        {
	            .dict{HL_CXX(system_header_name)},
	            .lines{
	                hl_line{
	                    0u,
	                    {tok_cxx_meta{
	                        text_span{0u, 1u},
	                        tok_cxx_meta_identifier{text_span{1u, 8u}},
	                        text_span{8u, 9u},
	                        tok_cxx_system_header_name{text_span{9u, 19u}}}}},
	                hl_line{20u, {}},
	                hl_line{
	                    21u,
	                    {tok_cxx_keyword{text_span{0u, 3u}}, text_span{3u, 4u},
	                     tok_cxx_identifier{text_span{4u, 8u}},
	                     tok_cxx_punctuator{text_span{8u, 9u}},
	                     tok_cxx_punctuator{text_span{9u, 10u}},
	                     text_span{10u, 11u},
	                     tok_cxx_punctuator{text_span{11u, 12u}}},
	                },
	                hl_line{
	                    34u,
	                    {text_span{0u, 4u},
	                     tok_cxx_identifier{text_span{4u, 7u}},
	                     tok_cxx_punctuator{text_span{7u, 9u}},
	                     tok_cxx_known_ident_3{text_span{9u, 13u}},
	                     text_span{13u, 14u},
	                     tok_cxx_punctuator{text_span{14u, 16u}},
	                     text_span{16u, 17u},
	                     tok_cxx_string{
	                         tok_cxx_string_delim{text_span{17u, 18u}},
	                         text_span{18u, 31u},
	                         tok_cxx_escape_sequence{text_span{31u, 33u}},
	                         tok_cxx_string_delim{text_span{33u, 34u}}},
	                     tok_cxx_punctuator{text_span{34u, 35u}}},
	                },
	                hl_line{70u, {tok_cxx_punctuator{text_span{0u, 1u}}}},
	            },
	        },
	    },
	    {
	        "hello-world-local.cc"sv,
	        R"(#include "iostream"

int main() {
    std::cout << "Hello, World!\n";
})"sv,
	        cxx_ctx,
	        {
	            .dict{HL_CXX(local_header_name)},
	            .lines{
	                hl_line{
	                    0u,
	                    {tok_cxx_meta{
	                        text_span{0u, 1u},
	                        tok_cxx_meta_identifier{text_span{1u, 8u}},
	                        text_span{8u, 9u},
	                        tok_cxx_local_header_name{text_span{9u, 19u}}}}},
	                hl_line{20u, {}},
	                hl_line{
	                    21u,
	                    {tok_cxx_keyword{text_span{0u, 3u}}, text_span{3u, 4u},
	                     tok_cxx_identifier{text_span{4u, 8u}},
	                     tok_cxx_punctuator{text_span{8u, 9u}},
	                     tok_cxx_punctuator{text_span{9u, 10u}},
	                     text_span{10u, 11u},
	                     tok_cxx_punctuator{text_span{11u, 12u}}},
	                },
	                hl_line{
	                    34u,
	                    {text_span{0u, 4u},
	                     tok_cxx_identifier{text_span{4u, 7u}},
	                     tok_cxx_punctuator{text_span{7u, 9u}},
	                     tok_cxx_known_ident_3{text_span{9u, 13u}},
	                     text_span{13u, 14u},
	                     tok_cxx_punctuator{text_span{14u, 16u}},
	                     text_span{16u, 17u},
	                     tok_cxx_string{
	                         tok_cxx_string_delim{text_span{17u, 18u}},
	                         text_span{18u, 31u},
	                         tok_cxx_escape_sequence{text_span{31u, 33u}},
	                         tok_cxx_string_delim{text_span{33u, 34u}}},
	                     tok_cxx_punctuator{text_span{34u, 35u}}},
	                },
	                hl_line{70u, {tok_cxx_punctuator{text_span{0u, 1u}}}},
	            },
	        },
	    },
	    {
	        "include/x-macro.h"sv,
	        R"(#define CXX_TOKENS(X)                                  \
	X(deleted_newline) /*slash followed by a newline*/ \
	X(local_header_name)                               \
	X(system_header_name)                              \
	X(universal_character_name)                        \
	X(macro_name)                                      \
	X(macro_arg_list)                                  \
	X(macro_arg)                                       \
	X(macro_va_args)                                   \
	X(macro_replacement)

#define CXX_HILITE_TOKENS(X) \
	HILITE_TOKENS(X)         \
	CXX_TOKENS(X)
)"sv,
	        cxx_ctx,
	        {
	            .dict{HL_CXX(deleted_newline), HL_CXX(macro_name),
	                  HL_CXX(macro_arg_list), HL_CXX(macro_arg),
	                  HL_CXX(macro_replacement)},
	            .lines{
	                hl_line{0u,
	                        {tok_cxx_meta{
	                            text_span{0u, 1u},
	                            tok_cxx_meta_identifier{text_span{1u, 7u}},
	                            text_span{7u, 8u},
	                            tok_cxx_macro_name{text_span{8u, 18u}},
	                            tok_cxx_macro_arg_list{
	                                text_span{18u, 19u},
	                                tok_cxx_macro_arg{text_span{19u, 20u}},
	                                text_span{20u, 21u}},
	                            text_span{21u, 55u},
	                            tok_cxx_deleted_newline{text_span{55u, 56u}}}}},
	                hl_line{57u,
	                        {tok_cxx_meta{
	                            text_span{0u, 1u},
	                            tok_cxx_macro_replacement{
	                                tok_cxx_identifier{text_span{1u, 2u}},
	                                tok_cxx_punctuator{text_span{2u, 3u}},
	                                tok_cxx_identifier{text_span{3u, 18u}},
	                                tok_cxx_punctuator{text_span{18u, 19u}},
	                                text_span{19u, 20u},
	                                tok_cxx_block_comment{text_span{20u, 51u}},
	                                text_span{51u, 52u},
	                                tok_cxx_deleted_newline{
	                                    text_span{52u, 53u}}}}}},
	                hl_line{
	                    111u,
	                    {tok_cxx_meta{tok_cxx_macro_replacement{
	                        text_span{0u, 1u},
	                        tok_cxx_identifier{text_span{1u, 2u}},
	                        tok_cxx_punctuator{text_span{2u, 3u}},
	                        tok_cxx_identifier{text_span{3u, 20u}},
	                        tok_cxx_punctuator{text_span{20u, 21u}},
	                        text_span{21u, 52u},
	                        tok_cxx_deleted_newline{text_span{52u, 53u}}}}}},
	                hl_line{
	                    165u,
	                    {tok_cxx_meta{tok_cxx_macro_replacement{
	                        text_span{0u, 1u},
	                        tok_cxx_identifier{text_span{1u, 2u}},
	                        tok_cxx_punctuator{text_span{2u, 3u}},
	                        tok_cxx_identifier{text_span{3u, 21u}},
	                        tok_cxx_punctuator{text_span{21u, 22u}},
	                        text_span{22u, 52u},
	                        tok_cxx_deleted_newline{text_span{52u, 53u}}}}}},
	                hl_line{
	                    219u,
	                    {tok_cxx_meta{tok_cxx_macro_replacement{
	                        text_span{0u, 1u},
	                        tok_cxx_identifier{text_span{1u, 2u}},
	                        tok_cxx_punctuator{text_span{2u, 3u}},
	                        tok_cxx_identifier{text_span{3u, 27u}},
	                        tok_cxx_punctuator{text_span{27u, 28u}},
	                        text_span{28u, 52u},
	                        tok_cxx_deleted_newline{text_span{52u, 53u}}}}}},
	                hl_line{
	                    273u,
	                    {tok_cxx_meta{tok_cxx_macro_replacement{
	                        text_span{0u, 1u},
	                        tok_cxx_identifier{text_span{1u, 2u}},
	                        tok_cxx_punctuator{text_span{2u, 3u}},
	                        tok_cxx_identifier{text_span{3u, 13u}},
	                        tok_cxx_punctuator{text_span{13u, 14u}},
	                        text_span{14u, 52u},
	                        tok_cxx_deleted_newline{text_span{52u, 53u}}}}}},
	                hl_line{
	                    327u,
	                    {tok_cxx_meta{tok_cxx_macro_replacement{
	                        text_span{0u, 1u},
	                        tok_cxx_identifier{text_span{1u, 2u}},
	                        tok_cxx_punctuator{text_span{2u, 3u}},
	                        tok_cxx_identifier{text_span{3u, 17u}},
	                        tok_cxx_punctuator{text_span{17u, 18u}},
	                        text_span{18u, 52u},
	                        tok_cxx_deleted_newline{text_span{52u, 53u}}}}}},
	                hl_line{
	                    381u,
	                    {tok_cxx_meta{tok_cxx_macro_replacement{
	                        text_span{0u, 1u},
	                        tok_cxx_identifier{text_span{1u, 2u}},
	                        tok_cxx_punctuator{text_span{2u, 3u}},
	                        tok_cxx_identifier{text_span{3u, 12u}},
	                        tok_cxx_punctuator{text_span{12u, 13u}},
	                        text_span{13u, 52u},
	                        tok_cxx_deleted_newline{text_span{52u, 53u}}}}}},
	                hl_line{
	                    435u,
	                    {tok_cxx_meta{tok_cxx_macro_replacement{
	                        text_span{0u, 1u},
	                        tok_cxx_identifier{text_span{1u, 2u}},
	                        tok_cxx_punctuator{text_span{2u, 3u}},
	                        tok_cxx_identifier{text_span{3u, 16u}},
	                        tok_cxx_punctuator{text_span{16u, 17u}},
	                        text_span{17u, 52u},
	                        tok_cxx_deleted_newline{text_span{52u, 53u}}}}}},
	                hl_line{489u,
	                        {tok_cxx_meta{tok_cxx_macro_replacement{
	                            text_span{0u, 1u},
	                            tok_cxx_identifier{text_span{1u, 2u}},
	                            tok_cxx_punctuator{text_span{2u, 3u}},
	                            tok_cxx_identifier{text_span{3u, 20u}},
	                            tok_cxx_punctuator{text_span{20u, 21u}}}}}},
	                hl_line{511u, {}},
	                hl_line{512u,
	                        {tok_cxx_meta{
	                            text_span{0u, 1u},
	                            tok_cxx_meta_identifier{text_span{1u, 7u}},
	                            text_span{7u, 8u},
	                            tok_cxx_macro_name{text_span{8u, 25u}},
	                            tok_cxx_macro_arg_list{
	                                text_span{25u, 26u},
	                                tok_cxx_macro_arg{text_span{26u, 27u}},
	                                text_span{27u, 28u}},
	                            text_span{28u, 29u},
	                            tok_cxx_deleted_newline{text_span{29u, 30u}}}}},
	                hl_line{543u,
	                        {tok_cxx_meta{
	                            text_span{0u, 1u},
	                            tok_cxx_macro_replacement{
	                                tok_cxx_identifier{text_span{1u, 14u}},
	                                tok_cxx_punctuator{text_span{14u, 15u}},
	                                tok_cxx_identifier{text_span{15u, 16u}},
	                                tok_cxx_punctuator{text_span{16u, 17u}},
	                                text_span{17u, 26u},
	                                tok_cxx_deleted_newline{
	                                    text_span{26u, 27u}}}}}},
	                hl_line{571u,
	                        {tok_cxx_meta{tok_cxx_macro_replacement{
	                            text_span{0u, 1u},
	                            tok_cxx_identifier{text_span{1u, 11u}},
	                            tok_cxx_punctuator{text_span{11u, 12u}},
	                            tok_cxx_identifier{text_span{12u, 13u}},
	                            tok_cxx_punctuator{text_span{13u, 14u}}}}}},
	                hl_line{586u, {}},
	            },
	        },
	    },
	    {
	        "line-comment.cpp"sv,
	        R"(// line comment
auto const PI = 3.14159; // another comment, \
continued to next line)"sv,
	        cxx_ctx,
	        {
	            .dict{HL_CXX(deleted_newline)},
	            .lines{
	                hl_line{0u, {tok_cxx_line_comment{text_span{0u, 15u}}}},
	                hl_line{
	                    16u,
	                    {tok_cxx_keyword{text_span{0u, 4u}}, text_span{4u, 5u},
	                     tok_cxx_keyword{text_span{5u, 10u}},
	                     text_span{10u, 11u},
	                     tok_cxx_identifier{text_span{11u, 13u}},
	                     text_span{13u, 14u},
	                     tok_cxx_punctuator{text_span{14u, 15u}},
	                     text_span{15u, 16u},
	                     tok_cxx_number{text_span{16u, 17u}},
	                     tok_cxx_number{text_span{17u, 23u}},
	                     tok_cxx_punctuator{text_span{23u, 24u}},
	                     text_span{24u, 25u},
	                     tok_cxx_line_comment{
	                         text_span{25u, 45u},
	                         tok_cxx_deleted_newline{text_span{45u, 46u}}}},
	                },
	                hl_line{63u, {tok_cxx_line_comment{text_span{0u, 22u}}}},
	            },
	        },
	    },
	    {
	        "characters.hxx"sv,
	        R"('a'; '\n'; u8'🍌'; L'W', 'x'_suffix;)"sv,
	        cxx_ctx,
	        {
	            .dict{},
	            .lines{
	                hl_line{
	                    0u,
	                    {tok_cxx_character{
	                         tok_cxx_char_delim{text_span{0u, 1u}},
	                         text_span{1u, 2u},
	                         tok_cxx_char_delim{text_span{2u, 3u}}},
	                     tok_cxx_punctuator{text_span{3u, 4u}},
	                     text_span{4u, 5u},
	                     tok_cxx_character{
	                         tok_cxx_char_delim{text_span{5u, 6u}},
	                         tok_cxx_escape_sequence{text_span{6u, 8u}},
	                         tok_cxx_char_delim{text_span{8u, 9u}}},
	                     tok_cxx_punctuator{text_span{9u, 10u}},
	                     text_span{10u, 11u},
	                     tok_cxx_character{
	                         tok_cxx_char_encoding{text_span{11u, 13u}},
	                         tok_cxx_char_delim{text_span{13u, 14u}},
	                         text_span{14u, 18u},
	                         tok_cxx_char_delim{text_span{18u, 19u}}},
	                     tok_cxx_punctuator{text_span{19u, 20u}},
	                     text_span{20u, 21u},
	                     tok_cxx_character{
	                         tok_cxx_char_encoding{text_span{21u, 22u}},
	                         tok_cxx_char_delim{text_span{22u, 23u}},
	                         text_span{23u, 24u},
	                         tok_cxx_char_delim{text_span{24u, 25u}}},
	                     tok_cxx_punctuator{text_span{25u, 26u}},
	                     text_span{26u, 27u},
	                     tok_cxx_character{
	                         tok_cxx_char_delim{text_span{27u, 28u}},
	                         text_span{28u, 29u},
	                         tok_cxx_char_delim{text_span{29u, 30u}},
	                         tok_cxx_char_udl{text_span{30u, 37u}}},
	                     tok_cxx_punctuator{text_span{37u, 38u}}},
	                },
	            },
	        },
	    },
	    {
	        "bad-characters.hxx"sv,
	        R"('\xGG';
'\ubad';
'\U1234123';
'\)"sv,
	        cxx_ctx,
	        {
	            .dict{},
	            .lines{
	                hl_line{0u, {text_span{0u, 7u}}},
	                hl_line{8u, {text_span{0u, 8u}}},
	                hl_line{17u, {text_span{0u, 12u}}},
	                hl_line{30u, {text_span{0u, 2u}}},
	            },
	        },
	    },
	    {
	        "strings.c++"sv,
	        R"(u8"oct: \033[m; hex: \x1B[m;"sv;
u"banana: \uD83C\uDF4C"sv;
U"banana: \U0001F34C"sv;
"split\
-string";)"sv,
	        cxx_ctx,
	        {
	            .dict{HL_CXX(deleted_newline),
	                  HL_CXX(universal_character_name)},
	            .lines{
	                hl_line{
	                    0u,
	                    {tok_cxx_string{
	                         tok_cxx_string_encoding{text_span{0u, 2u}},
	                         tok_cxx_string_delim{text_span{2u, 3u}},
	                         text_span{3u, 8u},
	                         tok_cxx_escape_sequence{text_span{8u, 12u}},
	                         text_span{12u, 21u},
	                         tok_cxx_escape_sequence{text_span{21u, 25u}},
	                         text_span{25u, 28u},
	                         tok_cxx_string_delim{text_span{28u, 29u}},
	                         tok_cxx_string_udl{text_span{29u, 31u}}},
	                     tok_cxx_punctuator{text_span{31u, 32u}}},
	                },
	                hl_line{
	                    33u,
	                    {tok_cxx_string{
	                         tok_cxx_string_encoding{text_span{0u, 1u}},
	                         tok_cxx_string_delim{text_span{1u, 2u}},
	                         text_span{2u, 10u},
	                         tok_cxx_universal_character_name{
	                             text_span{10u, 16u}},
	                         tok_cxx_universal_character_name{
	                             text_span{16u, 22u}},
	                         tok_cxx_string_delim{text_span{22u, 23u}},
	                         tok_cxx_string_udl{text_span{23u, 25u}}},
	                     tok_cxx_punctuator{text_span{25u, 26u}}},
	                },
	                hl_line{
	                    60u,
	                    {tok_cxx_string{
	                         tok_cxx_string_encoding{text_span{0u, 1u}},
	                         tok_cxx_string_delim{text_span{1u, 2u}},
	                         text_span{2u, 10u},
	                         tok_cxx_universal_character_name{
	                             text_span{10u, 20u}},
	                         tok_cxx_string_delim{text_span{20u, 21u}},
	                         tok_cxx_string_udl{text_span{21u, 23u}}},
	                     tok_cxx_punctuator{text_span{23u, 24u}}},
	                },
	                hl_line{85u,
	                        {tok_cxx_string{
	                            tok_cxx_string_delim{text_span{0u, 1u}},
	                            text_span{1u, 6u},
	                            tok_cxx_deleted_newline{text_span{6u, 7u}}}}},
	                hl_line{
	                    93u,
	                    {tok_cxx_string{
	                         text_span{0u, 7u},
	                         tok_cxx_string_delim{text_span{7u, 8u}}},
	                     tok_cxx_punctuator{text_span{8u, 9u}}},
	                },
	            },
	        },
	    },
	    {
	        "raw-strings.cpp"sv,
	        R"(u8R"prefix(oct: \033[m; hex: \x1B[m;)prefix"
LR"prefix(content)not yet)prefix"
)"
	        R"--=--(R"(without
prefix)"
R"(line \
continuations \
returned)"sv;)--=--"sv,

	        cxx_ctx,
	        {
	            .dict{HL_CXX(deleted_newline)},
	            .lines{
	                hl_line{0u,
	                        {tok_cxx_raw_string{
	                            tok_cxx_string_encoding{text_span{0u, 2u}},
	                            tok_cxx_string_delim{text_span{2u, 11u}},
	                            text_span{11u, 36u},
	                            tok_cxx_string_delim{text_span{36u, 44u}}}}},
	                hl_line{45u,
	                        {tok_cxx_raw_string{
	                            tok_cxx_string_encoding{text_span{0u, 1u}},
	                            tok_cxx_string_delim{text_span{1u, 10u}},
	                            text_span{10u, 25u},
	                            tok_cxx_string_delim{text_span{25u, 33u}}}}},
	                hl_line{79u,
	                        {tok_cxx_raw_string{
	                            tok_cxx_string_delim{text_span{0u, 3u}},
	                            text_span{3u, 10u}}}},
	                hl_line{90u,
	                        {tok_cxx_raw_string{
	                            text_span{0u, 6u},
	                            tok_cxx_string_delim{text_span{6u, 8u}}}}},
	                hl_line{99u,
	                        {tok_cxx_raw_string{
	                            tok_cxx_string_delim{text_span{0u, 3u}},
	                            text_span{3u, 8u},
	                            tok_cxx_deleted_newline{text_span{8u, 9u}}}}},
	                hl_line{109u,
	                        {tok_cxx_raw_string{
	                            text_span{0u, 14u},
	                            tok_cxx_deleted_newline{text_span{14u, 15u}}}}},
	                hl_line{
	                    125u,
	                    {tok_cxx_raw_string{
	                         text_span{0u, 8u},
	                         tok_cxx_string_delim{text_span{8u, 10u}},
	                         tok_cxx_string_udl{text_span{10u, 12u}}},
	                     tok_cxx_punctuator{text_span{12u, 13u}}},
	                },
	            },
	        },
	    },
	    {
	        "va-args.C"sv,
	        "#define outer(...) inner(__VA_ARGS__)"sv,
	        cxx_ctx,
	        {
	            .dict{HL_CXX(macro_name), HL_CXX(macro_arg_list),
	                  HL_CXX(macro_va_args), HL_CXX(macro_replacement)},
	            .lines{
	                hl_line{0u,
	                        {tok_cxx_meta{
	                            text_span{0u, 1u},
	                            tok_cxx_meta_identifier{text_span{1u, 7u}},
	                            text_span{7u, 8u},
	                            tok_cxx_macro_name{text_span{8u, 13u}},
	                            tok_cxx_macro_arg_list{
	                                text_span{13u, 14u},
	                                tok_cxx_macro_va_args{text_span{14u, 17u}},
	                                text_span{17u, 18u}},
	                            text_span{18u, 19u},
	                            tok_cxx_macro_replacement{
	                                tok_cxx_identifier{text_span{19u, 24u}},
	                                tok_cxx_punctuator{text_span{24u, 25u}},
	                                tok_cxx_identifier{text_span{25u, 36u}},
	                                tok_cxx_punctuator{text_span{36u, 37u}}}}}},
	            },
	        },
	    },
	    {
	        "control-lines.cpp"sv,
	        R"(#pragma something, something, the Dark Side

#include <system>
#include "local"

#define MACRO
#	undef MACRO

#if expression
#else
#endif

#ifdef MACRO
#endif

#ifndef MACRO
#endif

#line 234 "file.h"

#error I'm afraid, Dave
)"sv,
	        cxx_ctx,
	        {
	            .dict{HL_CXX(local_header_name), HL_CXX(system_header_name),
	                  HL_CXX(macro_name), HL_CXX(macro_replacement)},
	            .lines{
	                hl_line{0u,
	                        {tok_cxx_meta{
	                            text_span{0u, 1u},
	                            tok_cxx_meta_identifier{text_span{1u, 7u}},
	                            text_span{7u, 8u},
	                            tok_cxx_identifier{text_span{8u, 17u}},
	                            tok_cxx_punctuator{text_span{17u, 18u}},
	                            text_span{18u, 19u},
	                            tok_cxx_identifier{text_span{19u, 28u}},
	                            tok_cxx_punctuator{text_span{28u, 29u}},
	                            text_span{29u, 30u},
	                            tok_cxx_identifier{text_span{30u, 33u}},
	                            text_span{33u, 34u},
	                            tok_cxx_identifier{text_span{34u, 38u}},
	                            text_span{38u, 39u},
	                            tok_cxx_identifier{text_span{39u, 43u}}}}},
	                hl_line{44u, {}},
	                hl_line{
	                    45u,
	                    {tok_cxx_meta{
	                        text_span{0u, 1u},
	                        tok_cxx_meta_identifier{text_span{1u, 8u}},
	                        text_span{8u, 9u},
	                        tok_cxx_system_header_name{text_span{9u, 17u}}}}},
	                hl_line{
	                    63u,
	                    {tok_cxx_meta{
	                        text_span{0u, 1u},
	                        tok_cxx_meta_identifier{text_span{1u, 8u}},
	                        text_span{8u, 9u},
	                        tok_cxx_local_header_name{text_span{9u, 16u}}}}},
	                hl_line{80u, {}},
	                hl_line{81u,
	                        {tok_cxx_meta{
	                            text_span{0u, 1u},
	                            tok_cxx_meta_identifier{text_span{1u, 7u}},
	                            text_span{7u, 8u},
	                            tok_cxx_macro_name{text_span{8u, 13u}}}}},
	                hl_line{95u,
	                        {tok_cxx_meta{
	                            text_span{0u, 2u},
	                            tok_cxx_meta_identifier{text_span{2u, 7u}},
	                            text_span{7u, 8u},
	                            tok_cxx_macro_name{text_span{8u, 13u}}}}},
	                hl_line{109u, {}},
	                hl_line{110u,
	                        {tok_cxx_meta{
	                            text_span{0u, 1u},
	                            tok_cxx_meta_identifier{text_span{1u, 3u}},
	                            text_span{3u, 4u},
	                            tok_cxx_identifier{text_span{4u, 14u}}}}},
	                hl_line{125u,
	                        {tok_cxx_meta{
	                            text_span{0u, 1u},
	                            tok_cxx_meta_identifier{text_span{1u, 5u}}}}},
	                hl_line{131u,
	                        {tok_cxx_meta{
	                            text_span{0u, 1u},
	                            tok_cxx_meta_identifier{text_span{1u, 6u}}}}},
	                hl_line{138u, {}},
	                hl_line{139u,
	                        {tok_cxx_meta{
	                            text_span{0u, 1u},
	                            tok_cxx_meta_identifier{text_span{1u, 6u}},
	                            text_span{6u, 7u},
	                            tok_cxx_macro_name{text_span{7u, 12u}}}}},
	                hl_line{152u,
	                        {tok_cxx_meta{
	                            text_span{0u, 1u},
	                            tok_cxx_meta_identifier{text_span{1u, 6u}}}}},
	                hl_line{159u, {}},
	                hl_line{160u,
	                        {tok_cxx_meta{
	                            text_span{0u, 1u},
	                            tok_cxx_meta_identifier{text_span{1u, 7u}},
	                            text_span{7u, 8u},
	                            tok_cxx_macro_name{text_span{8u, 13u}}}}},
	                hl_line{174u,
	                        {tok_cxx_meta{
	                            text_span{0u, 1u},
	                            tok_cxx_meta_identifier{text_span{1u, 6u}}}}},
	                hl_line{181u, {}},
	                hl_line{
	                    182u,
	                    {tok_cxx_meta{
	                        text_span{0u, 1u},
	                        tok_cxx_meta_identifier{text_span{1u, 5u}},
	                        text_span{5u, 6u},
	                        tok_cxx_number{text_span{6u, 9u}},
	                        text_span{9u, 10u},
	                        tok_cxx_string{
	                            tok_cxx_string_delim{text_span{10u, 11u}},
	                            text_span{11u, 17u},
	                            tok_cxx_string_delim{text_span{17u, 18u}}}}}},
	                hl_line{201u, {}},
	                hl_line{
	                    202u,
	                    {tok_cxx_meta{
	                         text_span{0u, 1u},
	                         tok_cxx_meta_identifier{text_span{1u, 6u}},
	                         text_span{6u, 7u},
	                         tok_cxx_identifier{text_span{7u, 8u}}},
	                     text_span{8u, 23u}},
	                },
	                hl_line{226u, {}},
	            },
	        },
	    },
	    {
	        "splice-idents.cpp"sv,
	        R"(int func(std::ve\
ctor<int> cons\
t& in) {
	ret\
urn;
})"sv,
	        cxx_ctx,
	        {
	            .dict{HL_CXX(deleted_newline)},
	            .lines{
	                hl_line{
	                    0u,
	                    {tok_cxx_keyword{text_span{0u, 3u}}, text_span{3u, 4u},
	                     tok_cxx_identifier{text_span{4u, 8u}},
	                     tok_cxx_punctuator{text_span{8u, 9u}},
	                     tok_cxx_identifier{text_span{9u, 12u}},
	                     tok_cxx_punctuator{text_span{12u, 14u}},
	                     tok_cxx_known_ident_3{
	                         text_span{14u, 16u},
	                         tok_cxx_deleted_newline{text_span{16u, 17u}}}},
	                },
	                hl_line{
	                    18u,
	                    {tok_cxx_known_ident_3{text_span{0u, 4u}},
	                     tok_cxx_punctuator{text_span{4u, 5u}},
	                     tok_cxx_keyword{text_span{5u, 8u}},
	                     tok_cxx_punctuator{text_span{8u, 9u}},
	                     text_span{9u, 10u},
	                     tok_cxx_keyword{
	                         text_span{10u, 14u},
	                         tok_cxx_deleted_newline{text_span{14u, 15u}}}},
	                },
	                hl_line{
	                    34u,
	                    {tok_cxx_keyword{text_span{0u, 1u}},
	                     tok_cxx_punctuator{text_span{1u, 2u}},
	                     text_span{2u, 3u},
	                     tok_cxx_identifier{text_span{3u, 5u}},
	                     tok_cxx_punctuator{text_span{5u, 6u}},
	                     text_span{6u, 7u},
	                     tok_cxx_punctuator{text_span{7u, 8u}}},
	                },
	                hl_line{
	                    43u,
	                    {text_span{0u, 1u},
	                     tok_cxx_keyword{
	                         text_span{1u, 4u},
	                         tok_cxx_deleted_newline{text_span{4u, 5u}}}},
	                },
	                hl_line{
	                    49u,
	                    {tok_cxx_keyword{text_span{0u, 3u}},
	                     tok_cxx_punctuator{text_span{3u, 4u}}},
	                },
	                hl_line{54u, {tok_cxx_punctuator{text_span{0u, 1u}}}},
	            },
	        },
	    },
	    {
	        "splice-idents-other-eols.cpp"sv,
	        "int func(std::ve\\\r\n"
	        "ctor<int> cons\\\r"
	        "t& in) {\n"
	        "	ret\\\r\n"
	        "urn;\n"
	        "}"sv,
	        cxx_ctx,
	        {
	            .dict{HL_CXX(deleted_newline)},
	            .lines{
	                hl_line{
	                    0u,
	                    {tok_cxx_keyword{text_span{0u, 3u}}, text_span{3u, 4u},
	                     tok_cxx_identifier{text_span{4u, 8u}},
	                     tok_cxx_punctuator{text_span{8u, 9u}},
	                     tok_cxx_identifier{text_span{9u, 12u}},
	                     tok_cxx_punctuator{text_span{12u, 14u}},
	                     tok_cxx_known_ident_3{
	                         text_span{14u, 16u},
	                         tok_cxx_deleted_newline{text_span{16u, 17u}}}},
	                },
	                hl_line{
	                    19u,
	                    {tok_cxx_known_ident_3{text_span{0u, 4u}},
	                     tok_cxx_punctuator{text_span{4u, 5u}},
	                     tok_cxx_keyword{text_span{5u, 8u}},
	                     tok_cxx_punctuator{text_span{8u, 9u}},
	                     text_span{9u, 10u},
	                     tok_cxx_keyword{
	                         text_span{10u, 14u},
	                         tok_cxx_deleted_newline{text_span{14u, 15u}}}},
	                },
	                hl_line{
	                    35u,
	                    {tok_cxx_keyword{text_span{0u, 1u}},
	                     tok_cxx_punctuator{text_span{1u, 2u}},
	                     text_span{2u, 3u},
	                     tok_cxx_identifier{text_span{3u, 5u}},
	                     tok_cxx_punctuator{text_span{5u, 6u}},
	                     text_span{6u, 7u},
	                     tok_cxx_punctuator{text_span{7u, 8u}}},
	                },
	                hl_line{
	                    44u,
	                    {text_span{0u, 1u},
	                     tok_cxx_keyword{
	                         text_span{1u, 4u},
	                         tok_cxx_deleted_newline{text_span{4u, 5u}}}},
	                },
	                hl_line{
	                    51u,
	                    {tok_cxx_keyword{text_span{0u, 3u}},
	                     tok_cxx_punctuator{text_span{3u, 4u}}},
	                },
	                hl_line{56u, {tok_cxx_punctuator{text_span{0u, 1u}}}},
	            },
	        },
	    },
	    /*
	    {
	        ".cpp"sv,
	        R"()"sv,
	        cxx_ctx,
	        {},
	    },
	     */
	};

	INSTANTIATE_TEST_SUITE_P(cxx, highlighter, ::testing::ValuesIn(cxx));
}  // namespace highlighter::testing
