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

#include <utils/algorithm.h>
#include <utils/fileutils.h>

#include <QString>
#include <QVector>

namespace ProjectExplorer {

enum class HeaderPathType {
    User,
    BuiltIn,
    System,
    Framework,
};

class HeaderPath
{
public:
    HeaderPath() = default;
    HeaderPath(const QString &path, HeaderPathType type) : path(path), type(type) { }
    HeaderPath(const char *path, HeaderPathType type) : HeaderPath(QLatin1String(path), type) {}
    HeaderPath(const Utils::FilePath &path, HeaderPathType type)
        : HeaderPath(path.toString(), type) { }

    bool operator==(const HeaderPath &other) const
    {
        return type == other.type && path == other.path;
    }

    bool operator!=(const HeaderPath &other) const
    {
        return !(*this == other);
    }

    template<typename F> static HeaderPath makeUser(const F &fp)
    {
        return {fp, HeaderPathType::User};
    }
    template<typename F> static HeaderPath makeBuiltIn(const F &fp)
    {
        return {fp, HeaderPathType::BuiltIn};
    }
    template<typename F> static HeaderPath makeSystem(const F &fp)
    {
        return {fp, HeaderPathType::System};
    }
    template<typename F> static HeaderPath makeFramework(const F &fp)
    {
        return {fp, HeaderPathType::Framework};
    }

    friend auto qHash(const HeaderPath &key, uint seed = 0)
    {
        return ((qHash(key.path) << 2) | uint(key.type)) ^ seed;
    }

    QString path;
    HeaderPathType type = HeaderPathType::User;
};

using HeaderPaths = QVector<HeaderPath>;
template<typename C> HeaderPaths toHeaderPaths(const C &list, HeaderPathType type)
{
    return Utils::transform<HeaderPaths>(list, [type](const auto &fp) {
        return HeaderPath(fp, type);
    });
}
template<typename C> HeaderPaths toUserHeaderPaths(const C &list)
{
    return toHeaderPaths(list, HeaderPathType::User);
}
template<typename C> HeaderPaths toBuiltInHeaderPaths(const C &list)
{
    return toHeaderPaths(list, HeaderPathType::BuiltIn);
}
template<typename C> HeaderPaths toFrameworkHeaderPaths(const C &list)
{
    return toHeaderPaths(list, HeaderPathType::Framework);
}

} // namespace ProjectExplorer
