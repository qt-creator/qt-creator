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

#include "clangbackendipc_global.h"

#include <utils/smallstringio.h>

#include <QDataStream>

namespace ClangBackEnd {

class FilePath
{
public:
    FilePath() = default;
    explicit FilePath(Utils::SmallString &&filePath)
    {
        auto foundReverse = std::find(filePath.rbegin(), filePath.rend(), '/');
        auto found = foundReverse.base();

        Utils::SmallString fileName(found, filePath.end());
        if (foundReverse != filePath.rend())
            filePath.resize(std::size_t(std::distance(filePath.begin(), --found)));

        directory_ = std::move(filePath);
        name_ = std::move(fileName);
    }

    explicit FilePath(const QString &filePath)
        : FilePath(Utils::SmallString(filePath))
    {
    }

    FilePath(Utils::SmallString &&directory, Utils::SmallString &&name)
        : directory_(std::move(directory)),
          name_(std::move(name))
    {}

    const Utils::SmallString &directory() const
    {
        return directory_;
    }

    Utils::SmallString takeDirectory()
    {
        return std::move(directory_);
    }

    const Utils::SmallString &name() const
    {
        return name_;
    }

    Utils::SmallString takeName()
    {
        return std::move(name_);
    }

    Utils::PathString path()  const
    {
        return {directory_, "/", name_};
    }

    friend QDataStream &operator<<(QDataStream &out, const FilePath &filePath)
    {
        out << filePath.directory_;
        out << filePath.name_;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, FilePath &filePath)
    {
        in >> filePath.directory_;
        in >> filePath.name_;

        return in;
    }

    friend std::ostream &operator<<(std::ostream &out, const FilePath &filePath)
    {
        out << filePath.directory() << "/" << filePath.name();

        return out;
    }

    friend bool operator==(const FilePath &first, const FilePath &second)
    {
        return first.name_ == second.name_
            && first.directory_ == second.directory_;
    }

    friend bool operator<(const FilePath &first, const FilePath &second)
    {
        return std::tie(first.name_, first.directory_)
             < std::tie(second.name_, second.directory_);
    }

    FilePath clone() const
    {
        return FilePath(directory_.clone(), name_.clone());
    }

private:
    Utils::SmallString directory_;
    Utils::SmallString name_;
};

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const FilePath &filePath);
void PrintTo(const FilePath &filePath, ::std::ostream* os);

} // namespace ClangBackEnd
