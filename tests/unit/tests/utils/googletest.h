// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#ifdef __GNUC__
#  pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

#include "conditionally-disabled-tests.h"

#include "../printers/gtest-creator-printing.h"
#include "../printers/gtest-qt-printing.h"
#include "../printers/gtest-std-printing.h"

#include "../utils/google-using-declarations.h"

#include "../matchers/unittest-matchers.h"

#include "unittest-utility-functions.h"

#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest-printers.h>
#include <gtest/gtest-typed-test.h>
#include <gtest/gtest.h>

QT_WARNING_DISABLE_CLANG("-Wpadded")
