/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "directorypathid.h"
#include "filepathid.h"

#include <QDataStream>

#include <vector>

namespace ClangBackEnd {
class DirectoryAndFilePathId
{
public:
    constexpr DirectoryAndFilePathId() = default;

    DirectoryAndFilePathId(const char *) = delete;

    DirectoryAndFilePathId(int directoryPathId, int filePathId)
        : directoryPathId(directoryPathId)
        , filePathId(filePathId)
    {}

    bool isValid() const { return directoryPathId.isValid() && filePathId.isValid(); }

    friend bool operator==(DirectoryAndFilePathId first, DirectoryAndFilePathId second)
    {
        return first.isValid() && first.directoryPathId == second.directoryPathId
               && first.filePathId == second.filePathId;
    }

    friend bool operator!=(DirectoryAndFilePathId first, DirectoryAndFilePathId second)
    {
        return !(first == second);
    }

    friend bool operator<(DirectoryAndFilePathId first, DirectoryAndFilePathId second)
    {
        return std::tie(first.directoryPathId, first.filePathId)
               < std::tie(second.directoryPathId, second.filePathId);
    }

    friend QDataStream &operator<<(QDataStream &out,
                                   const DirectoryAndFilePathId &directoryAndFilePathId)
    {
        out << directoryAndFilePathId.directoryPathId;
        out << directoryAndFilePathId.filePathId;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, DirectoryAndFilePathId &directoryAndFilePathId)
    {
        in >> directoryAndFilePathId.directoryPathId;
        in >> directoryAndFilePathId.filePathId;
        return in;
    }

public:
    DirectoryPathId directoryPathId;
    FilePathId filePathId;
};

using DirectoryAndFilePathIds = std::vector<DirectoryAndFilePathId>;

} // namespace ClangBackEnd
