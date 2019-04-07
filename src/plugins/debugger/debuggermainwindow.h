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

#include "debugger_global.h"

#include <utils/fancymainwindow.h>
#include <utils/statuslabel.h>

#include <QAction>
#include <QPointer>
#include <QToolButton>

#include <functional>

namespace Core {
class Context;
class Id;
} // Core

namespace Utils {

// To be used for actions that need hideable toolbuttons.
class DEBUGGER_EXPORT OptionalAction : public QAction
{
    Q_OBJECT

public:
    OptionalAction(const QString &text = QString());
    ~OptionalAction() override;

    void setVisible(bool on);
    void setToolButtonStyle(Qt::ToolButtonStyle style);

public:
    QPointer<QToolButton> m_toolButton;
};

class DEBUGGER_EXPORT Perspective : public QObject
{
public:
    Perspective(const QString &id, const QString &name,
                const QString &parentPerspectiveId = QString(),
                const QString &settingId = QString());
    ~Perspective();

    enum OperationType { SplitVertical, SplitHorizontal, AddToTab, Raise };

    void setCentralWidget(QWidget *centralWidget);
    void addWindow(QWidget *widget,
                   OperationType op,
                   QWidget *anchorWidget,
                   bool visibleByDefault = true,
                   Qt::DockWidgetArea area = Qt::BottomDockWidgetArea);

    void addToolBarAction(QAction *action);
    void addToolBarAction(OptionalAction *action);
    void addToolBarWidget(QWidget *widget);
    void addToolbarSeparator();

    void useSubPerspectiveSwitcher(QWidget *widget);

    using ShouldPersistChecker = std::function<bool()>;
    void setShouldPersistChecker(const ShouldPersistChecker &checker);

    QString id() const; // Currently used by GammaRay plugin.
    QString name() const;
    QWidget *centralWidget() const;

    using Callback = std::function<void()>;
    void setAboutToActivateCallback(const Callback &cb);

    void setEnabled(bool enabled);

    void select();
    void destroy();

    static Perspective *findPerspective(const QString &perspectiveId);

    bool isCurrent() const;

private:
    void rampDownAsCurrent();
    void rampUpAsCurrent();

    Perspective(const Perspective &) = delete;
    void operator=(const Perspective &) = delete;

    friend class DebuggerMainWindow;
    friend class DebuggerMainWindowPrivate;
    friend class PerspectivePrivate;
    class PerspectivePrivate *d = nullptr;
};

class DEBUGGER_EXPORT DebuggerMainWindow : public FancyMainWindow
{
    Q_OBJECT

public:
    static DebuggerMainWindow *instance();

    static void ensureMainWindowExists();
    static void doShutdown();

    static void showStatusMessage(const QString &message, int timeoutMS);
    static void enterDebugMode();
    static void leaveDebugMode();

    static QWidget *centralWidgetStack();
    void addSubPerspectiveSwitcher(QWidget *widget);

    static void savePersistentSettings();
    static void restorePersistentSettings();

    static Perspective *currentPerspective();

private:
    DebuggerMainWindow();
    ~DebuggerMainWindow() override;

    void contextMenuEvent(QContextMenuEvent *ev) override;

    friend class Perspective;
    friend class PerspectivePrivate;
    friend class DockOperation;
    class DebuggerMainWindowPrivate *d = nullptr;
};

} // Utils
