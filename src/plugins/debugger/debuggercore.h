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

#include "debuggerconstants.h"

#include <projectexplorer/abi.h>

#include <QObject>
#include <QSharedPointer>

#include <functional>

QT_BEGIN_NAMESPACE
class QIcon;
class QMessageBox;
class QWidget;
class QMenu;
class QAction;
QT_END_NAMESPACE

namespace CPlusPlus { class Snapshot; }

namespace Utils { class SavedAction; }

namespace Debugger {
namespace Internal {

class BreakHandler;
class DebuggerEngine;
class Symbol;
class Section;
class GlobalDebuggerOptions;
class WatchTreeView;

enum TestCases
{
    // Gdb
    TestNoBoundsOfCurrentFunction = 1
};

// Some convenience.
void updateState(DebuggerEngine *engine);
void updateWatchersWindow(bool showWatch, bool showReturn);
const CPlusPlus::Snapshot &cppCodeModelSnapshot();
bool hasSnapshots();
void openTextEditor(const QString &titlePattern, const QString &contents);

// void runTest(const QString &fileName);
void showMessage(const QString &msg, int channel, int timeout = -1);

bool isReverseDebugging();
void runControlStarted(DebuggerEngine *engine);
void runControlFinished(DebuggerEngine *engine);
void displayDebugger(DebuggerEngine *engine, bool updateEngine);
void synchronizeBreakpoints();

void saveModeToRestore();
QWidget *mainWindow();
bool isRegistersWindowVisible();
bool isModulesWindowVisible();
void showModuleSymbols(const QString &moduleName, const QVector<Internal::Symbol> &symbols);
void showModuleSections(const QString &moduleName, const QVector<Internal::Section> &sections);
void openMemoryEditor();

void setThreadBoxContents(const QStringList &list, int index);

QSharedPointer<Internal::GlobalDebuggerOptions> globalDebuggerOptions();

WatchTreeView *inspectorView();
QVariant sessionValue(const QByteArray &name);
void setSessionValue(const QByteArray &name, const QVariant &value);
QVariant configValue(const QString &name);
void setConfigValue(const QString &name, const QVariant &value);

bool isTestRun();

Utils::SavedAction *action(int code);
bool boolSetting(int code);
QString stringSetting(int code);
QStringList stringListSetting(int code);

BreakHandler *breakHandler();
DebuggerEngine *currentEngine();

QMessageBox *showMessageBox(int icon, const QString &title,
    const QString &text, int buttons = 0);

bool isReverseDebuggingEnabled();

QAction *addAction(QMenu *menu, const QString &display, bool on,
                   const std::function<void()> &onTriggered = {});
QAction *addAction(QMenu *menu, const QString &d1, const QString &d2, bool on,
                   const std::function<void()> &onTriggered);
QAction *addCheckableAction(QMenu *menu, const QString &display, bool on, bool checked,
                            const std::function<void()> &onTriggered);

} // namespace Internal
} // namespace Debugger
