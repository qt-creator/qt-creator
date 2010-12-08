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

class DebuggerMainWindow;
class DebuggerRunControl;
class DebuggerStartParameters;

// This is the "external" interface of the debugger plugin that's visible
// from Qt Creator core. The internal interface to global debugger
// functionality that is used by debugger views and debugger engines
// is DebuggerCore, implemented in DebuggerPluginPrivate.

class DEBUGGER_EXPORT DebuggerPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    DebuggerPlugin();
    ~DebuggerPlugin();

    // Used by Maemo debugging support.
    static DebuggerRunControl *createDebugger(const DebuggerStartParameters &sp,
        ProjectExplorer::RunConfiguration *rc = 0);
    static void startDebugger(ProjectExplorer::RunControl *runControl);

    // Used by QmlJSInspector.
    static bool isActiveDebugLanguage(int language);
    static DebuggerMainWindow *mainWindow();

private:
    // IPlugin implementation.
    bool initialize(const QStringList &arguments, QString *errorMessage);
    void remoteCommand(const QStringList &options, const QStringList &arguments);
    ShutdownFlag aboutToShutdown();
    void extensionsInitialized();
};

} // namespace Debugger

#endif // DEBUGGERPLUGIN_H
