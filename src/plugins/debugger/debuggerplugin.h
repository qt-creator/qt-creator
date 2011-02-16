/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef DEBUGGERPLUGIN_H
#define DEBUGGERPLUGIN_H

#include "debugger_global.h"

#include <extensionsystem/iplugin.h>

QT_FORWARD_DECLARE_CLASS(QAction)

namespace ProjectExplorer {
class RunConfiguration;
class RunControl;
}

namespace Debugger {

class DebuggerMainWindow;
class DebuggerRunControl;
class DebuggerStartParameters;

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
    static QAction *visibleDebugAction();

private:
    // IPlugin implementation.
    bool initialize(const QStringList &arguments, QString *errorMessage);
    void remoteCommand(const QStringList &options, const QStringList &arguments);
    ShutdownFlag aboutToShutdown();
    void extensionsInitialized();
};

} // namespace Debugger

#endif // DEBUGGERPLUGIN_H
