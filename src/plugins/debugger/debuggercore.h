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

#include <functional>

QT_BEGIN_NAMESPACE
class QWidget;
class QMenu;
class QAction;
QT_END_NAMESPACE

namespace Utils {
class BaseTreeView;
class SavedAction;
}

namespace Debugger {
namespace Internal {

class GlobalDebuggerOptions;

enum TestCases
{
    // Gdb
    TestNoBoundsOfCurrentFunction = 1
};

// Some convenience.
void openTextEditor(const QString &titlePattern, const QString &contents);

GlobalDebuggerOptions *globalDebuggerOptions();

bool isTestRun();

Utils::SavedAction *action(int code);

bool boolSetting(int code);
QString stringSetting(int code);
QStringList stringListSetting(int code);

QAction *addAction(QMenu *menu, const QString &display, bool on,
                   const std::function<void()> &onTriggered = {});
QAction *addAction(QMenu *menu, const QString &d1, const QString &d2, bool on,
                   const std::function<void()> &onTriggered);
QAction *addCheckableAction(QMenu *menu, const QString &display, bool on, bool checked,
                            const std::function<void()> &onTriggered);

void addHideColumnActions(QMenu *menu, QWidget *widget);


// Qt's various build paths for unpatched versions
QStringList qtBuildPaths();

QWidget *addSearch(Utils::BaseTreeView *treeView);

} // namespace Internal
} // namespace Debugger
