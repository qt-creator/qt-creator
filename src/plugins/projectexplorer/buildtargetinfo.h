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

#include <utils/algorithm.h>
#include <utils/fileutils.h>

#include <QList>
#include <QSet>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT BuildTargetInfo
{
public:
    BuildTargetInfo() = default;
    BuildTargetInfo(const QString &targetName, const Utils::FileName &targetFilePath,
                    const Utils::FileName &projectFilePath) :
        targetName(targetName),
        targetFilePath(targetFilePath),
        projectFilePath(projectFilePath)
    { }

    QString targetName;
    Utils::FileName targetFilePath;
    Utils::FileName projectFilePath;

    bool isValid() const { return !targetName.isEmpty(); }
};

inline bool operator==(const BuildTargetInfo &ti1, const BuildTargetInfo &ti2)
{
    return ti1.targetName == ti2.targetName && ti1.targetFilePath == ti2.targetFilePath
            && ti1.projectFilePath == ti2.projectFilePath;
}

inline bool operator!=(const BuildTargetInfo &ti1, const BuildTargetInfo &ti2)
{
    return !(ti1 == ti2);
}

inline uint qHash(const BuildTargetInfo &ti)
{
    return qHash(ti.targetName);
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

    bool hasTarget(const QString &targetName) {
        return Utils::anyOf(list, [&targetName](const BuildTargetInfo &ti) {
            return ti.targetName == targetName;
        });
    }

    Utils::FileName targetFilePath(const QString &targetName) {
        return Utils::findOrDefault(list, [&targetName](const BuildTargetInfo &ti) {
            return ti.targetName == targetName;
        }).targetFilePath;
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
