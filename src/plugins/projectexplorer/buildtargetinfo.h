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
#ifndef BUILDTARGETINFO_H
#define BUILDTARGETINFO_H

#include "projectexplorer_export.h"

#include <utils/fileutils.h>

#include <QList>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT BuildTargetInfo
{
public:
    BuildTargetInfo() {}
    BuildTargetInfo(const Utils::FileName &targetFilePath, const Utils::FileName &projectFilePath)
        : targetFilePath(targetFilePath), projectFilePath(projectFilePath)
    {
    }

    BuildTargetInfo(const QString &targetFilePath, const QString &projectFilePath)
        : targetFilePath(Utils::FileName::fromUserInput(targetFilePath)),
          projectFilePath(Utils::FileName::fromUserInput(projectFilePath))
    {
    }

    Utils::FileName targetFilePath;
    Utils::FileName projectFilePath;

    bool isValid() const { return !targetFilePath.isEmpty(); }
};

class PROJECTEXPLORER_EXPORT BuildTargetInfoList
{
public:
    Utils::FileName targetForProject(const QString &projectFilePath) const
    {
        return targetForProject(Utils::FileName::fromString(projectFilePath));
    }

    Utils::FileName targetForProject(const Utils::FileName &projectFilePath) const
    {
        foreach (const BuildTargetInfo &ti, list) {
            if (ti.projectFilePath == projectFilePath)
                return ti.targetFilePath;
        }
        return Utils::FileName();
    }

    QList<BuildTargetInfo> list;
};

} // namespace ProjectExplorer

#endif // BUILDTARGETINFO_H
