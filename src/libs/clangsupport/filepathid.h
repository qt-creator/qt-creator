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

#include "clangsupport_global.h"

#include <QDataStream>

#include <cstdint>
#include <tuple>
#include <vector>

namespace ClangBackEnd {

class FilePathId
{
public:
    constexpr FilePathId() = default;

    FilePathId(int filePathId)
        : filePathId(filePathId)
    {}

    bool isValid() const
    {
        return filePathId >= 0;
    }

    friend bool operator==(FilePathId first, FilePathId second)
    {
        return first.isValid() && first.filePathId == second.filePathId;
    }

    friend bool operator!=(FilePathId first, FilePathId second)
    {
        return !(first==second);
    }

    friend bool operator<(FilePathId first, FilePathId second)
    {
        return first.filePathId < second.filePathId;
    }

    friend QDataStream &operator<<(QDataStream &out, const FilePathId &filePathId)
    {
        out << filePathId.filePathId;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, FilePathId &filePathId)
    {
        in >> filePathId.filePathId;

        return in;
    }

public:
    int filePathId = -1;
};

using FilePathIds = std::vector<FilePathId>;

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const FilePathId &filePathId);
} // namespace ClangBackEnd

namespace std {
template<> struct hash<ClangBackEnd::FilePathId>
{
    using argument_type = ClangBackEnd::FilePathId;
    using result_type = std::size_t;
    result_type operator()(const argument_type& filePathId) const
    {
        return std::hash<int>{}(filePathId.filePathId);
    }
};

} // namespace std
