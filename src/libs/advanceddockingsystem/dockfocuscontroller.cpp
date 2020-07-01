/****************************************************************************
**
** Copyright (C) 2020 Uwe Kindler
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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or (at your option) any later version.
** The licenses are as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPLv21 included in the packaging
** of this file. Please review the following information to ensure
** the GNU Lesser General Public License version 2.1 requirements
** will be met: https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
        if (d->m_dockManager->isRestoringState())
            return;

        DockAreaWidget* dockArea = qobject_cast<DockAreaWidget *>(sender());
        if (!dockArea || open)
            return;

        auto container = dockArea->dockContainer();
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
