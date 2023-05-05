// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include <gtest/gtest-printers.h>
#include <gtest/gtest-typed-test.h>

#include "conditionally-disabled-tests.h"
#include "gtest-creator-printing.h"
#include "gtest-llvm-printing.h"
#include "gtest-qt-printing.h"
#include "gtest-std-printing.h"

#include "google-using-declarations.h"

#include "unittest-matchers.h"

#include "unittest-utility-functions.h"
