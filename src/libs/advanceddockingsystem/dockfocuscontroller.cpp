// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#include "dockfocuscontroller.h"

#include "ads_globals_p.h"
#include "dockareatitlebar.h"
#include "dockareawidget.h"
#include "dockcontainerwidget.h"
#include "dockmanager.h"
#include "dockwidget.h"
#include "dockwidgettab.h"
#include "floatingdockcontainer.h"

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
#include "linux/floatingwidgettitlebar.h"
#endif

#include <QApplication>
#include <QPointer>
#include <QWindow>

#include <algorithm>
#include <iostream>

namespace ADS {

static const char *const g_focusedDockWidgetProperty = "FocusedDockWidget";

class DockFocusControllerPrivate
{
public:
    DockFocusController *q;
    QPointer<DockWidget> m_focusedDockWidget = nullptr;
    QPointer<DockAreaWidget> m_focusedArea = nullptr;
    QPointer<DockWidget> m_oldFocusedDockWidget = nullptr;
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    QPointer<FloatingDockContainer> m_floatingWidget = nullptr;
#endif
    DockManager *m_dockManager = nullptr;
    bool m_forceFocusChangedSignal = false;
    bool m_tabPressed = false;

    /**
     * Private data constructor
     */
    DockFocusControllerPrivate(DockFocusController *parent);

    /**
     * This function updates the focus style of the given dock widget and
     * the dock area that it belongs to
     */
    void updateDockWidgetFocus(DockWidget *dockWidget);
}; // class DockFocusControllerPrivate

static void updateDockWidgetFocusStyle(DockWidget *dockWidget, bool focused)
{
    dockWidget->setProperty("focused", focused);
    dockWidget->tabWidget()->setProperty("focused", focused);
    dockWidget->tabWidget()->updateStyle();
    internal::repolishStyle(dockWidget);
}

static void updateDockAreaFocusStyle(DockAreaWidget *dockArea, bool focused)
{
    dockArea->setProperty("focused", focused);
    internal::repolishStyle(dockArea);
    internal::repolishStyle(dockArea->titleBar());
}

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
static void updateFloatingWidgetFocusStyle(FloatingDockContainer *floatingWidget, bool focused)
{
    if (floatingWidget->hasNativeTitleBar())
        return;

    auto titleBar = qobject_cast<FloatingWidgetTitleBar *>(floatingWidget->titleBarWidget());
    if (!titleBar)
        return;

    titleBar->setProperty("focused", focused);
    titleBar->updateStyle();
}
#endif

DockFocusControllerPrivate::DockFocusControllerPrivate(DockFocusController *parent)
    : q(parent)
{}

void DockFocusControllerPrivate::updateDockWidgetFocus(DockWidget *dockWidget)
{
    if (!dockWidget->features().testFlag(DockWidget::DockWidgetFocusable))
        return;

    QWindow *window = nullptr;
    auto dockContainer = dockWidget->dockContainer();
    if (dockContainer)
        window = dockContainer->window()->windowHandle();

    if (window)
        window->setProperty(g_focusedDockWidgetProperty,
                            QVariant::fromValue(QPointer<DockWidget>(dockWidget)));

    DockAreaWidget *newFocusedDockArea = nullptr;
    if (m_focusedDockWidget)
        updateDockWidgetFocusStyle(m_focusedDockWidget, false);

    DockWidget *old = m_focusedDockWidget;
    m_focusedDockWidget = dockWidget;
    updateDockWidgetFocusStyle(m_focusedDockWidget, true);
    newFocusedDockArea = m_focusedDockWidget->dockAreaWidget();
    if (newFocusedDockArea && (m_focusedArea != newFocusedDockArea)) {
        if (m_focusedArea) {
            QObject::disconnect(m_focusedArea,
                                &DockAreaWidget::viewToggled,
                                q,
                                &DockFocusController::onFocusedDockAreaViewToggled);
            updateDockAreaFocusStyle(m_focusedArea, false);
        }

        m_focusedArea = newFocusedDockArea;
        updateDockAreaFocusStyle(m_focusedArea, true);
        QObject::connect(m_focusedArea,
                         &DockAreaWidget::viewToggled,
                         q,
                         &DockFocusController::onFocusedDockAreaViewToggled);
    }

    FloatingDockContainer *newFloatingWidget = nullptr;
    dockContainer = m_focusedDockWidget->dockContainer();
    if (dockContainer)
        newFloatingWidget = dockContainer->floatingWidget();

    if (newFloatingWidget)
        newFloatingWidget->setProperty(g_focusedDockWidgetProperty,
                                       QVariant::fromValue(QPointer<DockWidget>(dockWidget)));

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    // This code is required for styling the floating widget titlebar for linux
    // depending on the current focus state
    if (m_floatingWidget != newFloatingWidget) {
        if (m_floatingWidget) {
            updateFloatingWidgetFocusStyle(m_floatingWidget, false);
        }
        m_floatingWidget = newFloatingWidget;

        if (m_floatingWidget) {
            updateFloatingWidgetFocusStyle(m_floatingWidget, true);
        }
    }
#endif

    if (old == dockWidget && !m_forceFocusChangedSignal)
        return;

    m_forceFocusChangedSignal = false;
    if (dockWidget->isVisible()) {
        emit m_dockManager->focusedDockWidgetChanged(old, dockWidget);
    } else {
        m_oldFocusedDockWidget = old;
        QObject::connect(dockWidget,
                         &DockWidget::visibilityChanged,
                         q,
                         &DockFocusController::onDockWidgetVisibilityChanged);
    }
}

void DockFocusController::onDockWidgetVisibilityChanged(bool visible)
{
    auto dockWidget = qobject_cast<DockWidget *>(sender());
    QObject::disconnect(dockWidget,
                        &DockWidget::visibilityChanged,
                        this,
                        &DockFocusController::onDockWidgetVisibilityChanged);
    if (dockWidget && visible)
        emit d->m_dockManager->focusedDockWidgetChanged(d->m_oldFocusedDockWidget, dockWidget);
}

DockFocusController::DockFocusController(DockManager *dockManager)
    : QObject(dockManager)
    , d(new DockFocusControllerPrivate(this))
{
    d->m_dockManager = dockManager;
    connect(qApp,
            &QApplication::focusChanged,
            this,
            &DockFocusController::onApplicationFocusChanged);
    connect(qApp,
            &QApplication::focusWindowChanged,
            this,
            &DockFocusController::onFocusWindowChanged);
    connect(d->m_dockManager,
            &DockManager::stateRestored,
            this,
            &DockFocusController::onStateRestored);
}

DockFocusController::~DockFocusController()
{
    delete d;
}

void DockFocusController::onFocusWindowChanged(QWindow *focusWindow)
{
    if (!focusWindow)
        return;

    auto dockWidgetVar = focusWindow->property(g_focusedDockWidgetProperty);
    if (!dockWidgetVar.isValid())
        return;

    auto dockWidget = dockWidgetVar.value<QPointer<DockWidget>>();
    if (!dockWidget)
        return;

    d->updateDockWidgetFocus(dockWidget);
}

void DockFocusController::onApplicationFocusChanged(QWidget *focusedOld, QWidget *focusedNow)
{
    Q_UNUSED(focusedOld);

    // Ignore focus changes if we are restoring state, or if user clicked a tab which in turn
    // caused the focus change.
    if (d->m_dockManager->isRestoringState() || d->m_tabPressed)
        return;

    qCInfo(adsLog) << Q_FUNC_INFO << "old: " << focusedOld << "new:" << focusedNow;

    if (!focusedNow)
        return;

    DockWidget *dockWidget = qobject_cast<DockWidget *>(focusedNow);

    bool focusActual = dockWidget && dockWidget->widget();

    if (!dockWidget)
        dockWidget = internal::findParent<DockWidget *>(focusedNow);

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    if (!dockWidget)
        return;
#else
    if (!dockWidget || dockWidget->tabWidget()->isHidden())
        return;
#endif

    d->updateDockWidgetFocus(dockWidget);

    if (focusActual) {
        // QDS: Focus the actual content widget when dockWidget gets focus
        QMetaObject::invokeMethod(dockWidget->widget(), QOverload<>::of(&QWidget::setFocus),
                                  Qt::QueuedConnection);
    }
}

void DockFocusController::setDockWidgetTabFocused(DockWidgetTab *tab)
{
    auto dockWidget = tab->dockWidget();
    if (dockWidget)
        d->updateDockWidgetFocus(dockWidget);
}

void DockFocusController::clearDockWidgetFocus(DockWidget *dockWidget)
{
    dockWidget->clearFocus();
    updateDockWidgetFocusStyle(dockWidget, false);
}

void DockFocusController::setDockWidgetFocused(DockWidget *focusedNow)
{
    d->updateDockWidgetFocus(focusedNow);
}

void DockFocusController::onFocusedDockAreaViewToggled(bool open)
{
    if (d->m_dockManager->isRestoringState() || !d->m_focusedArea || open)
        return;

    DockAreaWidget *dockArea = qobject_cast<DockAreaWidget *>(sender());
    if (!dockArea || open)
        return;

    auto container = dockArea->dockContainer();
    auto openedDockAreas = container->openedDockAreas();
    if (openedDockAreas.isEmpty())
        return;

    d->updateDockWidgetFocus(openedDockAreas[0]->currentDockWidget());
}

void DockFocusController::notifyWidgetOrAreaRelocation(QWidget *droppedWidget)
{
    if (d->m_dockManager->isRestoringState())
        return;

    DockWidget *dockWidget = qobject_cast<DockWidget *>(droppedWidget);
    if (!dockWidget) {
        DockAreaWidget *dockArea = qobject_cast<DockAreaWidget *>(droppedWidget);
        if (dockArea)
            dockWidget = dockArea->currentDockWidget();
    }

    if (!dockWidget)
        return;

    d->m_forceFocusChangedSignal = true;
    DockManager::setWidgetFocus(dockWidget);
}

void DockFocusController::notifyFloatingWidgetDrop(FloatingDockContainer *floatingWidget)
{
    if (!floatingWidget || d->m_dockManager->isRestoringState())
        return;

    auto dockWidgetVar = floatingWidget->property(g_focusedDockWidgetProperty);
    if (!dockWidgetVar.isValid())
        return;

    auto dockWidget = dockWidgetVar.value<DockWidget *>();
    if (dockWidget) {
        dockWidget->dockAreaWidget()->setCurrentDockWidget(dockWidget);
        DockManager::setWidgetFocus(dockWidget->tabWidget());
    }
}

void DockFocusController::onStateRestored()
{
    if (d->m_focusedDockWidget)
        updateDockWidgetFocusStyle(d->m_focusedDockWidget, false);
}

DockWidget *DockFocusController::focusedDockWidget() const
{
    return d->m_focusedDockWidget.data();
}

void DockFocusController::setDockWidgetTabPressed(bool value)
{
    d->m_tabPressed = value;
}

} // namespace ADS
