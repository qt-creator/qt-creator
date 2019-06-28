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

#include <QDataStream>

#include <vector>

namespace ClangBackEnd {
class DirectoryPathId
{
public:
    constexpr DirectoryPathId() = default;

    DirectoryPathId(const char *) = delete;

    DirectoryPathId(int directoryPathId)
        : directoryPathId(directoryPathId)
    {}

    bool isValid() const { return directoryPathId >= 0; }

    friend bool operator==(DirectoryPathId first, DirectoryPathId second)
    {
        return first.isValid() && first.directoryPathId == second.directoryPathId;
    }

    friend bool operator!=(DirectoryPathId first, DirectoryPathId second)
    {
        return !(first == second);
    }

    friend bool operator<(DirectoryPathId first, DirectoryPathId second)
    {
        return first.directoryPathId < second.directoryPathId;
    }

    friend QDataStream &operator<<(QDataStream &out, const DirectoryPathId &directoryPathId)
    {
        out << directoryPathId.directoryPathId;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, DirectoryPathId &directoryPathId)
    {
        in >> directoryPathId.directoryPathId;

        return in;
    }

public:
    int directoryPathId = -1;
};

using DirectoryPathIds = std::vector<DirectoryPathId>;

} // namespace ClangBackEnd
