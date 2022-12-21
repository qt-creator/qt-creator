// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "debuggerconstants.h"

#include <projectexplorer/abi.h>

#include <functional>

QT_BEGIN_NAMESPACE
class QObject;
class QWidget;
class QMenu;
class QAction;
QT_END_NAMESPACE

namespace Utils { class BaseTreeView; }

namespace Debugger::Internal {

enum TestCases
{
    // Gdb
    TestNoBoundsOfCurrentFunction = 1
};

// Some convenience.
void openTextEditor(const QString &titlePattern, const QString &contents);

bool isTestRun();

QAction *addAction(const QObject *parent, QMenu *menu, const QString &display, bool on,
                   const std::function<void()> &onTriggered = {});
QAction *addAction(const QObject *parent, QMenu *menu, const QString &d1, const QString &d2, bool on,
                   const std::function<void()> &onTriggered);
QAction *addCheckableAction(const QObject *parent, QMenu *menu, const QString &display, bool on,
                            bool checked, const std::function<void()> &onTriggered);

// Qt's various build paths for unpatched versions
QStringList qtBuildPaths();

QWidget *addSearch(Utils::BaseTreeView *treeView);

} // Debugger::Internal
