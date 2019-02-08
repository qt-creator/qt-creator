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

#include "filepathview.h"
#include "filepath.h"

#include <utils/hostosinfo.h>
#include <utils/smallstringio.h>

namespace ClangBackEnd {

class FilePath;

class NativeFilePath
{
    using size_type = Utils::PathString::size_type;

public:
    NativeFilePath() = default;
    explicit NativeFilePath(Utils::PathString &&filePath)
        : m_path(std::move(filePath))
    {
        NativeFilePathView view{m_path};

        m_slashIndex = view.slashIndex();
    }

    NativeFilePath(NativeFilePathView filePathView)
        : m_path(filePathView.toStringView()),
          m_slashIndex(filePathView.slashIndex())
    {}

    explicit NativeFilePath(FilePathView filePathView) { *this = fromFilePath(filePathView); }

    explicit NativeFilePath(const FilePath &filePath)
        : NativeFilePath{FilePathView{filePath}}
    {}

    template<size_type Size>
    NativeFilePath(const char(&string)[Size]) noexcept
        : NativeFilePath(NativeFilePathView(string, Size - 1))
    {
        static_assert(Size >= 1, "Invalid string literal! Length is zero!");
    }

    explicit NativeFilePath(const Utils::PathString &filePath)
        : NativeFilePath(filePath.clone())
    {
    }

    explicit NativeFilePath(Utils::PathString &&filePath, std::ptrdiff_t slashIndex)
        : m_path(std::move(filePath)),
          m_slashIndex(slashIndex)
    {
    }

    explicit NativeFilePath(const QString &filePath)
        : NativeFilePath(Utils::PathString(filePath))
    {
    }

    NativeFilePath(Utils::SmallStringView directory, Utils::SmallStringView name)
        : m_path({directory, Utils::HostOsInfo::isWindowsHost() ? "\\" : "/", name}),
          m_slashIndex(directory.size())
    {}

    Utils::SmallStringView directory() const noexcept
    {
        return m_path.mid(0, std::size_t(std::max(std::ptrdiff_t(0), m_slashIndex)));    }

    Utils::SmallStringView name() const noexcept
    {
        return m_path.mid(std::size_t(m_slashIndex + 1),
                          std::size_t(std::ptrdiff_t(m_path.size()) - m_slashIndex - std::ptrdiff_t(1)));
    }

    bool empty() const
    {
        return m_path.empty();
    }

    const Utils::PathString &path()  const noexcept
    {
        return m_path;
    }

    operator const Utils::PathString&() const noexcept
    {
        return m_path;
    }

    operator NativeFilePathView() const noexcept
    {
        return NativeFilePathView(Utils::SmallStringView(m_path));
    }

    friend QDataStream &operator<<(QDataStream &out, const NativeFilePath &filePath)
    {
        out << filePath.m_path;
        out << uint(filePath.m_slashIndex);

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, NativeFilePath &filePath)
    {
        uint slashIndex;

        in >> filePath.m_path;
        in >> slashIndex;

        filePath.m_slashIndex = slashIndex;

        return in;
    }

    friend bool operator==(const NativeFilePath &first, const NativeFilePath &second)
    {
        return first.m_path == second.m_path;
    }

    friend bool operator==(const NativeFilePath &first, const NativeFilePathView &second)
    {
        return first.path() == second.toStringView();
    }

    friend bool operator==(const NativeFilePathView &first, const NativeFilePath &second)
    {
        return second == first;
    }

    friend bool operator<(const NativeFilePath &first, const NativeFilePath &second)
    {
        return first.m_path < second.m_path;
    }

    NativeFilePath clone() const
    {
        return *this;
    }

    std::ptrdiff_t slashIndex() const
    {
        return m_slashIndex;
    }

    template<typename String>
    static NativeFilePath fromFilePath(String filePath)
    {
        Utils::PathString nativePath{filePath.data(), filePath.size()};

        if (Utils::HostOsInfo::isWindowsHost())
            nativePath.replace('/', '\\');

        return NativeFilePath(std::move(nativePath));
    }

private:
    Utils::PathString m_path;
    std::ptrdiff_t m_slashIndex = -1;
};

using NativeFilePaths = std::vector<NativeFilePath>;
} // namespace ClangBackEnd

