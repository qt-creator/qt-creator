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

#include <QVector>

namespace CppTools {

struct ProjectPartHeaderPath {
    enum Type {
        InvalidPath,
        IncludePath,
        FrameworkPath
    };

public:
    QString path;
    Type type;

    ProjectPartHeaderPath()
        : type(InvalidPath)
    {}

    ProjectPartHeaderPath(const QString &path, Type type)
        : path(path),
          type(type)
    {}

    bool isValid() const
    {
        return type != InvalidPath;
    }

    bool isFrameworkPath() const
    {
        return type == FrameworkPath;
    }

    bool operator==(const ProjectPartHeaderPath &other) const
    {
        return path == other.path
            && type == other.type;
    }

    bool operator!=(const ProjectPartHeaderPath &other) const
    {
        return !(*this == other);
    }
};

using ProjectPartHeaderPaths = QVector<ProjectPartHeaderPath>;

inline uint qHash(const ProjectPartHeaderPath &key, uint seed = 0)
{
    return ((qHash(key.path) << 2) | key.type) ^ seed;
}
} // namespace CppTools
