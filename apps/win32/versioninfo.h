// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <Windows.h>
#include <cov/version.hh>

#define STRINGIFY1(x) #x
#define STRINGIFY(x) STRINGIFY1(x)

#define RC_ORGANISATION "midnightBITS"
#define RC_MODULE COV_PROJECT_NAME
#define RC_VERSION_STRING \
	COV_VERSION_STR       \
	COV_VERSION_STABILITY

#define RC_VERSION COV_VERSION_MAJOR, COV_VERSION_MINOR, COV_VERSION_PATCH, 0
