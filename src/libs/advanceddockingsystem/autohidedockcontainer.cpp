// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#include "autohidedockcontainer.h"

#include "ads_globals_p.h"
#include "autohidesidebar.h"
#include "autohidetab.h"
#include "dockareawidget.h"
#include "dockcomponentsfactory.h"
#include "dockmanager.h"
#include "resizehandle.h"

#include <QApplication>
#include <QBoxLayout>
#include <QCursor>
#include <QPainter>
#include <QPointer>
#include <QSplitter>
#include <QXmlStreamWriter>

#include <iostream>

namespace ADS {

static const int resizeMargin = 30;

bool static isHorizontalArea(SideBarLocation area)
{
    switch (area) {
    case SideBarLocation::SideBarTop:
    case SideBarLocation::SideBarBottom:
        return true;
    case SideBarLocation::SideBarLeft:
    case SideBarLocation::SideBarRight:
        return false;
    default:
        return true;
    }

    return true;
}

Qt::Edge static edgeFromSideTabBarArea(SideBarLocation area)
{
    switch (area) {
    case SideBarLocation::SideBarTop:
        return Qt::BottomEdge;
    case SideBarLocation::SideBarBottom:
        return Qt::TopEdge;
    case SideBarLocation::SideBarLeft:
        return Qt::RightEdge;
    case SideBarLocation::SideBarRight:
        return Qt::LeftEdge;
    default:
        return Qt::LeftEdge;
    }

    return Qt::LeftEdge;
}

int resizeHandleLayoutPosition(SideBarLocation area)
{
    switch (area) {
    case SideBarLocation::SideBarBottom:
    case SideBarLocation::SideBarRight:
        return 0;

    case SideBarLocation::SideBarTop:
    case SideBarLocation::SideBarLeft:
        return 1;

    default:
        return 0;
    }

    return 0;
}

/**
 * Private data of CAutoHideDockContainer - pimpl
 */
struct AutoHideDockContainerPrivate
{
    AutoHideDockContainer *q;
    DockAreaWidget *m_dockArea{nullptr};
    DockWidget *m_dockWidget{nullptr};
    SideBarLocation m_sideTabBarArea = SideBarNone;
    QBoxLayout *m_layout = nullptr;
    ResizeHandle *m_resizeHandle = nullptr;
    QSize m_size; // creates invalid size
    QPointer<AutoHideTab> m_sideTab;
    QSize m_sizeCache;

    /**
     * Private data constructor
     */
    AutoHideDockContainerPrivate(AutoHideDockContainer *parent);

    /**
     * Convenience function to get a dock widget area
     */
    DockWidgetArea getDockWidgetArea(SideBarLocation area)
    {
        switch (area) {
        case SideBarLocation::SideBarLeft:
            return LeftDockWidgetArea;
        case SideBarLocation::SideBarRight:
            return RightDockWidgetArea;
        case SideBarLocation::SideBarBottom:
            return BottomDockWidgetArea;
        case SideBarLocation::SideBarTop:
            return TopDockWidgetArea;
        default:
            return LeftDockWidgetArea;
        }

        return LeftDockWidgetArea;
    }

    /**
     * Update the resize limit of the resize handle
     */
    void updateResizeHandleSizeLimitMax()
    {
        auto rect = q->dockContainer()->contentRect();
        const auto maxResizeHandleSize = m_resizeHandle->orientation() == Qt::Horizontal
                                             ? rect.width()
                                             : rect.height();
        m_resizeHandle->setMaxResizeSize(maxResizeHandleSize - resizeMargin);
    }

    /**
     * Convenience function to check, if this is an horizontal area
     */
    bool isHorizontal() const { return isHorizontalArea(m_sideTabBarArea); }

    /**
     * Forward this event to the dock container
     */
    void forwardEventToDockContainer(QEvent *event)
    {
        auto dockContainer = q->dockContainer();
        if (dockContainer)
            dockContainer->handleAutoHideWidgetEvent(event, q);
    }

}; // struct AutoHideDockContainerPrivate

AutoHideDockContainerPrivate::AutoHideDockContainerPrivate(AutoHideDockContainer *parent)
    : q(parent)
{}

DockContainerWidget *AutoHideDockContainer::dockContainer() const
{
    return internal::findParent<DockContainerWidget *>(this);
}

AutoHideDockContainer::AutoHideDockContainer(DockWidget *dockWidget,
                                             SideBarLocation area,
                                             DockContainerWidget *parent)
    : QFrame(parent)
    , d(new AutoHideDockContainerPrivate(this))
{
    hide(); // auto hide dock container is initially always hidden
    d->m_sideTabBarArea = area;
    d->m_sideTab = componentsFactory()->createDockWidgetSideTab(nullptr);
    connect(d->m_sideTab, &AutoHideTab::pressed, this, &AutoHideDockContainer::toggleCollapseState);
    d->m_dockArea = new DockAreaWidget(dockWidget->dockManager(), parent);
    d->m_dockArea->setObjectName("autoHideDockArea");
    d->m_dockArea->setAutoHideDockContainer(this);

    setObjectName("autoHideDockContainer");

    d->m_layout = new QBoxLayout(isHorizontalArea(area) ? QBoxLayout::TopToBottom
                                                        : QBoxLayout::LeftToRight);
    d->m_layout->setContentsMargins(0, 0, 0, 0);
    d->m_layout->setSpacing(0);
    setLayout(d->m_layout);
    d->m_resizeHandle = new ResizeHandle(edgeFromSideTabBarArea(area), this);
    d->m_resizeHandle->setMinResizeSize(64);
    bool opaqueResize = DockManager::testConfigFlag(DockManager::OpaqueSplitterResize);
    d->m_resizeHandle->setOpaqueResize(opaqueResize);
    d->m_size = d->m_dockArea->size();
    d->m_sizeCache = dockWidget->size();

    addDockWidget(dockWidget);
    parent->registerAutoHideWidget(this);
    // The dock area should not be added to the layout before it contains the dock widget. If you
    // add it to the layout before it contains the dock widget then you will likely see this
    // warning for OpenGL widgets or QAxWidgets:
    // setGeometry: Unable to set geometry XxY+Width+Height on QWidgetWindow/'WidgetClassWindow
    d->m_layout->addWidget(d->m_dockArea);
    d->m_layout->insertWidget(resizeHandleLayoutPosition(area), d->m_resizeHandle);
}

void AutoHideDockContainer::updateSize()
{
    auto dockContainerParent = dockContainer();
    if (!dockContainerParent)
        return;

    auto rect = dockContainerParent->contentRect();

    switch (sideBarLocation()) {
    case SideBarLocation::SideBarTop:
        resize(rect.width(), qMin(rect.height() - resizeMargin, d->m_size.height()));
        move(rect.topLeft());
        break;

    case SideBarLocation::SideBarLeft:
        resize(qMin(d->m_size.width(), rect.width() - resizeMargin), rect.height());
        move(rect.topLeft());
        break;

    case SideBarLocation::SideBarRight: {
        resize(qMin(d->m_size.width(), rect.width() - resizeMargin), rect.height());
        QPoint p = rect.topRight();
        p.rx() -= (width() - 1);
        move(p);
    } break;

    case SideBarLocation::SideBarBottom: {
        resize(rect.width(), qMin(rect.height() - resizeMargin, d->m_size.height()));
        QPoint p = rect.bottomLeft();
        p.ry() -= (height() - 1);
        move(p);
    } break;

    default:
        break;
    }

    if (orientation() == Qt::Horizontal)
        d->m_sizeCache.setHeight(this->height());
    else
        d->m_sizeCache.setWidth(this->width());
}

AutoHideDockContainer::~AutoHideDockContainer()
{
    qCInfo(adsLog) << Q_FUNC_INFO;

    // Remove event filter in case there are any queued messages
    qApp->removeEventFilter(this);
    if (dockContainer())
        dockContainer()->removeAutoHideWidget(this);

    if (d->m_sideTab)
        delete d->m_sideTab;

    delete d;
}

AutoHideSideBar *AutoHideDockContainer::autoHideSideBar() const
{
    if (d->m_sideTab) {
        return d->m_sideTab->sideBar();
    } else {
        auto container = dockContainer();
        return container ? container->autoHideSideBar(d->m_sideTabBarArea) : nullptr;
    }
}

AutoHideTab *AutoHideDockContainer::autoHideTab() const
{
    return d->m_sideTab;
}

DockWidget *AutoHideDockContainer::dockWidget() const
{
    return d->m_dockWidget;
}

void AutoHideDockContainer::addDockWidget(DockWidget *dockWidget)
{
    if (d->m_dockWidget) {
        // Remove the old dock widget at this area
        d->m_dockArea->removeDockWidget(d->m_dockWidget);
    }

    d->m_dockWidget = dockWidget;
    d->m_sideTab->setDockWidget(dockWidget);
    DockAreaWidget *oldDockArea = dockWidget->dockAreaWidget();
    auto isRestoringState = dockWidget->dockManager()->isRestoringState();
    if (oldDockArea && !isRestoringState) {
        // The initial size should be a little bit bigger than the original dock area size to
        // prevent that the resize handle of this auto hid dock area is near of the splitter of
        // the old dock area.
        d->m_size = oldDockArea->size() + QSize(16, 16);
        oldDockArea->removeDockWidget(dockWidget);
    }
    d->m_dockArea->addDockWidget(dockWidget);
    updateSize();
    // The dock area is not visible and will not update the size when updateSize() is called for
    // this auto hide container. Therefore we explicitly resize it here. As soon as it will
    // become visible, it will get the right size
    d->m_dockArea->resize(size());
}

SideBarLocation AutoHideDockContainer::sideBarLocation() const
{
    return d->m_sideTabBarArea;
}

void AutoHideDockContainer::setSideBarLocation(SideBarLocation sideBarLocation)
{
    if (d->m_sideTabBarArea == sideBarLocation)
        return;

    d->m_sideTabBarArea = sideBarLocation;
    d->m_layout->removeWidget(d->m_resizeHandle);
    d->m_layout->setDirection(isHorizontalArea(sideBarLocation) ? QBoxLayout::TopToBottom
                                                                : QBoxLayout::LeftToRight);
    d->m_layout->insertWidget(resizeHandleLayoutPosition(sideBarLocation), d->m_resizeHandle);
    d->m_resizeHandle->setHandlePosition(edgeFromSideTabBarArea(sideBarLocation));
    internal::repolishStyle(this, internal::RepolishDirectChildren);
}

DockAreaWidget *AutoHideDockContainer::dockAreaWidget() const
{
    return d->m_dockArea;
}

void AutoHideDockContainer::moveContentsToParent()
{
    cleanupAndDelete();
    // If we unpin the auto hide dock widget, then we insert it into the same
    // location like it had as a auto hide widget.  This brings the least surprise
    // to the user and he does not have to search where the widget was inserted.
    d->m_dockWidget->setDockArea(nullptr);
    auto DockContainer = dockContainer();
    DockContainer->addDockWidget(d->getDockWidgetArea(d->m_sideTabBarArea), d->m_dockWidget);
}

void AutoHideDockContainer::cleanupAndDelete()
{
    const auto dockWidget = d->m_dockWidget;
    if (dockWidget) {
        auto sideTab = d->m_sideTab;
        sideTab->removeFromSideBar();
        sideTab->setParent(nullptr);
        sideTab->hide();
    }

    hide();
    deleteLater();
}

void AutoHideDockContainer::saveState(QXmlStreamWriter &s)
{
    s.writeStartElement("widget");
    s.writeAttribute("name", d->m_dockWidget->objectName());
    s.writeAttribute("closed", QString::number(d->m_dockWidget->isClosed() ? 1 : 0));
    s.writeAttribute("size",
                     QString::number(d->isHorizontal() ? d->m_size.height() : d->m_size.width()));
    s.writeEndElement();
}

void AutoHideDockContainer::toggleView(bool enable)
{
    if (enable) {
        if (d->m_sideTab)
            d->m_sideTab->show();
    } else {
        if (d->m_sideTab)
            d->m_sideTab->hide();

        hide();
        qApp->removeEventFilter(this);
    }
}

void AutoHideDockContainer::collapseView(bool enable)
{
    if (enable) {
        hide();
        qApp->removeEventFilter(this);
    } else {
        updateSize();
        d->updateResizeHandleSizeLimitMax();
        raise();
        show();
        d->m_dockWidget->dockManager()->setDockWidgetFocused(d->m_dockWidget);
        qApp->installEventFilter(this);
    }

    qCInfo(adsLog) << Q_FUNC_INFO << enable;
    d->m_sideTab->updateStyle();
}

void AutoHideDockContainer::toggleCollapseState()
{
    collapseView(isVisible());
}

void AutoHideDockContainer::setSize(int size)
{
    if (d->isHorizontal())
        d->m_size.setHeight(size);
    else
        d->m_size.setWidth(size);

    updateSize();
}

Qt::Orientation AutoHideDockContainer::orientation() const
{
    return internal::isHorizontalSideBarLocation(d->m_sideTabBarArea) ? Qt::Horizontal
                                                                      : Qt::Vertical;
}

void AutoHideDockContainer::resetToInitialDockWidgetSize()
{
    if (orientation() == Qt::Horizontal)
        setSize(d->m_sizeCache.height());
    else
        setSize(d->m_sizeCache.width());
}

void AutoHideDockContainer::moveToNewSideBarLocation(SideBarLocation newSideBarLocation, int index)
{
    if (newSideBarLocation == sideBarLocation() && index == tabIndex())
        return;

    auto oldOrientation = orientation();
    auto sideBar = dockContainer()->autoHideSideBar(newSideBarLocation);
    sideBar->addAutoHideWidget(this, index);
    // If we move a horizontal auto hide container to a vertical position then we resize it to the
    // orginal dock widget size, to avoid an extremely stretched dock widget after insertion.
    if (sideBar->orientation() != oldOrientation)
        resetToInitialDockWidgetSize();
}

int AutoHideDockContainer::tabIndex() const
{
    return d->m_sideTab->tabIndex();
}

/**
 * Returns true if the object given in ancestor is an ancestor of the object given in descendant
 */
static bool objectIsAncestorOf(const QObject* descendant, const QObject* ancestor)
{
    if (!ancestor)
        return false;

    while (descendant) {
        if (descendant == ancestor)
            return true;

        descendant = descendant->parent();
    }

    return false;
}

/**
 * Returns true if the object given in ancestor is the object given in descendant
 * or if it is an ancestor of the object given in descendant
 */
static bool isObjectOrAncestor(const QObject *descendant, const QObject *ancestor)
{
    if (ancestor && (descendant == ancestor))
        return true;
    else
        return objectIsAncestorOf(descendant, ancestor);
}

bool AutoHideDockContainer::eventFilter(QObject *watched, QEvent *event)
{
    // A switch case statement would be nicer here, but we cannot use
    // internal::FloatingWidgetDragStartEvent in a switch case
    if (event->type() == QEvent::Resize) {
        if (!d->m_resizeHandle->isResizing())
            updateSize();
    } else if (event->type() == QEvent::MouseButtonPress) {
        auto widget = qobject_cast<QWidget *>(watched);
        // Ignore non widget events
        if (!widget)
            return QFrame::eventFilter(watched, event);

        // Now check, if the user clicked into the side tab and ignore this event,
        // because the side tab click handler will call collapseView(). If we
        // do not ignore this here, then we will collapse the container and the side tab
        // click handler will uncollapse it
        if (widget == d->m_sideTab.data())
            return QFrame::eventFilter(watched, event);

        // Now we check, if the user clicked inside of this auto hide container.
        // If the click is inside of this auto hide container, then we can
        // ignore the event, because the auto hide overlay should not get collapsed if
        // user works in it
        if (isObjectOrAncestor(widget, this))
            return QFrame::eventFilter(watched, event);

        // Ignore the mouse click if it is not inside of this container
        if (!isObjectOrAncestor(widget, dockContainer()))
            return QFrame::eventFilter(watched, event);

        // User clicked into container - collapse the auto hide widget
        collapseView(true);
    } else if (event->type() == internal::g_floatingWidgetDragStartEvent) {
        // If we are dragging our own floating widget, the we do not need to collapse the view
        auto widget = dockContainer()->floatingWidget();
        if (widget != watched)
            collapseView(true);
    } else if (event->type() == internal::g_dockedWidgetDragStartEvent) {
        collapseView(true);
    }

    return QFrame::eventFilter(watched, event);
}

void AutoHideDockContainer::resizeEvent(QResizeEvent *event)
{
    QFrame::resizeEvent(event);

    if (d->m_resizeHandle->isResizing()) {
        d->m_size = this->size();
        d->updateResizeHandleSizeLimitMax();
    }
}

void AutoHideDockContainer::leaveEvent(QEvent *event)
{
    // Resizing of the dock container via the resize handle in non opaque mode
    // mays cause a leave event that is not really a leave event. Therefore
    // we check here, if we are really outside of our rect.
    auto pos = mapFromGlobal(QCursor::pos());
    if (!rect().contains(pos))
        d->forwardEventToDockContainer(event);

    QFrame::leaveEvent(event);
}

bool AutoHideDockContainer::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::Enter:
    case QEvent::Hide:
        d->forwardEventToDockContainer(event);
        break;

    case QEvent::MouseButtonPress:
        return true;
        break;

    default:
        break;
    }

    return QFrame::event(event);
}

} // namespace ADS
