/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <QDataStream>

#include <vector>

namespace ClangBackEnd {

enum class IncludeSearchPathType : unsigned char {
    Invalid,
    User,
    BuiltIn,
    System,
    Framework,
};

class IncludeSearchPath
{
public:
    IncludeSearchPath() = default;
    IncludeSearchPath(Utils::PathString &&path, int index, IncludeSearchPathType type)
        : path(std::move(path))
        , index(index)
        , type(type)
    {}

    IncludeSearchPath(Utils::PathString &&path, int index, int type)
        : path(std::move(path))
        , index(index)
        , type(static_cast<IncludeSearchPathType>(type))
    {}

    friend QDataStream &operator<<(QDataStream &out, const IncludeSearchPath &includeSearchPath)
    {
        out << includeSearchPath.path;
        out << includeSearchPath.index;
        out << static_cast<unsigned char>(includeSearchPath.type);

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, IncludeSearchPath &includeSearchPath)
    {
        unsigned char type;

        in >> includeSearchPath.path;
        in >> includeSearchPath.index;
        in >> type;

        includeSearchPath.type = static_cast<IncludeSearchPathType>(type);

        return in;
    }

    friend bool operator==(const IncludeSearchPath &first, const IncludeSearchPath &second)
    {
        return std::tie(first.type, first.index, first.path)
               == std::tie(second.type, second.index, second.path);
    }

    friend bool operator<(const IncludeSearchPath &first, const IncludeSearchPath &second)
    {
        return std::tie(first.path, first.index, first.type)
               < std::tie(second.path, second.index, second.type);
    }

public:
    Utils::PathString path;
    int index = -1;
    IncludeSearchPathType type = IncludeSearchPathType::Invalid;
};

using IncludeSearchPaths = std::vector<IncludeSearchPath>;

} // namespace ClangBackEnd
