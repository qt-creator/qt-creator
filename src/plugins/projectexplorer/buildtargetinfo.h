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
#include <utils/environment.h>
#include <utils/fileutils.h>

#include <QList>
#include <QSet>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT BuildTargetInfo
{
public:
    QString buildKey; // Used to identify this BuildTargetInfo object in its list.
    QString displayName;

    Utils::FileName targetFilePath;
    Utils::FileName projectFilePath;
    Utils::FileName workingDirectory;
    bool isQtcRunnable = true;
    bool usesTerminal = false;

    std::function<void(Utils::Environment &, bool)> runEnvModifier;
};

inline bool operator==(const BuildTargetInfo &ti1, const BuildTargetInfo &ti2)
{
    return ti1.buildKey == ti2.buildKey
        && ti1.displayName == ti2.displayName
        && ti1.targetFilePath == ti2.targetFilePath
        && ti1.projectFilePath == ti2.projectFilePath;
}

inline bool operator!=(const BuildTargetInfo &ti1, const BuildTargetInfo &ti2)
{
    return !(ti1 == ti2);
}

inline uint qHash(const BuildTargetInfo &ti)
{
    return qHash(ti.displayName) ^ qHash(ti.buildKey);
}

class PROJECTEXPLORER_EXPORT BuildTargetInfoList
{
public:
    BuildTargetInfo buildTargetInfo(const QString &buildKey) {
        return Utils::findOrDefault(list, [&buildKey](const BuildTargetInfo &ti) {
            return ti.buildKey == buildKey;
        });
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
