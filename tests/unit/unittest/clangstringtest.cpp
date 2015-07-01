/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <clangstring.h>

#include <utf8string.h>

#include <clang-c/CXString.h>
#include <clang-c/Index.h>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

namespace {

using ClangBackEnd::ClangString;

TEST(ClangString, ConvertToUtf8String)
{
    const CXString cxString = { "text", 0};

    ASSERT_THAT(Utf8String(ClangString(cxString)), Utf8StringLiteral("text"));
}

TEST(ClangString, ConvertNullStringToUtf8String)
{
    const CXString cxString = { 0, 0};

    ASSERT_THAT(Utf8String(ClangString(cxString)), Utf8String());
}

TEST(ClangString, MoveClangString)
{
    ClangString text(CXString{ "text", 0});

    const ClangString text2 = std::move(text);

    ASSERT_TRUE(text.isNull());
    ASSERT_FALSE(text2.isNull());
}

}
