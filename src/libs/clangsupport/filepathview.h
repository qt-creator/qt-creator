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

#include <utils/smallstringview.h>

#include <algorithm>

namespace ClangBackEnd {
class FilePathView : protected Utils::SmallStringView
{
public:
    explicit FilePathView(const char *const string, const size_type size) noexcept
        : Utils::SmallStringView(string, size),
          m_slashIndex(lastSlashIndex(*this))
    {
    }

    explicit FilePathView(Utils::SmallStringView filePath)
        : Utils::SmallStringView(filePath),
          m_slashIndex(lastSlashIndex(filePath))
    {
    }

    template <typename String>
    explicit FilePathView(String filePath)
        : FilePathView(filePath.data(), filePath.size())
    {
    }

    template<size_type Size>
    FilePathView(const char(&string)[Size]) noexcept
        : FilePathView(string, Size - 1)
    {
        static_assert(Size >= 1, "Invalid string literal! Length is zero!");
    }

    Utils::SmallStringView toStringView() const
    {
        return *this;
    }

    std::ptrdiff_t slashIndex() const
    {
        return m_slashIndex;
    }

    Utils::SmallStringView directory() const noexcept
    {
        return mid(0, std::size_t(std::max(std::ptrdiff_t(0), m_slashIndex)));
    }

    Utils::SmallStringView name() const noexcept
    {
        return mid(std::size_t(m_slashIndex + 1),
                   std::size_t(std::ptrdiff_t(size()) - m_slashIndex - std::ptrdiff_t(1)));
    }

    static
    std::ptrdiff_t lastSlashIndex(Utils::SmallStringView filePath)
    {
        auto foundReverse = std::find(filePath.rbegin(), filePath.rend(), '/');
        auto found = foundReverse.base();
        --found;

        return std::distance(filePath.begin(), found);
    }

    friend bool operator==(const FilePathView &first, const FilePathView &second)
    {
        return first.toStringView() == second.toStringView();
    }

private:
    std::ptrdiff_t m_slashIndex = -1;
};

}
