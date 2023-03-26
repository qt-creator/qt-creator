// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/qtcassert.h>

#define QMT_CHECK(condition) QTC_CHECK(condition)
#define QMT_ASSERT(condition, action) QTC_ASSERT(condition, action)
// TODO implement logging of where and what
#define QMT_CHECK_X(condition, where, what) QMT_CHECK(condition)
