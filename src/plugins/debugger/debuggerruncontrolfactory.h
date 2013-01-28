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

#ifndef DEBUGGERRUNCONTROLFACTORY_H
#define DEBUGGERRUNCONTROLFACTORY_H

#include <projectexplorer/runconfiguration.h>

namespace Debugger {

class DebuggerEngine;
class DebuggerRunControl;
class DebuggerStartParameters;

namespace Internal {

class DebuggerRunControlFactory
    : public ProjectExplorer::IRunControlFactory
{
public:
    explicit DebuggerRunControlFactory(QObject *parent);

    // FIXME: Used by qmljsinspector.cpp:469
    ProjectExplorer::RunControl *create(
        ProjectExplorer::RunConfiguration *runConfiguration,
        ProjectExplorer::RunMode mode,
        QString *errorMessage);

    bool canRun(ProjectExplorer::RunConfiguration *runConfiguration,
        ProjectExplorer::RunMode mode) const;

    static DebuggerEngine *createEngine(DebuggerEngineType et,
        const DebuggerStartParameters &sp,
        QString *errorMessage);

    static DebuggerRunControl *createAndScheduleRun(
        const DebuggerStartParameters &sp,
        ProjectExplorer::RunConfiguration *runConfiguration = 0);

    static DebuggerRunControl *doCreate(const DebuggerStartParameters &sp,
        ProjectExplorer::RunConfiguration *rc, QString *errorMessage);

private:
    QString displayName() const;
    ProjectExplorer::RunConfigWidget *createConfigurationWidget(
        ProjectExplorer::RunConfiguration *runConfiguration);
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGERRUNCONTROLFACTORY_H
