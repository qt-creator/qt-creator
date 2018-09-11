/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include <gtest/gtest-printers.h>

#include "compare-operators.h"

#include "conditionally-disabled-tests.h"
#include "gtest-qt-printing.h"
#include "gtest-creator-printing.h"
#ifdef CLANG_UNIT_TESTS
#  include "gtest-clang-printing.h"
#endif

#include "google-using-declarations.h"

#include "unittest-matchers.h"

#include "unittest-utility-functions.h"
