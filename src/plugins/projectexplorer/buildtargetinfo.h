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

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT BuildTargetInfo
{
public:
    QString buildKey; // Used to identify this BuildTargetInfo object in its list.
    QString displayName;
    QString displayNameUniquifier;

    Utils::FilePath targetFilePath;
    Utils::FilePath projectFilePath;
    Utils::FilePath workingDirectory;
    bool isQtcRunnable = true;
    bool usesTerminal = false;

    uint runEnvModifierHash = 0; // Make sure to update this when runEnvModifier changes!

    std::function<void(Utils::Environment &, bool)> runEnvModifier;
};

inline bool operator==(const BuildTargetInfo &ti1, const BuildTargetInfo &ti2)
{
    return ti1.buildKey == ti2.buildKey
        && ti1.displayName == ti2.displayName
        && ti1.targetFilePath == ti2.targetFilePath
        && ti1.projectFilePath == ti2.projectFilePath
        && ti1.workingDirectory == ti2.workingDirectory
        && ti1.isQtcRunnable == ti2.isQtcRunnable
        && ti1.usesTerminal == ti2.usesTerminal
        && ti1.runEnvModifierHash == ti2.runEnvModifierHash;
}

inline bool operator!=(const BuildTargetInfo &ti1, const BuildTargetInfo &ti2)
{
    return !(ti1 == ti2);
}

inline uint qHash(const BuildTargetInfo &ti)
{
    return qHash(ti.displayName) ^ qHash(ti.buildKey);
}

} // namespace ProjectExplorer
