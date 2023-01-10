// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#include "dockfocuscontroller.h"

#include "dockareawidget.h"
#include "dockareatitlebar.h"
#include "dockcontainerwidget.h"
#include "dockmanager.h"
#include "dockwidget.h"
#include "dockwidgettab.h"
#include "floatingdockcontainer.h"

#ifdef Q_OS_LINUX
#include "linux/floatingwidgettitlebar.h"
#endif

#include <QApplication>
#include <QPointer>

#include <algorithm>
#include <iostream>

namespace ADS
{
    /**
     * Private data class of CDockFocusController class (pimpl)
     */
    class DockFocusControllerPrivate
    {
    public:
        DockFocusController *q;
        QPointer<DockWidget> m_focusedDockWidget = nullptr;
        QPointer<DockAreaWidget> m_focusedArea = nullptr;
    #ifdef Q_OS_LINUX
        QPointer<FloatingDockContainer> m_floatingWidget = nullptr;
    #endif
        DockManager *m_dockManager = nullptr;

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

#ifdef Q_OS_LINUX
    static void updateFloatingWidgetFocusStyle(FloatingDockContainer *floatingWidget, bool focused)
    {
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
        DockAreaWidget *newFocusedDockArea = nullptr;
        if (m_focusedDockWidget)
            updateDockWidgetFocusStyle(m_focusedDockWidget, false);

        DockWidget *old = m_focusedDockWidget;
        m_focusedDockWidget = dockWidget;
        updateDockWidgetFocusStyle(m_focusedDockWidget, true);
        newFocusedDockArea = m_focusedDockWidget->dockAreaWidget();
        if (newFocusedDockArea && (m_focusedArea != newFocusedDockArea))
        {
            if (m_focusedArea)
            {
                QObject::disconnect(m_focusedArea, &DockAreaWidget::viewToggled,
                                    q, &DockFocusController::onFocusedDockAreaViewToggled);
                updateDockAreaFocusStyle(m_focusedArea, false);
            }

            m_focusedArea = newFocusedDockArea;
            updateDockAreaFocusStyle(m_focusedArea, true);
            QObject::connect(m_focusedArea, &DockAreaWidget::viewToggled,
                             q, &DockFocusController::onFocusedDockAreaViewToggled);
        }

        auto newFloatingWidget = m_focusedDockWidget->dockContainer()->floatingWidget();
        if (newFloatingWidget)
            newFloatingWidget->setProperty("FocusedDockWidget", QVariant::fromValue(dockWidget));

    #ifdef Q_OS_LINUX
        // This code is required for styling the floating widget titlebar for linux
        // depending on the current focus state
        if (m_floatingWidget == newFloatingWidget)
            return;

        if (m_floatingWidget)
            updateFloatingWidgetFocusStyle(m_floatingWidget, false);

        m_floatingWidget = newFloatingWidget;

        if (m_floatingWidget)
            updateFloatingWidgetFocusStyle(m_floatingWidget, true);
    #endif

        if (old != dockWidget)
            emit m_dockManager->focusedDockWidgetChanged(old, dockWidget);
    }

    DockFocusController::DockFocusController(DockManager *dockManager)
        : QObject(dockManager)
        , d(new DockFocusControllerPrivate(this))
    {
        d->m_dockManager = dockManager;
        connect(qApp, &QApplication::focusChanged,
                this, &DockFocusController::onApplicationFocusChanged);
        connect(d->m_dockManager, &DockManager::stateRestored,
                this, &DockFocusController::onStateRestored);
    }

    DockFocusController::~DockFocusController()
    {
        delete d;
    }

    void DockFocusController::onApplicationFocusChanged(QWidget *focusedOld, QWidget *focusedNow)
    {
        Q_UNUSED(focusedOld);

        if (d->m_dockManager->isRestoringState())
            return;

        if (!focusedNow)
            return;

        DockWidget *dockWidget = nullptr;
        auto dockWidgetTab = qobject_cast<DockWidgetTab *>(focusedNow);
        if (dockWidgetTab)
            dockWidget = dockWidgetTab->dockWidget();

        if (!dockWidget)
            dockWidget = qobject_cast<DockWidget *>(focusedNow);

        if (!dockWidget)
            dockWidget = internal::findParent<DockWidget *>(focusedNow);

    #ifdef Q_OS_LINUX
        if (!dockWidget)
            return;
    #else
        if (!dockWidget || dockWidget->tabWidget()->isHidden())
            return;
    #endif

        d->updateDockWidgetFocus(dockWidget);
    }

    void DockFocusController::setDockWidgetFocused(DockWidget *focusedNow)
    {
        d->updateDockWidgetFocus(focusedNow);
    }

    void DockFocusController::onFocusedDockAreaViewToggled(bool open)
    {
        if (d->m_dockManager->isRestoringState() || !d->m_focusedArea || open)
            return;

        auto container = d->m_focusedArea->dockContainer();
        auto openedDockAreas = container->openedDockAreas();
        if (openedDockAreas.isEmpty())
            return;

        DockManager::setWidgetFocus(openedDockAreas[0]->currentDockWidget()->tabWidget());
    }

    void DockFocusController::notifyWidgetOrAreaRelocation(QWidget *droppedWidget)
    {
        if (d->m_dockManager->isRestoringState())
            return;

        DockWidget *dockWidget = qobject_cast<DockWidget *>(droppedWidget);
        if (dockWidget)
        {
            DockManager::setWidgetFocus(dockWidget->tabWidget());
            return;
        }

        DockAreaWidget* dockArea = qobject_cast<DockAreaWidget*>(droppedWidget);
        if (!dockArea)
            return;

        dockWidget = dockArea->currentDockWidget();
        DockManager::setWidgetFocus(dockWidget->tabWidget());
    }

    void DockFocusController::notifyFloatingWidgetDrop(FloatingDockContainer *floatingWidget)
    {
        if (!floatingWidget || d->m_dockManager->isRestoringState())
            return;

        auto vDockWidget = floatingWidget->property("FocusedDockWidget");
        if (!vDockWidget.isValid())
            return;

        auto dockWidget = vDockWidget.value<DockWidget *>();
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

} // namespace ADS
