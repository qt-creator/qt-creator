// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <utils/id.h>

#include <QObject>

#include <functional>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QWidget;
QT_END_NAMESPACE

namespace Utils { class FancyMainWindow; }

namespace Core {

class PerspectivesView;

class CORE_EXPORT Perspective : public QObject
{
public:
    // A null view registers the perspective with the default (Debug) view.
    Perspective(const QString &id, const QString &name,
                const QString &parentPerspectiveId = QString(),
                const QString &settingId = QString(),
                PerspectivesView *view = nullptr);
    ~Perspective();

    enum OperationType { SplitVertical, SplitHorizontal, AddToTab, Raise };

    void addWindow(QWidget *widget, // No ownership passed.
                   OperationType op,
                   QWidget *anchorWidget,
                   bool visibleByDefault = true,
                   Qt::DockWidgetArea area = Qt::BottomDockWidgetArea);

    void addToolBarAction(QAction *action, Qt::ToolButtonStyle style = Qt::ToolButtonIconOnly);
    void addToolBarWidget(QWidget *widget); // No ownership passed.
    void addToolbarSeparator();

    void registerNextPrevShortcuts(QAction *next, QAction *prev);

    void useSubPerspectiveSwitcher(QWidget *widget); // No ownership passed.

    QString id() const; // Currently used by GammaRay plugin.
    QString parentPerspectiveId() const;
    QString name() const;

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

    friend class PerspectivesView;
    friend class PerspectivesViewPrivate;
    friend class PerspectivePrivate;
    class PerspectivePrivate *d = nullptr;
};

class CORE_EXPORT PerspectivesView : public QObject
{
    Q_OBJECT

public:
    // The default (Debug) view.
    static PerspectivesView *instance();
    static void ensureMainWindowExists();

    // Creates an additional view, hosted in its own mode.
    static PerspectivesView *createView(const QString &settingsGroup,
                                        const Utils::Id &modeContext,
                                        const QString &statusObjectName);

    // Tears down a view created with createView(). Must be called before the
    // hosting mode's widget is destroyed (see the Profiler mode).
    static void destroyView(PerspectivesView *view);

    // Shuts down all views.
    static void doShutdown();

    void showStatusMessage(const QString &message, int timeoutMS);
    void enableMainWindow(bool on);
    void showPermanentStatusMessage(const QString &message);
    void enter();
    void leave();

    void addSubPerspectiveSwitcher(QWidget *widget);
    void addPerspectiveMenu(QMenu *menu);

    Perspective *currentPerspective();

    Utils::FancyMainWindow *mainWindow();
    QWidget *centralWidgetStack();

signals:
    void perspectivesChanged();
    void modeActivationRequested();

private:
    PerspectivesView(const QString &settingsGroup,
                     const Utils::Id &modeContext,
                     const QString &statusObjectName);
    ~PerspectivesView() override;

    void savePersistentSettings() const;
    void restorePersistentSettings();

    friend class Perspective;
    friend class PerspectivePrivate;
    friend class DockOperation;
    class PerspectivesViewPrivate *d = nullptr;
};

} // Core
