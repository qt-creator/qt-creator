/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "sourcepathview.h"

#include <utils/smallstring.h>

namespace QmlDesigner {

class SourcePath : public Utils::PathString
{
    using size_type = Utils::PathString::size_type;

public:
    SourcePath() = default;
    explicit SourcePath(Utils::PathString &&sourcePath)
        : Utils::PathString(std::move(sourcePath))
    {
        SourcePathView view{*this};

        m_slashIndex = view.slashIndex();
    }

    explicit SourcePath(Utils::SmallStringView &&sourcePath)
        : Utils::PathString(sourcePath)
    {
        SourcePathView view{*this};

        m_slashIndex = view.slashIndex();
    }

    SourcePath(SourcePathView sourcePathView)
        : Utils::PathString(sourcePathView.toStringView())
        , m_slashIndex(sourcePathView.slashIndex())
    {
    }

    template<size_type Size>
    SourcePath(const char (&string)[Size]) noexcept
        : SourcePath(SourcePathView(string, Size - 1))
    {
        static_assert(Size >= 1, "Invalid string literal! Length is zero!");
    }

    explicit SourcePath(const Utils::PathString &sourcePath)
        : SourcePath(sourcePath.clone())
    {
    }

    explicit SourcePath(Utils::PathString &&sourcePath, std::ptrdiff_t slashIndex)
        : Utils::PathString(std::move(sourcePath))
        , m_slashIndex(slashIndex)
    {
    }

    explicit SourcePath(const QString &sourcePath)
        : SourcePath(Utils::PathString(sourcePath))
    {
    }

    SourcePath(Utils::SmallStringView directory, Utils::SmallStringView name)
        : Utils::PathString(Utils::PathString::join({directory, "/", name}))
        , m_slashIndex(std::ptrdiff_t(directory.size()))
    {}

    bool isValid() const { return size() > 0 && m_slashIndex >= 0; }

    Utils::SmallStringView directory() const noexcept
    {
        return mid(0, std::size_t(std::max(std::ptrdiff_t(0), m_slashIndex)));
    }

    Utils::SmallStringView name() const noexcept
    {
        return mid(std::size_t(m_slashIndex + 1),
                   std::size_t(std::ptrdiff_t(size()) - m_slashIndex - std::ptrdiff_t(1)));
    }

    const Utils::PathString &path()  const noexcept
    {
        return *this;
    }

    operator SourcePathView() const noexcept { return SourcePathView(toStringView()); }

    operator Utils::SmallStringView() const noexcept
    {
        return toStringView();
    }

    friend bool operator==(const SourcePath &first, const SourcePath &second)
    {
        return first.slashIndex() == second.slashIndex()
            && first.name() == second.name()
            && first.directory() == second.directory();
    }

    friend bool operator==(const SourcePath &first, const SourcePathView &second)
    {
        return first.toStringView() == second.toStringView();
    }

    friend bool operator==(const SourcePathView &first, const SourcePath &second)
    {
        return second == first;
    }

    friend bool operator<(const SourcePath &first, const SourcePath &second)
    {
        return std::make_tuple(first.slashIndex(), first.name(), first.directory())
             < std::make_tuple(second.slashIndex(), second.name(), second.directory());
    }

    SourcePath clone() const { return *this; }

    std::ptrdiff_t slashIndex() const { return m_slashIndex; }

private:
    std::ptrdiff_t m_slashIndex = -1;
};

using SourcePaths = std::vector<SourcePath>;

} // namespace QmlDesigner
