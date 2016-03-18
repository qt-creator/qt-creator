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

#include <iostream>

#define CPP_ASSERT_STRINGIFY_HELPER(x) #x
#define CPP_ASSERT_STRINGIFY(x) CPP_ASSERT_STRINGIFY_HELPER(x)
#define CPP_ASSERT_STRING(cond) std::cerr \
    << "SOFT ASSERT: \"" cond"\" in file " __FILE__ ", line " CPP_ASSERT_STRINGIFY(__LINE__) \
    << std::endl;

#define CPP_ASSERT(cond, action) if (cond) {} else { CPP_ASSERT_STRING(#cond); action; } do {} while (0)
#define CPP_CHECK(cond) if (cond) {} else { CPP_ASSERT_STRING(#cond); } do {} while (0)
