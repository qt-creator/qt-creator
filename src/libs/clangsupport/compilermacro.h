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

class CompilerMacro
{
public:
    constexpr CompilerMacro() = default;

    CompilerMacro(Utils::SmallString &&key,
                  Utils::SmallString &&value)
        : key(std::move(key)),
          value(std::move(value))
    {}

    friend QDataStream &operator<<(QDataStream &out, const CompilerMacro &compilerMacro)
    {
        out << compilerMacro.key;
        out << compilerMacro.value;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, CompilerMacro &compilerMacro)
    {
        in >> compilerMacro.key;
        in >> compilerMacro.value;

        return in;
    }

    friend bool operator==(const CompilerMacro &first, const CompilerMacro &second)
    {
        return first.key == second.key
            && first.value == second.value;
    }

    friend bool operator<(const CompilerMacro &first, const CompilerMacro &second)
    {
        return std::tie(first.key, first.value) < std::tie(second.key, second.value);
    }

public:
    Utils::SmallString key;
    Utils::SmallString value;
};

using CompilerMacros = std::vector<CompilerMacro>;
}
