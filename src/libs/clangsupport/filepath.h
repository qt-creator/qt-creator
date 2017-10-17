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

#include "clangsupport_global.h"

#include <utils/smallstringio.h>

#include <QDataStream>

namespace ClangBackEnd {

class FilePath
{
public:
    FilePath() = default;
    explicit FilePath(Utils::PathString &&filePath)
        : m_path(std::move(filePath))
    {
        auto foundReverse = std::find(m_path.rbegin(), m_path.rend(), '/');
        auto found = foundReverse.base();
        --found;

        m_slashIndex = std::size_t(std::distance(m_path.begin(), found));
    }

    explicit FilePath(const Utils::PathString &filePath)
        : FilePath(filePath.clone())
    {
    }

    explicit FilePath(Utils::PathString &&filePath, std::size_t slashIndex)
        : m_path(std::move(filePath)),
          m_slashIndex(slashIndex)
    {
    }

    explicit FilePath(const QString &filePath)
        : FilePath(Utils::PathString(filePath))
    {
    }

    FilePath(Utils::SmallStringView directory, Utils::SmallStringView name)
        : m_path({directory, "/", name}),
          m_slashIndex(directory.size())
    {}

    Utils::SmallStringView directory() const noexcept
    {
        return m_path.mid(0, m_slashIndex);
    }

    Utils::SmallStringView name() const noexcept
    {
        return m_path.mid(m_slashIndex + 1, m_path.size() - m_slashIndex - 1);
    }

    const Utils::PathString &path()  const noexcept
    {
        return m_path;
    }

    operator Utils::PathString() const noexcept
    {
        return m_path;
    }

    friend QDataStream &operator<<(QDataStream &out, const FilePath &filePath)
    {
        out << filePath.m_path;
        out << uint(filePath.m_slashIndex);

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, FilePath &filePath)
    {
        uint slashIndex;

        in >> filePath.m_path;
        in >> slashIndex;

        filePath.m_slashIndex = slashIndex;

        return in;
    }

    friend std::ostream &operator<<(std::ostream &out, const FilePath &filePath)
    {
        return out << "(" << filePath.path() << ", " << filePath.slashIndex() << ")";
    }

    friend bool operator==(const FilePath &first, const FilePath &second)
    {
        return first.m_path == second.m_path;
    }

    friend bool operator==(const FilePath &first, const Utils::SmallStringView &second)
    {
        return first.path() == second;
    }

    friend bool operator==(const Utils::SmallStringView &first, const FilePath&second)
    {
        return second == first;
    }

    friend bool operator<(const FilePath &first, const FilePath &second)
    {
        return first.m_path < second.m_path;
    }

    FilePath clone() const
    {
        return *this;
    }

    std::size_t slashIndex() const
    {
        return m_slashIndex;
    }

private:
    Utils::PathString m_path = "/";
    std::size_t m_slashIndex = 0;
};

using FilePaths = std::vector<FilePath>;

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const FilePath &filePath);

} // namespace ClangBackEnd
