/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#ifndef BUILDTARGETINFO_H
#define BUILDTARGETINFO_H

#include "projectexplorer_export.h"

#include <utils/fileutils.h>

#include <QList>
#include <QSet>

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

inline bool operator==(const BuildTargetInfo &ti1, const BuildTargetInfo &ti2)
{
    return ti1.targetFilePath == ti2.targetFilePath;
}

inline bool operator!=(const BuildTargetInfo &ti1, const BuildTargetInfo &ti2)
{
    return !(ti1 == ti2);
}

inline uint qHash(const BuildTargetInfo &ti)
{
    return qHash(ti.targetFilePath);
}


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

inline bool operator==(const BuildTargetInfoList &til1, const BuildTargetInfoList &til2)
{
    return til1.list.toSet() == til2.list.toSet();
}

inline bool operator!=(const BuildTargetInfoList &til1, const BuildTargetInfoList &til2)
{
    return !(til1 == til2);
}

} // namespace ProjectExplorer

#endif // BUILDTARGETINFO_H
