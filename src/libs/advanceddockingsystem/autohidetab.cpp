// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#include "autohidetab.h"

#include "ads_globals_p.h"
#include "advanceddockingsystemtr.h"
#include "autohidedockcontainer.h"
#include "autohidesidebar.h"
#include "dockareawidget.h"
#include "dockmanager.h"
#include "dockoverlay.h"
#include "dockwidget.h"
#include "floatingdragpreview.h"

#include <QApplication>
#include <QBoxLayout>
#include <QContextMenuEvent>
#include <QElapsedTimer>
#include <QMenu>

namespace ADS {

static const char *const g_locationProperty = "Location";

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
    bool m_mousePressed = false;
    eDragState m_dragState = DraggingInactive;
    QPoint m_globalDragStartMousePosition;
    QPoint m_dragStartMousePosition;
    AbstractFloatingWidget *m_floatingWidget = nullptr;
    Qt::Orientation m_dragStartOrientation;

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

    /**
     * Helper function to create and initialize the menu entries for the "Auto Hide Group To..." menu.
     */
    QAction *createAutoHideToAction(const QString &title, SideBarLocation location, QMenu *menu)
    {
        auto action = menu->addAction(title);
        action->setProperty("Location", location);
        QObject::connect(action, &QAction::triggered, q, &AutoHideTab::onAutoHideToActionClicked);
        action->setEnabled(location != q->sideBarLocation());
        return action;
    }

    /**
     * Test function for current drag state.
     */
    bool isDraggingState(eDragState dragState) const { return m_dragState == dragState; }

    /**
     * Saves the drag start position in global and local coordinates.
     */
    void saveDragStartMousePosition(const QPoint &globalPos)
    {
        m_globalDragStartMousePosition = globalPos;
        m_dragStartMousePosition = q->mapFromGlobal(globalPos);
    }

    /**
     * Starts floating of the dock widget that belongs to this title bar
     * Returns true, if floating has been started and false if floating is not possible for any reason.
     */
    bool startFloating(eDragState draggingState = DraggingFloatingWidget);

    template<typename T>
    AbstractFloatingWidget *createFloatingWidget(T *widget)
    {
        auto w = new FloatingDragPreview(widget);
        q->connect(w, &FloatingDragPreview::draggingCanceled, [=]() {
            m_dragState = DraggingInactive;
        });
        return w;
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

bool AutoHideTabPrivate::startFloating(eDragState draggingState)
{
    auto dockArea = m_dockWidget->dockAreaWidget();
    qCInfo(adsLog) << Q_FUNC_INFO << "isFloating " << dockContainer()->isFloating();

    m_dragState = draggingState;
    AbstractFloatingWidget *floatingWidget = nullptr;
    floatingWidget = createFloatingWidget(dockArea);
    auto size = dockArea->size();
    auto startPos = m_dragStartMousePosition;
    auto autoHideContainer = m_dockWidget->autoHideDockContainer();
    m_dragStartOrientation = autoHideContainer->orientation();

    switch (m_sideBar->sideBarLocation()) {
    case SideBarLeft:
        startPos.rx() = autoHideContainer->rect().left() + 10;
        break;

    case SideBarRight:
        startPos.rx() = autoHideContainer->rect().right() - 10;
        break;

    case SideBarTop:
        startPos.ry() = autoHideContainer->rect().top() + 10;
        break;

    case SideBarBottom:
        startPos.ry() = autoHideContainer->rect().bottom() - 10;
        break;

    case SideBarNone:
        return false;
    }
    floatingWidget->startFloating(startPos, size, DraggingFloatingWidget, q);
    auto dockManager = m_dockWidget->dockManager();
    auto overlay = dockManager->containerOverlay();
    overlay->setAllowedAreas(OuterDockAreas);
    m_floatingWidget = floatingWidget;
    qApp->postEvent(m_dockWidget, new QEvent((QEvent::Type) internal::g_dockedWidgetDragStartEvent));

    return true;
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
bool AutoHideTab::iconOnly() const
{
    return DockManager::testAutoHideConfigFlag(DockManager::AutoHideSideBarsIconOnly)
           && !icon().isNull();
}

AutoHideSideBar *AutoHideTab::sideBar() const
{
    return d->m_sideBar;
}

int AutoHideTab::tabIndex() const
{
    if (!d->m_sideBar)
        return -1;

    return d->m_sideBar->indexOfTab(*this);
}

void AutoHideTab::setDockWidgetFloating()
{
    d->m_dockWidget->setFloating();
}

void AutoHideTab::unpinDockWidget()
{
    d->m_dockWidget->setAutoHide(false);
}

void AutoHideTab::requestCloseDockWidget()
{
    d->m_dockWidget->requestCloseDockWidget();
}

void AutoHideTab::onAutoHideToActionClicked()
{
    int location = sender()->property(g_locationProperty).toInt();
    d->m_dockWidget->setAutoHide(true, (SideBarLocation) location);
}

void AutoHideTab::setSideBar(AutoHideSideBar *sideTabBar)
{
    d->m_sideBar = sideTabBar;
    if (d->m_sideBar)
        d->updateOrientation();
}

void AutoHideTab::removeFromSideBar()
{
    if (d->m_sideBar == nullptr)
        return;

    d->m_sideBar->removeTab(this);
    setSideBar(nullptr);
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

void AutoHideTab::contextMenuEvent(QContextMenuEvent *event)
{
    event->accept();
    d->saveDragStartMousePosition(event->globalPos());

    const bool isFloatable = d->m_dockWidget->features().testFlag(DockWidget::DockWidgetFloatable);
    QMenu menu(this);

    QAction *detachAction = menu.addAction(Tr::tr("Detach"));
    detachAction->connect(detachAction,
                          &QAction::triggered,
                          this,
                          &AutoHideTab::setDockWidgetFloating);
    detachAction->setEnabled(isFloatable);

    auto isPinnable = d->m_dockWidget->features().testFlag(DockWidget::DockWidgetPinnable);
    detachAction->setEnabled(isPinnable);

    auto pinToMenu = menu.addMenu(Tr::tr("Pin To..."));
    pinToMenu->setEnabled(isPinnable);
    d->createAutoHideToAction(Tr::tr("Top"), SideBarTop, pinToMenu);
    d->createAutoHideToAction(Tr::tr("Left"), SideBarLeft, pinToMenu);
    d->createAutoHideToAction(Tr::tr("Right"), SideBarRight, pinToMenu);
    d->createAutoHideToAction(Tr::tr("Bottom"), SideBarBottom, pinToMenu);

    QAction *unpinAction = menu.addAction(Tr::tr("Unpin (Dock)"));
    unpinAction->connect(unpinAction, &QAction::triggered, this, &AutoHideTab::unpinDockWidget);
    menu.addSeparator();
    QAction *closeAction = menu.addAction(Tr::tr("Close"));
    closeAction->connect(closeAction,
                         &QAction::triggered,
                         this,
                         &AutoHideTab::requestCloseDockWidget);
    closeAction->setEnabled(d->m_dockWidget->features().testFlag(DockWidget::DockWidgetClosable));

    menu.exec(event->globalPos());
}

void AutoHideTab::mousePressEvent(QMouseEvent *event)
{
    // If AutoHideShowOnMouseOver is active, then the showing is triggered by a MousePressEvent
    // sent to this tab. To prevent accidental hiding of the tab by a mouse click, we wait at
    // least 500 ms before we accept the mouse click.
    if (!event->spontaneous()) {
        d->m_timeSinceHoverMousePress.restart();
        d->forwardEventToDockContainer(event);
    } else if (d->m_timeSinceHoverMousePress.hasExpired(500)) {
        d->forwardEventToDockContainer(event);
    }

    if (event->button() == Qt::LeftButton) {
        event->accept();
        d->m_mousePressed = true;
        d->saveDragStartMousePosition(event->globalPosition().toPoint());
        d->m_dragState = DraggingMousePressed;
    }

    Super::mousePressEvent(event);
}

void AutoHideTab::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        d->m_mousePressed = false;
        auto currentDragState = d->m_dragState;
        d->m_globalDragStartMousePosition = QPoint();
        d->m_dragStartMousePosition = QPoint();
        d->m_dragState = DraggingInactive;

        switch (currentDragState) {
        case DraggingTab:
            // End of tab moving, emit signal
            //if (d->DockArea) {
            //    event->accept();
            //    Q_EMIT moved(internal::globalPositionOf(event));
            //}
            break;

        case DraggingFloatingWidget:
            event->accept();
            d->m_floatingWidget->finishDragging();
            if (d->m_dockWidget->autoHideDockContainer()
                && d->m_dragStartOrientation != orientation())
                d->m_dockWidget->autoHideDockContainer()->resetToInitialDockWidgetSize();

            break;

        default:
            break; // do nothing
        }
    }

    Super::mouseReleaseEvent(event);
}

void AutoHideTab::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton) || d->isDraggingState(DraggingInactive)) {
        d->m_dragState = DraggingInactive;
        Super::mouseMoveEvent(event);
        return;
    }

    // Move floating window
    if (d->isDraggingState(DraggingFloatingWidget)) {
        d->m_floatingWidget->moveFloating();
        Super::mouseMoveEvent(event);
        return;
    }

    // move tab
    if (d->isDraggingState(DraggingTab)) {
        // Moving the tab is always allowed because it does not mean moving the dock widget around.
        //d->moveTab(ev);
    }

    auto mappedPos = mapToParent(event->pos());
    bool mouseOutsideBar = (mappedPos.x() < 0) || (mappedPos.x() > parentWidget()->rect().right());
    // Maybe a fixed drag distance is better here ?
    int dragDistanceY = qAbs(d->m_globalDragStartMousePosition.y()
                             - event->globalPosition().toPoint().y());
    if (dragDistanceY >= DockManager::startDragDistance() || mouseOutsideBar) {
        // Floating is only allowed for widgets that are floatable
        // We can create the drag preview if the widget is movable.
        auto Features = d->m_dockWidget->features();
        if (Features.testFlag(DockWidget::DockWidgetFloatable)
            || (Features.testFlag(DockWidget::DockWidgetMovable))) {
            d->startFloating();
        }
        return;
    }

    Super::mouseMoveEvent(event);
}

} // namespace ADS
