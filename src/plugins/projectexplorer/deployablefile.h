/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef DEPLOYABLEFILE_H
#define DEPLOYABLEFILE_H

#include "projectexplorer_export.h"

#include <utils/fileutils.h>

#include <QString>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT DeployableFile
{
public:
    DeployableFile();
    DeployableFile(const QString &m_localFilePath, const QString &m_remoteDir);
    DeployableFile(const Utils::FileName &localFilePath, const QString &remoteDir);

    Utils::FileName localFilePath() const { return m_localFilePath; }
    QString remoteDirectory() const { return m_remoteDir; }
    QString remoteFilePath() const;

    bool isValid() const;

private:
    Utils::FileName m_localFilePath;
    QString m_remoteDir;
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

#endif // DEPLOYABLEFILE_H
