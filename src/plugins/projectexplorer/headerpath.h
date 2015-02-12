/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef HEADERPATH_H
#define HEADERPATH_H

#include <QString>

#include "projectexplorer_export.h"

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT HeaderPath
{
public:
    enum Kind {
        GlobalHeaderPath,
        UserHeaderPath,
        FrameworkHeaderPath
    };

    HeaderPath()
        : m_kind(GlobalHeaderPath)
    { }

    HeaderPath(const QString &path, Kind kind)
        : m_path(path), m_kind(kind)
    { }

    QString path() const { return m_path; }
    Kind kind() const { return m_kind; }

    bool operator==(const HeaderPath &other) const
    {
        return m_kind == other.m_kind
                && m_path == other.m_path;
    }

private:
    QString m_path;
    Kind m_kind;
};

} // namespace ProjectExplorer

#endif // HEADERPATH_H
