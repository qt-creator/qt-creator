/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <utils/smallstringio.h>

#include <vector>

namespace ClangBackEnd {

enum class CompilerMacroType : unsigned char { Define, NotDefined };

class CompilerMacro
{
public:
    constexpr CompilerMacro() = default;

    CompilerMacro(Utils::SmallString &&key, Utils::SmallString &&value, int index)
        : key(std::move(key))
        , value(std::move(value))
        , index(index)
        , type(CompilerMacroType::Define)
    {}

    CompilerMacro(Utils::SmallString &&key)
        : key(std::move(key))
    {}

    CompilerMacro(const Utils::SmallString &key)
        : key(key)
    {}

    friend QDataStream &operator<<(QDataStream &out, const CompilerMacro &compilerMacro)
    {
        out << compilerMacro.key;
        out << compilerMacro.value;
        out << compilerMacro.index;
        out << static_cast<unsigned char>(compilerMacro.type);

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, CompilerMacro &compilerMacro)
    {
        unsigned char type;

        in >> compilerMacro.key;
        in >> compilerMacro.value;
        in >> compilerMacro.index;
        in >> type;

        compilerMacro.type = static_cast<CompilerMacroType>(type);

        return in;
    }

    friend bool operator==(const CompilerMacro &first, const CompilerMacro &second)
    {
        return first.key == second.key && first.value == second.value && first.type == second.type;
    }

    friend bool operator<(const CompilerMacro &first, const CompilerMacro &second)
    {
        return std::tie(first.key, first.type, first.value)
               < std::tie(second.key, second.type, second.value);
    }

public:
    Utils::SmallString key;
    Utils::SmallString value;
    int index = -1;
    CompilerMacroType type = CompilerMacroType::NotDefined;
};

using CompilerMacros = std::vector<CompilerMacro>;
}
