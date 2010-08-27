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

class DebuggerPluginPrivate;
class DebuggerRunControl;
class DebuggerStartParameters;

namespace Internal {
class DebuggerEngine;
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
    static void displayDebugger(Internal::DebuggerEngine *engine, bool updateEngine = true);

    QVariant sessionValue(const QString &name);
    void setSessionValue(const QString &name, const QVariant &value);
    QVariant configValue(const QString &name) const;
    void setConfigValue(const QString &name, const QVariant &value);
    void updateState(Internal::DebuggerEngine *engine);

    QIcon locationMarkIcon() const;
    void activateDebugMode();

    const CPlusPlus::Snapshot &cppCodeModelSnapshot() const;
    bool isRegisterViewVisible() const;

    void openTextEditor(const QString &titlePattern, const QString &contents);

public slots:
    void clearCppCodeModelSnapshot();
    void ensureLogVisible();

    // void runTest(const QString &fileName);
    void showMessage(const QString &msg, int channel, int timeout = -1);

private:
    friend class Internal::DebuggerEngine;
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
    Internal::DebuggerEngine *sessionTemplate();

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
