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

#ifndef DEBUGGERCORE_H
#define DEBUGGERCORE_H

#include "debuggerconstants.h"

#include <projectexplorer/abi.h>

#include <QObject>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QIcon;
class QMessageBox;
class QWidget;
QT_END_NAMESPACE

namespace CPlusPlus {
class Snapshot;
}

namespace Utils {
class SavedAction;
}

namespace ProjectExplorer {
class RunControl;
}

namespace Debugger {

class DebuggerEngine;

namespace Internal {

class BreakHandler;
class SnapshotHandler;
class Symbol;
class Section;
class DebuggerToolTipManager;
class GlobalDebuggerOptions;

enum TestCases
{
    // Gdb
    TestNoBoundsOfCurrentFunction = 1,
    TestPythonDumpers
};

class DebuggerCore : public QObject
{
    Q_OBJECT

public:
    DebuggerCore() {}

    virtual QVariant sessionValue(const QString &name) = 0;
    virtual void setSessionValue(const QString &name, const QVariant &value) = 0;
    virtual QVariant configValue(const QString &name) const = 0;
    virtual void setConfigValue(const QString &name, const QVariant &value) = 0;
    virtual void updateState(DebuggerEngine *engine) = 0;
    virtual void updateWatchersWindow(bool showWatch, bool showReturn) = 0;
    virtual QIcon locationMarkIcon() const = 0;
    virtual const CPlusPlus::Snapshot &cppCodeModelSnapshot() const = 0;
    virtual bool hasSnapshots() const = 0;
    virtual void openTextEditor(const QString &titlePattern, const QString &contents) = 0;
    virtual BreakHandler *breakHandler() const = 0;
    virtual SnapshotHandler *snapshotHandler() const = 0;
    virtual DebuggerEngine *currentEngine() const = 0;
    virtual bool isActiveDebugLanguage(int language) const = 0;

    // void runTest(const QString &fileName);
    virtual void showMessage(const QString &msg, int channel, int timeout = -1) = 0;

    virtual bool isReverseDebugging() const = 0;
    virtual void runControlStarted(DebuggerEngine *engine) = 0;
    virtual void runControlFinished(DebuggerEngine *engine) = 0;
    virtual void displayDebugger(DebuggerEngine *engine, bool updateEngine) = 0;
    virtual DebuggerLanguages activeLanguages() const = 0;
    virtual void synchronizeBreakpoints() = 0;

    virtual bool initialize(const QStringList &arguments, QString *errorMessage) = 0;
    virtual QWidget *mainWindow() const = 0;
    virtual bool isDockVisible(const QString &objectName) const = 0;
//    virtual QString debuggerForAbi(const ProjectExplorer::Abi &abi,
//        DebuggerEngineType et = NoEngineType) const = 0;
    virtual void showModuleSymbols(const QString &moduleName,
        const QVector<Symbol> &symbols) = 0;
    virtual void showModuleSections(const QString &moduleName,
        const QVector<Section> &sections) = 0;
    virtual void openMemoryEditor() = 0;
    virtual void languagesChanged() = 0;
    virtual void executeDebuggerCommand(const QString &command, DebuggerLanguages languages) = 0;

    virtual Utils::SavedAction *action(int code) const = 0;
    virtual bool boolSetting(int code) const = 0;
    virtual QString stringSetting(int code) const = 0;
    virtual void setThreads(const QStringList &list, int index) = 0;

    virtual DebuggerToolTipManager *toolTipManager() const = 0;
    virtual QSharedPointer<GlobalDebuggerOptions> globalDebuggerOptions() const = 0;

public slots:
    virtual void attachExternalApplication(ProjectExplorer::RunControl *rc) = 0;
};

// This is the only way to access the global object.
DebuggerCore *debuggerCore();
inline BreakHandler *breakHandler() { return debuggerCore()->breakHandler(); }
QMessageBox *showMessageBox(int icon, const QString &title,
    const QString &text, int buttons = 0);

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGERPLUGIN_H
