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

#include "googletest.h"

using testing::PrintToString;

namespace {

MATCHER_P4(IsSourceRange, startLine, startColumn, endLine, endColumn,
           std::string(negation ? "isn't" : "is")
           + "[(" + PrintToString(startLine)
           + ", " + PrintToString(startColumn)
           + "), (" + PrintToString(endLine)
           + ", " + PrintToString(endColumn)
           + ")]"
           )
{
    return arg.start().line() == uint(startLine)
        && arg.start().column() == uint(startColumn)
        && arg.end().line() == uint(endLine)
        && arg.end().column() == uint(endColumn);
}

MATCHER_P5(IsSourceRangeWithText, startLine, startColumn, endLine, endColumn, text,
           std::string(negation ? "isn't " : "is ")
           + "((" + PrintToString(startLine)
           + ", " + PrintToString(startColumn)
           + "), (" + PrintToString(endLine)
           + ", " + PrintToString(endColumn)
           + "), " + PrintToString(text)
           + ")"
           )
{
    return arg.start().line() == uint(startLine)
        && arg.start().column() == uint(startColumn)
        && arg.end().line() == uint(endLine)
        && arg.end().column() == uint(endColumn)
        && arg.text().toCarriageReturnsStripped() == text;

}

}
