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
class QTreeView;
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

enum TestCases
{
    // Gdb
    TestNoBoundsOfCurrentFunction = 1
};

// Some convenience.
void updateState(DebuggerEngine *engine);
void updateWatchersWindow(bool showWatch, bool showReturn);
QIcon locationMarkIcon();
const CPlusPlus::Snapshot &cppCodeModelSnapshot();
bool hasSnapshots();
void openTextEditor(const QString &titlePattern, const QString &contents);
bool isActiveDebugLanguage(int language);

// void runTest(const QString &fileName);
void showMessage(const QString &msg, int channel, int timeout = -1);

bool isReverseDebugging();
void runControlStarted(DebuggerEngine *engine);
void runControlFinished(DebuggerEngine *engine);
void displayDebugger(DebuggerEngine *engine, bool updateEngine);
DebuggerLanguages activeLanguages();
void synchronizeBreakpoints();

QWidget *mainWindow();
bool isDockVisible(const QString &objectName);
void showModuleSymbols(const QString &moduleName, const QVector<Internal::Symbol> &symbols);
void showModuleSections(const QString &moduleName, const QVector<Internal::Section> &sections);
void openMemoryEditor();

void setThreadBoxContents(const QStringList &list, int index);

QSharedPointer<Internal::GlobalDebuggerOptions> globalDebuggerOptions();

QTreeView *inspectorView();
QVariant sessionValue(const QByteArray &name);
void setSessionValue(const QByteArray &name, const QVariant &value);
QVariant configValue(const QByteArray &name);
void setConfigValue(const QByteArray &name, const QVariant &value);

Utils::SavedAction *action(int code);
bool boolSetting(int code);
QString stringSetting(int code);
QStringList stringListSetting(int code);

BreakHandler *breakHandler();
DebuggerEngine *currentEngine();

QMessageBox *showMessageBox(int icon, const QString &title,
    const QString &text, int buttons = 0);

bool isNativeMixedActive();
bool isNativeMixedEnabled();
bool isReverseDebuggingEnabled();

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGERPLUGIN_H
