/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef DEBUGGERPLUGIN_H
#define DEBUGGERPLUGIN_H

#include "debugger_global.h"

#include <extensionsystem/iplugin.h>

#include <QtCore/QObject>

namespace ProjectExplorer {
class RunConfiguration;
class RunControl;
}

namespace Debugger {

class DebuggerRunControl;
class DebuggerStartParameters;
class DebuggerUISwitcher;

// This is the "external" interface of the debugger plugin that's
// visible from Creator core. The internal interfact to global
// functionality to be used by debugger views and debugger engines
// is DebuggerCore, implemented in DebuggerPluginPrivate.

class DEBUGGER_EXPORT DebuggerPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    DebuggerPlugin();
    ~DebuggerPlugin();

    static DebuggerRunControl *createDebugger(const DebuggerStartParameters &sp,
        ProjectExplorer::RunConfiguration *rc = 0);
    static void startDebugger(ProjectExplorer::RunControl *runControl);
    static void displayDebugger(ProjectExplorer::RunControl *runControl);
    static bool isActiveDebugLanguage(int language);
    static DebuggerUISwitcher *uiSwitcher();

private:
    // IPlugin implementation.
    bool initialize(const QStringList &arguments, QString *errorMessage);
    void remoteCommand(const QStringList &options, const QStringList &arguments);
    ShutdownFlag aboutToShutdown();
    void extensionsInitialized();
    void readSettings();
    void writeSettings() const;
    void runControlStarted(DebuggerRunControl *runControl);
    void runControlFinished(DebuggerRunControl *runControl);
};

} // namespace Debugger

#endif // DEBUGGERPLUGIN_H
