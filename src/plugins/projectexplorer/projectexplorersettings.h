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

#ifndef PROJECTEXPLORERSETTINGS_H
#define PROJECTEXPLORERSETTINGS_H

#include <QUuid>

namespace ProjectExplorer {
namespace Internal {

class ProjectExplorerSettings
{
public:
    ProjectExplorerSettings() :
        buildBeforeDeploy(true), deployBeforeRun(true),
        saveBeforeBuild(false), showCompilerOutput(false),
        showRunOutput(true), showDebugOutput(false),
        cleanOldAppOutput(false), mergeStdErrAndStdOut(false),
        wrapAppOutput(true), useJom(true),
        autorestoreLastSession(false), prompToStopRunControl(false),
        maxAppOutputLines(100000)
    { }

    bool buildBeforeDeploy;
    bool deployBeforeRun;
    bool saveBeforeBuild;
    bool showCompilerOutput;
    bool showRunOutput;
    bool showDebugOutput;
    bool cleanOldAppOutput;
    bool mergeStdErrAndStdOut;
    bool wrapAppOutput;
    bool useJom;
    bool autorestoreLastSession; // This option is set in the Session Manager!
    bool prompToStopRunControl;
    int  maxAppOutputLines;

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
            && p1.environmentId == p2.environmentId;
}

} // namespace ProjectExplorer
} // namespace Internal

#endif // PROJECTEXPLORERSETTINGS_H
