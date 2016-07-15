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

#include <QUuid>

namespace ProjectExplorer {
namespace Internal {

class ProjectExplorerSettings
{
public:
    enum StopBeforeBuild { StopNone = 0, StopSameProject, StopAll, StopSameBuildDir };

    bool buildBeforeDeploy = true;
    bool deployBeforeRun = true;
    bool saveBeforeBuild = false;
    bool showCompilerOutput = false;
    bool showRunOutput = true;
    bool showDebugOutput = false;
    bool cleanOldAppOutput = false;
    bool mergeStdErrAndStdOut = false;
    bool wrapAppOutput = true;
    bool useJom = true;
    bool autorestoreLastSession = false; // This option is set in the Session Manager!
    bool prompToStopRunControl = false;
    int  maxAppOutputLines = 100000;
    StopBeforeBuild stopBeforeBuild = StopBeforeBuild::StopNone;

    // Add a UUid which is used to identify the development environment.
    // This is used to warn the user when he is trying to open a .user file that was created
    // somewhere else (which might lead to unexpected results).
    QUuid environmentId;
};

inline bool operator==(const ProjectExplorerSettings &p1, const ProjectExplorerSettings &p2)
{
    return p1.buildBeforeDeploy == p2.buildBeforeDeploy
            && p1.deployBeforeRun == p2.deployBeforeRun
            && p1.saveBeforeBuild == p2.saveBeforeBuild
            && p1.showCompilerOutput == p2.showCompilerOutput
            && p1.showRunOutput == p2.showRunOutput
            && p1.showDebugOutput == p2.showDebugOutput
            && p1.cleanOldAppOutput == p2.cleanOldAppOutput
            && p1.mergeStdErrAndStdOut == p2.mergeStdErrAndStdOut
            && p1.wrapAppOutput == p2.wrapAppOutput
            && p1.useJom == p2.useJom
            && p1.autorestoreLastSession == p2.autorestoreLastSession
            && p1.prompToStopRunControl == p2.prompToStopRunControl
            && p1.maxAppOutputLines == p2.maxAppOutputLines
            && p1.environmentId == p2.environmentId
            && p1.stopBeforeBuild == p2.stopBeforeBuild;
}

} // namespace ProjectExplorer
} // namespace Internal
