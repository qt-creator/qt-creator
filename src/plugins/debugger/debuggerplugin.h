/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include "debuggerconstants.h"

#include <extensionsystem/iplugin.h>

QT_BEGIN_NAMESPACE
class QIcon;
class QMessageBox;
QT_END_NAMESPACE

namespace CPlusPlus {
class Snapshot;
}

namespace ProjectExplorer {
class RunConfiguration;
class RunControl;
}

namespace Debugger {
class DebuggerEngine;
class DebuggerPluginPrivate;
class DebuggerRunControl;
class DebuggerStartParameters;

namespace Internal {
class DebuggerListener;
}

class DEBUGGER_EXPORT DebuggerPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    DebuggerPlugin();
    ~DebuggerPlugin();

    static DebuggerPlugin *instance();

    static DebuggerRunControl *createDebugger(const DebuggerStartParameters &sp,
        ProjectExplorer::RunConfiguration *rc = 0);
    static void startDebugger(ProjectExplorer::RunControl *runControl);
    static void displayDebugger(ProjectExplorer::RunControl *runControl);
    static void displayDebugger(DebuggerEngine *engine, bool updateEngine = true);

    QVariant sessionValue(const QString &name);
    void setSessionValue(const QString &name, const QVariant &value);
    QVariant configValue(const QString &name) const;
    void setConfigValue(const QString &name, const QVariant &value);
    void updateState(DebuggerEngine *engine);
    virtual void remoteCommand(const QStringList &options, const QStringList &arguments);

    QIcon locationMarkIcon() const;
    void activateDebugMode();

    const CPlusPlus::Snapshot &cppCodeModelSnapshot() const;
    bool isRegisterViewVisible() const;
    bool hasSnapsnots() const;

    void openTextEditor(const QString &titlePattern, const QString &contents);

public slots:
    void clearCppCodeModelSnapshot();
    void ensureLogVisible();

    // void runTest(const QString &fileName);
    void showMessage(const QString &msg, int channel, int timeout = -1);

private:
    friend class DebuggerEngine;
    friend class DebuggerPluginPrivate;
    friend class DebuggerRunControl;

    void resetLocation();
    void gotoLocation(const QString &fileName, int lineNumber, bool setMarker);
    void readSettings();
    void writeSettings() const;

    bool isReverseDebugging() const;
    void createNewDock(QWidget *widget);
    void runControlStarted(DebuggerRunControl *runControl);
    void runControlFinished(DebuggerRunControl *runControl);
    DebuggerLanguages activeLanguages() const;

    // This contains per-session data like breakpoints and watched
    // expression. It serves as a template for new engine instantiations.
    DebuggerEngine *sessionTemplate();

    QMessageBox *showMessageBox(int icon, const QString &title,
        const QString &text, int buttons = 0);

    bool initialize(const QStringList &arguments, QString *errorMessage);
    ShutdownFlag aboutToShutdown();
    void extensionsInitialized();

    QWidget *mainWindow() const;

    DebuggerPluginPrivate *d;
};

} // namespace Debugger

#endif // DEBUGGERPLUGIN_H
