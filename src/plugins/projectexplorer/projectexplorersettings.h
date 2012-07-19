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

#ifndef PROJECTEXPLORERSETTINGS_H
#define PROJECTEXPLORERSETTINGS_H

#include <QUuid>

namespace ProjectExplorer {
namespace Internal {

struct ProjectExplorerSettings
{
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
            && p1.maxAppOutputLines == p2.maxAppOutputLines;
}

} // namespace ProjectExplorer
} // namespace Internal

#endif // PROJECTEXPLORERSETTINGS_H
