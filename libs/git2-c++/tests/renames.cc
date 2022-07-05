// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cstdio>
#include <filesystem>
#include <fstream>
#include "setup.hh"

namespace git::testing::renames {
	using namespace ::std::literals;

	namespace {
		namespace file {
			struct text {
				std::string_view path{};
				std::string_view content{};
			};

			struct binary {
				std::string_view path{};
				std::basic_string_view<unsigned char> content{};
			};
		}  // namespace file

		template <size_t Length>
		constexpr std::basic_string_view<unsigned char> span(
		    unsigned char const (&v)[Length]) noexcept {
			return {v, Length};
		}

		namespace file {
			static constexpr auto text_1 =
			    "A file content.\n"
			    "Second line.\n"sv;
			static constexpr auto text_2 = "ref: refs/heads/master\n"sv;
			static constexpr auto text_3 =
			    "[core]\n"
			    "	repositoryformatversion = 0\n"
			    "	filemode = true\n"
			    "	bare = false\n"
			    "	logallrefupdates = true\n"sv;
			static constexpr auto text_4 =
			    "4e9be2749a866125dac0bb7fbe26c5a1ca667aac\n"sv;
			static constexpr auto text_5 = "{}"sv;

			static constexpr unsigned char binary_1[] = {
			    0x44, 0x49, 0x52, 0x43, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00,
			    0x00, 0x02, 0x62, 0xc4, 0x69, 0x8a, 0x1d, 0x7c, 0xe8, 0xa0,
			    0x62, 0xc4, 0x69, 0x8a, 0x1d, 0x7c, 0xe8, 0xa0, 0x00, 0x01,
			    0x03, 0x02, 0x01, 0x24, 0x45, 0x0f, 0x00, 0x00, 0x81, 0xa4,
			    0x00, 0x00, 0x03, 0xe8, 0x00, 0x00, 0x03, 0xe8, 0x00, 0x00,
			    0x00, 0x02, 0x9e, 0x26, 0xdf, 0xee, 0xb6, 0xe6, 0x41, 0xa3,
			    0x3d, 0xae, 0x49, 0x61, 0x19, 0x62, 0x35, 0xbd, 0xb9, 0x65,
			    0xb2, 0x1b, 0x00, 0x12, 0x70, 0x72, 0x6f, 0x70, 0x65, 0x72,
			    0x2f, 0x63, 0x6f, 0x6e, 0x66, 0x69, 0x67, 0x2e, 0x6a, 0x73,
			    0x6f, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			    0x62, 0xc4, 0x69, 0xa5, 0x1a, 0x6a, 0x69, 0xaa, 0x62, 0xc4,
			    0x62, 0xe1, 0x1b, 0x05, 0x5b, 0xeb, 0x00, 0x01, 0x03, 0x02,
			    0x01, 0x23, 0x5e, 0xd6, 0x00, 0x00, 0x81, 0xa4, 0x00, 0x00,
			    0x03, 0xe8, 0x00, 0x00, 0x03, 0xe8, 0x00, 0x00, 0x00, 0x1d,
			    0xdb, 0x97, 0xd5, 0x68, 0x14, 0x99, 0x6d, 0xf5, 0x3d, 0x13,
			    0xfa, 0x7a, 0x5a, 0x2b, 0xb7, 0x4e, 0xb8, 0x77, 0x1a, 0x67,
			    0x00, 0x12, 0x73, 0x75, 0x62, 0x66, 0x6f, 0x6c, 0x64, 0x65,
			    0x72, 0x2f, 0x66, 0x69, 0x6c, 0x65, 0x2e, 0x74, 0x78, 0x74,
			    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x54, 0x52,
			    0x45, 0x45, 0x00, 0x00, 0x00, 0x5a, 0x00, 0x32, 0x20, 0x32,
			    0x0a, 0xd2, 0x32, 0x01, 0xac, 0x94, 0x61, 0x71, 0x9c, 0x63,
			    0x8c, 0xea, 0x1d, 0xd7, 0x27, 0x1a, 0x27, 0x46, 0xe7, 0x16,
			    0x8d, 0x70, 0x72, 0x6f, 0x70, 0x65, 0x72, 0x00, 0x31, 0x20,
			    0x30, 0x0a, 0x5e, 0xe4, 0x06, 0x76, 0x7b, 0xc3, 0x70, 0x57,
			    0x57, 0xd4, 0x9a, 0xcd, 0xea, 0x6e, 0x88, 0x38, 0x03, 0x8b,
			    0x27, 0xef, 0x73, 0x75, 0x62, 0x66, 0x6f, 0x6c, 0x64, 0x65,
			    0x72, 0x00, 0x31, 0x20, 0x30, 0x0a, 0x2c, 0x5a, 0xd4, 0x6a,
			    0x86, 0x03, 0x06, 0x2a, 0x11, 0x0a, 0x7c, 0x20, 0xf1, 0xde,
			    0x53, 0xc5, 0x80, 0x42, 0x87, 0x6a, 0xcb, 0x27, 0x7d, 0xea,
			    0x0a, 0xb9, 0xe3, 0x45, 0xaa, 0x42, 0x92, 0x37, 0xac, 0xda,
			    0x26, 0x73, 0x54, 0xd6, 0x80, 0xb4};

			static constexpr unsigned char binary_2[] = {
			    0x78, 0x01, 0x4b, 0xca, 0xc9, 0x4f, 0x52, 0x30, 0xb2,
			    0x64, 0x70, 0x54, 0x48, 0xcb, 0xcc, 0x49, 0x55, 0x48,
			    0xce, 0xcf, 0x2b, 0x49, 0xcd, 0x2b, 0xd1, 0xe3, 0x0a,
			    0x4e, 0x05, 0x32, 0x53, 0x14, 0x72, 0x32, 0xf3, 0x52,
			    0xf5, 0xb8, 0x00, 0xdf, 0x25, 0x0b, 0xdb};

			static constexpr unsigned char binary_3[] = {
			    0x78, 0x01, 0x4b, 0xca, 0xc9, 0x4f, 0x52, 0x30,
			    0x34, 0x63, 0x70, 0x54, 0x48, 0xcb, 0xcc, 0x49,
			    0x55, 0x48, 0xce, 0xcf, 0x2b, 0x49, 0xcd, 0x2b,
			    0xd1, 0xe3, 0x02, 0x00, 0x5c, 0x1f, 0x07, 0x7b};

			static constexpr unsigned char binary_4[] = {
			    0x78, 0x01, 0x2b, 0x29, 0x4a, 0x4d, 0x55, 0x30, 0xb6, 0x64,
			    0x30, 0x34, 0x30, 0x30, 0x33, 0x31, 0x51, 0x48, 0xce, 0xcf,
			    0x4b, 0xcb, 0x4c, 0xd7, 0xcb, 0x2a, 0xce, 0xcf, 0x63, 0x98,
			    0xa7, 0x76, 0xff, 0xdd, 0xb6, 0x67, 0x8e, 0x8b, 0x6d, 0xd7,
			    0x79, 0x26, 0x4a, 0x26, 0x99, 0xee, 0xdd, 0x99, 0xba, 0x49,
			    0x1a, 0x00, 0x7b, 0x01, 0x11, 0xe8};

			static constexpr unsigned char binary_5[] = {
			    0x78, 0x01, 0x9d, 0x8d, 0x3b, 0x0a, 0x42, 0x31, 0x10, 0x45,
			    0xad, 0xb3, 0x8a, 0xe9, 0x05, 0xc9, 0xc7, 0x7c, 0x1e, 0x88,
			    0xd8, 0x5a, 0xb8, 0x01, 0xbb, 0x31, 0x1f, 0xdf, 0x80, 0x49,
			    0x20, 0xce, 0x6b, 0x5c, 0xbd, 0x01, 0x77, 0x60, 0x75, 0xe1,
			    0xc0, 0x39, 0x37, 0xf6, 0x5a, 0x89, 0x41, 0x05, 0xbf, 0xe3,
			    0x91, 0x33, 0x94, 0x25, 0x59, 0x2d, 0xa3, 0x75, 0x3a, 0x18,
			    0x63, 0xd0, 0x62, 0x76, 0x3e, 0xcb, 0x12, 0x95, 0xd6, 0x01,
			    0xd1, 0x05, 0x87, 0xb6, 0xe4, 0xa2, 0xac, 0xc0, 0x8d, 0xd7,
			    0x3e, 0xe0, 0x86, 0x23, 0x52, 0x83, 0x7b, 0xda, 0x1a, 0x9c,
			    0xea, 0x67, 0xce, 0xa5, 0x52, 0x6a, 0xf4, 0x5c, 0xf9, 0x41,
			    0xfc, 0x3e, 0xc4, 0x5e, 0xcf, 0xa0, 0x9c, 0xf5, 0xd2, 0xf8,
			    0xe3, 0xa2, 0x61, 0x2f, 0xb5, 0x94, 0x62, 0xd2, 0xf9, 0xca,
			    0xf9, 0x5f, 0x5f, 0x5c, 0x1b, 0x31, 0xe1, 0x0b, 0x7e, 0x21,
			    0xf1, 0x05, 0xfe, 0x43, 0x3d, 0xf6};

			static constexpr unsigned char binary_6[] = {
			    0x78, 0x01, 0x2b, 0x29, 0x4a, 0x4d, 0x55, 0x30, 0x36,
			    0x63, 0x30, 0x34, 0x30, 0x30, 0x33, 0x31, 0x51, 0x48,
			    0xcb, 0xcc, 0x49, 0xd5, 0x2b, 0xa9, 0x28, 0x61, 0x78,
			    0x1a, 0xb1, 0xe3, 0x51, 0xc7, 0xf2, 0x95, 0x46, 0x39,
			    0xc7, 0x64, 0x5a, 0x02, 0x32, 0x5f, 0x79, 0xa9, 0xb0,
			    0x9e, 0x5a, 0x0b, 0x00, 0x4d, 0xe7, 0x10, 0xf7};

			static constexpr unsigned char binary_7[] = {
			    0x78, 0x01, 0x4b, 0xca, 0xc9, 0x4f, 0x52, 0x30, 0x62,
			    0xa8, 0xae, 0x05, 0x00, 0x0f, 0x0b, 0x02, 0xea};

			static constexpr unsigned char binary_8[] = {
			    0x78, 0x01, 0x2b, 0x29, 0x4a, 0x4d, 0x55, 0x30, 0x37, 0x67,
			    0x30, 0x31, 0x00, 0x02, 0x85, 0xe2, 0xfc, 0xdc, 0xd4, 0xf2,
			    0x8c, 0xd4, 0xa2, 0x54, 0x85, 0xd4, 0x9c, 0xe2, 0x54, 0x86,
			    0xb8, 0x27, 0x6c, 0x65, 0xd5, 0x87, 0x0b, 0xc2, 0xc3, 0xaf,
			    0xcc, 0x3a, 0xfb, 0x2a, 0xaf, 0xc3, 0x82, 0xb9, 0x5b, 0xfd,
			    0x3d, 0x54, 0x61, 0x69, 0x52, 0x5a, 0x7e, 0x4e, 0x4a, 0x6a,
			    0x11, 0xc3, 0xa4, 0x0f, 0xae, 0x4c, 0xa2, 0x4b, 0xac, 0x75,
			    0x18, 0x36, 0x6b, 0x6e, 0x62, 0x15, 0x6f, 0x7e, 0x64, 0x17,
			    0xa4, 0xce, 0x0f, 0x00, 0x1c, 0x27, 0x1e, 0x94};

			static constexpr unsigned char binary_9[] = {
			    0x78, 0x01, 0x2b, 0x29, 0x4a, 0x4d, 0x55, 0x30, 0x31, 0x60,
			    0x30, 0x34, 0x30, 0x30, 0x33, 0x31, 0x51, 0x48, 0xcb, 0xcc,
			    0x49, 0xcd, 0x4b, 0xcc, 0x4d, 0xd5, 0x2b, 0xa9, 0x28, 0x61,
			    0xb8, 0x3d, 0xfd, 0x6a, 0x86, 0xc8, 0xcc, 0xdc, 0xaf, 0xb6,
			    0xc2, 0xbf, 0xaa, 0xa2, 0xb4, 0xb7, 0xfb, 0xed, 0x28, 0x97,
			    0x4a, 0x07, 0x00, 0x88, 0xdc, 0x12, 0x0f};

			static constexpr unsigned char binary_10[] = {
			    0x78, 0x01, 0x2b, 0x29, 0x4a, 0x4d, 0x55, 0x30, 0x36,
			    0x63, 0x30, 0x34, 0x30, 0x30, 0x33, 0x31, 0x51, 0x48,
			    0xcb, 0xcc, 0x49, 0xd5, 0x2b, 0xa9, 0x28, 0x61, 0xb8,
			    0x3d, 0xfd, 0x6a, 0x86, 0xc8, 0xcc, 0xdc, 0xaf, 0xb6,
			    0xc2, 0xbf, 0xaa, 0xa2, 0xb4, 0xb7, 0xfb, 0xed, 0x28,
			    0x97, 0x4a, 0x07, 0x00, 0x48, 0x36, 0x10, 0x73};

			static constexpr unsigned char binary_11[] = {
			    0x78, 0x01, 0x9d, 0x8e, 0x31, 0x8a, 0xc3, 0x30, 0x10, 0x45,
			    0xb7, 0xd6, 0x29, 0xa6, 0x0f, 0x04, 0x4b, 0x9a, 0x91, 0x34,
			    0xb0, 0x84, 0x5c, 0x20, 0x55, 0xba, 0x74, 0xf2, 0x68, 0xbc,
			    0x56, 0x21, 0x79, 0x71, 0x94, 0x26, 0xa7, 0x8f, 0x61, 0x6f,
			    0xb0, 0xd5, 0x83, 0x0f, 0xef, 0xf3, 0x64, 0x6b, 0xad, 0x0e,
			    0x70, 0x1e, 0xbf, 0xc6, 0xae, 0x0a, 0x5c, 0xd8, 0x4a, 0x2a,
			    0x49, 0xd0, 0x79, 0xb6, 0xc5, 0x5b, 0x4f, 0x4e, 0xd5, 0x66,
			    0x66, 0x22, 0x16, 0x49, 0xc9, 0xfa, 0x65, 0x46, 0xa7, 0xe6,
			    0x37, 0xef, 0xda, 0x07, 0x08, 0x66, 0x67, 0x85, 0xe2, 0x12,
			    0x28, 0x90, 0xc3, 0x88, 0x59, 0x13, 0x23, 0x2d, 0x09, 0xe9,
			    0x10, 0x59, 0xb9, 0x44, 0xb1, 0xb8, 0x78, 0x93, 0x5f, 0x63,
			    0xdd, 0x76, 0xb8, 0xe5, 0x5d, 0x6a, 0x87, 0x47, 0x79, 0x75,
			    0xf8, 0x6e, 0xef, 0x03, 0xd7, 0x56, 0x4b, 0xaf, 0x3f, 0xeb,
			    0x98, 0xeb, 0x78, 0x9e, 0x65, 0x6b, 0x17, 0xb0, 0x81, 0xe2,
			    0xe4, 0x63, 0xf0, 0x01, 0x4e, 0x93, 0x9b, 0x26, 0x73, 0xac,
			    0x47, 0xe5, 0xd0, 0xff, 0xfa, 0xe6, 0xae, 0xb2, 0xf5, 0x02,
			    0x7f, 0x3f, 0xe6, 0x03, 0x10, 0x1c, 0x4a, 0x64};

			static constexpr unsigned char binary_12[] = {
			    0x78, 0x01, 0x9d, 0x8e, 0x41, 0x0a, 0x02, 0x31, 0x0c, 0x45,
			    0x5d, 0xf7, 0x14, 0xd9, 0x0b, 0x92, 0xa6, 0x74, 0xa6, 0x05,
			    0x11, 0x3d, 0x80, 0x17, 0x70, 0x97, 0x36, 0xa9, 0x76, 0xd1,
			    0x8e, 0x8c, 0x9d, 0x8d, 0xa7, 0x77, 0xce, 0xe0, 0xea, 0xc3,
			    0xe3, 0x3d, 0xf8, 0x79, 0x69, 0xad, 0x0e, 0x20, 0x67, 0x0f,
			    0x63, 0x55, 0x05, 0x3f, 0x27, 0x0e, 0x3e, 0x60, 0x22, 0x2d,
			    0x56, 0x78, 0xb2, 0xd1, 0x05, 0x9c, 0x72, 0x8e, 0x85, 0x6d,
			    0x48, 0x5a, 0x32, 0x79, 0xe2, 0x8c, 0xc9, 0xbc, 0x79, 0xd5,
			    0x3e, 0x40, 0xcb, 0xe4, 0x48, 0x62, 0xa4, 0x22, 0xe8, 0x10,
			    0x6d, 0x50, 0x1f, 0x5c, 0x14, 0x41, 0x3b, 0x2b, 0xcf, 0x16,
			    0x31, 0x28, 0xd3, 0x2e, 0x18, 0xde, 0xc6, 0x6b, 0x59, 0xe1,
			    0xce, 0x6b, 0xae, 0x1d, 0x1e, 0xb2, 0x75, 0x38, 0xb7, 0xef,
			    0x3e, 0xd7, 0x56, 0xa5, 0xd7, 0xe7, 0x6b, 0xa4, 0x3a, 0x3e,
			    0xa7, 0xbc, 0xb4, 0x0b, 0xd8, 0xc9, 0xcf, 0xe8, 0x22, 0xf9,
			    0x08, 0x47, 0x24, 0x44, 0xb3, 0xd3, 0xfd, 0xe5, 0xd0, 0x7f,
			    0x7b, 0x73, 0x13, 0x81, 0xbc, 0xf4, 0x52, 0x9f, 0xe6, 0x07,
			    0xe2, 0x5f, 0x49, 0xe7};

			static constexpr unsigned char binary_13[] = {
			    0x78, 0x01, 0x2b, 0x29, 0x4a, 0x4d, 0x55, 0x30, 0xb3, 0x64,
			    0x30, 0x31, 0x00, 0x02, 0x85, 0x82, 0xa2, 0xfc, 0x82, 0xd4,
			    0x22, 0x86, 0xb8, 0x27, 0x6c, 0x65, 0xd5, 0x87, 0x0b, 0xc2,
			    0xc3, 0xaf, 0xcc, 0x3a, 0xfb, 0x2a, 0xaf, 0xc3, 0x82, 0xb9,
			    0x5b, 0xfd, 0x3d, 0x44, 0x41, 0x71, 0x69, 0x52, 0x5a, 0x7e,
			    0x4e, 0x0a, 0x50, 0x8d, 0x4e, 0xd4, 0x95, 0xac, 0x36, 0x66,
			    0x36, 0x2d, 0x41, 0xae, 0x1a, 0x85, 0x8f, 0xf7, 0x82, 0x8f,
			    0x36, 0x38, 0xb5, 0x67, 0x01, 0x00, 0x32, 0x08, 0x1c, 0xa5};

			static constexpr unsigned char binary_14[] = {
			    0x78, 0x01, 0x4b, 0xca, 0xc9, 0x4f, 0x52, 0x30,
			    0x60, 0x00, 0x00, 0x09, 0xb0, 0x01, 0xf0};

			static constexpr unsigned char binary_15[] = {
			    0x78, 0x01, 0x2b, 0x29, 0x4a, 0x4d, 0x55, 0x30, 0x36,
			    0x63, 0x30, 0x31, 0x00, 0x02, 0x85, 0xe2, 0xd2, 0xa4,
			    0xb4, 0xfc, 0x9c, 0x94, 0xd4, 0x22, 0x86, 0x49, 0x1f,
			    0x5c, 0x99, 0x44, 0x97, 0x58, 0xeb, 0x30, 0x6c, 0xd6,
			    0xdc, 0xc4, 0x2a, 0xde, 0xfc, 0xc8, 0x2e, 0x48, 0x9d,
			    0x1f, 0x00, 0x30, 0x22, 0x0d, 0xd2};

			static constexpr unsigned char binary_16[] = {
			    0x78, 0x01, 0x9d, 0x8e, 0x3b, 0x0e, 0xc2, 0x30, 0x10, 0x05,
			    0xa9, 0x7d, 0x8a, 0xed, 0x91, 0xd0, 0xfa, 0x13, 0x6f, 0x2c,
			    0x21, 0x44, 0x47, 0xc5, 0x05, 0xe8, 0x36, 0xf6, 0x42, 0x5c,
			    0x38, 0x46, 0xc6, 0x29, 0xc8, 0xe9, 0xc9, 0x19, 0xa8, 0x9e,
			    0x34, 0x9a, 0x91, 0x5e, 0xac, 0xa5, 0xe4, 0x0e, 0xc6, 0x84,
			    0x43, 0x6f, 0x22, 0x90, 0x8c, 0x35, 0xa8, 0x39, 0x06, 0xe7,
			    0x35, 0xe9, 0x10, 0xbd, 0x1d, 0xa3, 0xb0, 0x4e, 0x89, 0x0c,
			    0x69, 0x36, 0xe4, 0xbc, 0x90, 0xf6, 0x63, 0x52, 0x6f, 0x6e,
			    0xb2, 0x74, 0x98, 0x12, 0x71, 0xf0, 0xb2, 0xdb, 0x2c, 0x09,
			    0xa7, 0xa7, 0xa0, 0x75, 0x1c, 0x08, 0x1d, 0x8e, 0x3c, 0x78,
			    0x87, 0xc2, 0x93, 0xb3, 0x64, 0x9d, 0xe2, 0xb5, 0xcf, 0xb5,
			    0xc1, 0x9d, 0x5b, 0xcc, 0x0b, 0x3c, 0xd2, 0xba, 0xc0, 0xb9,
			    0x6c, 0xfb, 0x5c, 0x4b, 0x4e, 0x4b, 0x7e, 0xcd, 0x7d, 0xca,
			    0xfd, 0x73, 0x8a, 0xb5, 0x5c, 0x40, 0xfb, 0x81, 0xd0, 0x06,
			    0x13, 0x08, 0x8e, 0x68, 0x10, 0xd5, 0x4e, 0xf7, 0x97, 0x5d,
			    0xfe, 0xed, 0xd5, 0xad, 0x42, 0x6c, 0xbc, 0x7d, 0xd5, 0x0f,
			    0xb8, 0xdb, 0x48, 0x8f};
		}  // namespace file

		constexpr std::string_view subdirs[] = {
		    "renames/.git/objects/pack"sv,
		    "renames/.git/objects/info"sv,
		    "renames/.git/refs/tags"sv,
		    "renames/.git/branches"sv,
		};

		constexpr file::text text[] = {
		    {"renames/.git/HEAD"sv, file::text_2},
		    {"renames/.git/config"sv, file::text_3},
		    {"renames/.git/refs/heads/master"sv, file::text_4},
		    {"renames/proper/config.json"sv, file::text_5},
		    {"renames/subfolder/file.txt"sv, file::text_1},
		};

		constexpr file::binary binary[] = {
		    {"renames/.git/index"sv, span(file::binary_1)},
		    {"renames/.git/objects/2c/5ad46a8603062a110a7c20f1de53c58042876a"sv,
		     span(file::binary_10)},
		    {"renames/.git/objects/4e/9be2749a866125dac0bb7fbe26c5a1ca667aac"sv,
		     span(file::binary_16)},
		    {"renames/.git/objects/57/ba8580b2ef1da6193806cc9fa18befc252ac0b"sv,
		     span(file::binary_8)},
		    {"renames/.git/objects/5e/e406767bc3705757d49acdea6e8838038b27ef"sv,
		     span(file::binary_4)},
		    {"renames/.git/objects/92/f0450215a43b2c00b329b2051783e23e52270f"sv,
		     span(file::binary_9)},
		    {"renames/.git/objects/9d/91c8d8c42391d31352ee1a99559cc8813fb42e"sv,
		     span(file::binary_15)},
		    {"renames/.git/objects/9e/26dfeeb6e641a33dae4961196235bdb965b21b"sv,
		     span(file::binary_7)},
		    {"renames/.git/objects/bd/7a96e461aed0bfe034a970408a5640eab43734"sv,
		     span(file::binary_12)},
		    {"renames/.git/objects/c4/a21c57f65652474ae8945f84552e9e9d7c14f3"sv,
		     span(file::binary_5)},
		    {"renames/.git/objects/d2/3201ac9461719c638cea1dd7271a2746e7168d"sv,
		     span(file::binary_13)},
		    {"renames/.git/objects/db/97d56814996df53d13fa7a5a2bb74eb8771a67"sv,
		     span(file::binary_2)},
		    {"renames/.git/objects/e5/58b8e288a7a9326cc61c845069ea4a2405caad"sv,
		     span(file::binary_3)},
		    {"renames/.git/objects/e6/9de29bb2d1d6434b8b29ae775ad8c2e48c5391"sv,
		     span(file::binary_14)},
		    {"renames/.git/objects/ef/632d992fd030018e5839dd017ea71008ea22d9"sv,
		     span(file::binary_11)},
		    {"renames/.git/objects/f9/d520c5628333a5ae67e0fc1228aa686a5fef15"sv,
		     span(file::binary_6)},
		};
	}  // namespace

	repository open_repo() {
		using namespace std::filesystem;

		std::error_code ignore{};
		remove_all(setup::test_dir() / "renames"sv, ignore);

		for (auto const subdir : subdirs) {
			create_directories(setup::test_dir() / setup::make_path(subdir),
			                   ignore);
		}

		for (auto const [filename, contents] : text) {
			auto const p = setup::test_dir() / setup::make_path(filename);
			create_directories(p.parent_path(), ignore);
			std::ofstream out{p};
			out.write(contents.data(),
			          static_cast<std::streamsize>(contents.size()));
		}

		for (auto const nfo : binary) {
			auto const p = setup::test_dir() / setup::make_path(nfo.path);
			create_directories(p.parent_path(), ignore);
			std::ofstream out{p, std::ios::binary};
			out.write(reinterpret_cast<char const*>(nfo.content.data()),
			          static_cast<std::streamsize>(nfo.content.size()));
		}

		return git::repository::open(setup::test_dir() / "renames/.git"sv);
	}
}  // namespace git::testing::renames
