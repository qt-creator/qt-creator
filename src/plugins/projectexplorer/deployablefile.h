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

#include "projectexplorer_export.h"

#include <utils/fileutils.h>

#include <QString>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT DeployableFile
{
public:
    enum Type
    {
        TypeNormal,
        TypeExecutable
    };

    DeployableFile() = default;
    DeployableFile(const QString &m_localFilePath, const QString &m_remoteDir,
                   Type type = TypeNormal);
    DeployableFile(const Utils::FileName &localFilePath, const QString &remoteDir,
                   Type type = TypeNormal);

    Utils::FileName localFilePath() const { return m_localFilePath; }
    QString remoteDirectory() const { return m_remoteDir; }
    QString remoteFilePath() const;

    bool isValid() const;

    bool isExecutable() const;

private:
    Utils::FileName m_localFilePath;
    QString m_remoteDir;
    Type m_type = TypeNormal;
};


inline bool operator==(const DeployableFile &d1, const DeployableFile &d2)
{
    return d1.localFilePath() == d2.localFilePath() && d1.remoteDirectory() == d2.remoteDirectory();
}

inline bool operator!=(const DeployableFile &d1, const DeployableFile &d2)
{
    return !(d1 == d2);
}

PROJECTEXPLORER_EXPORT uint qHash(const DeployableFile &d);

} // namespace ProjectExplorer
