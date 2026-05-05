// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "debugger_global.h"

#include <utils/fancymainwindow.h>
#include <utils/statuslabel.h>

#include <QAction>

#include <functional>

namespace Utils {

class DEBUGGER_EXPORT Perspective : public QObject
{
public:
    Perspective(const QString &id, const QString &name,
                const QString &parentPerspectiveId = QString(),
                const QString &settingId = QString());
    ~Perspective();

    enum OperationType { SplitVertical, SplitHorizontal, AddToTab, Raise };

    void setCentralWidget(QWidget *centralWidget);
    void addWindow(QWidget *widget, // Perspective takes ownership.
                   OperationType op,
                   QWidget *anchorWidget,
                   bool visibleByDefault = true,
                   Qt::DockWidgetArea area = Qt::BottomDockWidgetArea);

    void addToolBarAction(QAction *action, Qt::ToolButtonStyle style = Qt::ToolButtonIconOnly);
    void addToolBarWidget(QWidget *widget); // Perspecive takes ownership.
    void addToolbarSeparator();

    void registerNextPrevShortcuts(QAction *next, QAction *prev);

    void useSubPerspectiveSwitcher(QWidget *widget); // No ownership passed.

    QString id() const; // Currently used by GammaRay plugin.
    QString parentPerspectiveId() const;
    QString name() const;
    QWidget *centralWidget() const;

    using Callback = std::function<void()>;
    void setAboutToActivateCallback(const Callback &cb);

    void setEnabled(bool enabled);

    void select();
    void destroy();

    static Perspective *findPerspective(const QString &perspectiveId);

    bool isCurrent() const;

    static const char *savesHeaderKey();

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
    static void enableMainWindow(bool on);
    static void showPermanentStatusMessage(const QString &message);
    static void enterDebugMode();
    static void leaveDebugMode();

    static QWidget *centralWidgetStack();
    void addSubPerspectiveSwitcher(QWidget *widget);
    static void addPerspectiveMenu(QMenu *menu);

    static Perspective *currentPerspective();

signals:
    void perspectivesChanged();
    void debugModeRequested();

private:
    DebuggerMainWindow();
    ~DebuggerMainWindow() override;

    void savePersistentSettings() const;
    void restorePersistentSettings();

    void contextMenuEvent(QContextMenuEvent *ev) override;

    friend class Perspective;
    friend class PerspectivePrivate;
    friend class DockOperation;
    class DebuggerMainWindowPrivate *d = nullptr;
};

} // Utils
