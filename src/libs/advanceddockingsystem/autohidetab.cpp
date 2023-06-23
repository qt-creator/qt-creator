// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#include "autohidetab.h"

#include "ads_globals_p.h"
#include "autohidedockcontainer.h"
#include "autohidesidebar.h"
#include "dockmanager.h"
#include "dockwidget.h"

#include <QApplication>
#include <QBoxLayout>
#include <QElapsedTimer>

namespace ADS {
/**
 * Private data class of CDockWidgetTab class (pimpl)
 */
struct AutoHideTabPrivate
{
    AutoHideTab *q;
    DockWidget *m_dockWidget = nullptr;
    AutoHideSideBar *m_sideBar = nullptr;
    Qt::Orientation m_orientation{Qt::Vertical};
    QElapsedTimer m_timeSinceHoverMousePress;

    /**
     * Private data constructor
     */
    AutoHideTabPrivate(AutoHideTab *parent);

    /**
     * Update the orientation, visibility and spacing based on the area of the side bar
     */
    void updateOrientation();

    /**
     * Convenience function to ease dock container access
     */
    DockContainerWidget *dockContainer() const
    {
        return m_dockWidget ? m_dockWidget->dockContainer() : nullptr;
    }

    /**
     * Forward this event to the dock container
     */
    void forwardEventToDockContainer(QEvent *event)
    {
        auto container = dockContainer();
        if (container)
            container->handleAutoHideWidgetEvent(event, q);
    }
}; // struct DockWidgetTabPrivate

AutoHideTabPrivate::AutoHideTabPrivate(AutoHideTab *parent)
    : q(parent)
{}

void AutoHideTabPrivate::updateOrientation()
{
    bool iconOnly = DockManager::testAutoHideConfigFlag(DockManager::AutoHideSideBarsIconOnly);
    if (iconOnly && !q->icon().isNull()) {
        q->setText("");
        q->setOrientation(Qt::Horizontal);
    } else {
        auto area = m_sideBar->sideBarLocation();
        q->setOrientation((area == SideBarBottom || area == SideBarTop) ? Qt::Horizontal
                                                                        : Qt::Vertical);
    }
}

void AutoHideTab::setSideBar(AutoHideSideBar *sideTabBar)
{
    d->m_sideBar = sideTabBar;
    if (d->m_sideBar)
        d->updateOrientation();
}

AutoHideSideBar *AutoHideTab::sideBar() const
{
    return d->m_sideBar;
}

void AutoHideTab::removeFromSideBar()
{
    if (d->m_sideBar == nullptr)
        return;

    d->m_sideBar->removeTab(this);
    setSideBar(nullptr);
}

AutoHideTab::AutoHideTab(QWidget *parent)
    : PushButton(parent)
    , d(new AutoHideTabPrivate(this))
{
    setAttribute(Qt::WA_NoMousePropagation);
    setFocusPolicy(Qt::NoFocus);
}

AutoHideTab::~AutoHideTab()
{
    qCInfo(adsLog) << Q_FUNC_INFO;
    delete d;
}

void AutoHideTab::updateStyle()
{
    internal::repolishStyle(this, internal::RepolishDirectChildren);
    update();
}

SideBarLocation AutoHideTab::sideBarLocation() const
{
    if (d->m_sideBar)
        return d->m_sideBar->sideBarLocation();

    return SideBarLeft;
}

void AutoHideTab::setOrientation(Qt::Orientation value)
{
    d->m_orientation = value;
    if (orientation() == Qt::Horizontal) {
        setMinimumWidth(100);
        setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    } else {
        setMinimumHeight(100);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    }

    PushButton::setButtonOrientation((Qt::Horizontal == value) ? PushButton::Horizontal
                                                               : PushButton::VerticalTopToBottom);
    updateStyle();
}

Qt::Orientation AutoHideTab::orientation() const
{
    return d->m_orientation;
}

bool AutoHideTab::isActiveTab() const
{
    if (d->m_dockWidget && d->m_dockWidget->autoHideDockContainer())
        return d->m_dockWidget->autoHideDockContainer()->isVisible();

    return false;
}

DockWidget *AutoHideTab::dockWidget() const
{
    return d->m_dockWidget;
}

void AutoHideTab::setDockWidget(DockWidget *dockWidget)
{
    if (!dockWidget)
        return;

    d->m_dockWidget = dockWidget;
    setText(dockWidget->windowTitle());
    setIcon(d->m_dockWidget->icon());
    setToolTip(dockWidget->windowTitle());
}

bool AutoHideTab::event(QEvent *event)
{
    if (!DockManager::testAutoHideConfigFlag(DockManager::AutoHideShowOnMouseOver))
        return Super::event(event);

    switch (event->type()) {
    case QEvent::Enter:
    case QEvent::Leave:
        d->forwardEventToDockContainer(event);
        break;

    case QEvent::MouseButtonPress:
        // If AutoHideShowOnMouseOver is active, then the showing is triggered by a MousePressEvent
        // sent to this tab. To prevent accidental hiding of the tab by a mouse click, we wait at
        // least 500 ms before we accept the mouse click.
        if (!event->spontaneous()) {
            d->m_timeSinceHoverMousePress.restart();
            d->forwardEventToDockContainer(event);
        } else if (d->m_timeSinceHoverMousePress.hasExpired(500)) {
            d->forwardEventToDockContainer(event);
        }
        break;

    default:
        break;
    }
    return Super::event(event);
}

bool AutoHideTab::iconOnly() const
{
    return DockManager::testAutoHideConfigFlag(DockManager::AutoHideSideBarsIconOnly)
           && !icon().isNull();
}

} // namespace ADS
