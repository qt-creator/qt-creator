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

#include "gtest-creator-printing.h"

#include <gtest/gtest-printers.h>

#include <coreplugin/find/searchresultitem.h>

#include <projectexplorer/projectmacro.h>

namespace Core {
namespace Search {

using testing::PrintToString;

class TextPosition;
class TextRange;

void PrintTo(const TextPosition &position, ::std::ostream *os)
{
    *os << "("
        << position.line << ", "
        << position.column << ", "
        << position.offset << ")";
}

void PrintTo(const TextRange &range, ::std::ostream *os)
{
    *os << "("
        << PrintToString(range.begin) << ", "
        << PrintToString(range.end) << ")";
}

}
}

namespace ProjectExplorer {

static const char *typeToString(const MacroType &type)
{
    switch (type) {
        case MacroType::Invalid: return "MacroType::Invalid";
        case MacroType::Define: return "MacroType::Define";
        case MacroType::Undefine: return  "MacroType::Undefine";
    }

    return "";
}

std::ostream &operator<<(std::ostream &out, const MacroType &type)
{
    out << typeToString(type);

    return out;
}

std::ostream &operator<<(std::ostream &out, const Macro &macro)
{
    out << "("
        << macro.key.data() << ", "
        << macro.value.data() << ", "
        << macro.type << ")";

  return out;
}

}
