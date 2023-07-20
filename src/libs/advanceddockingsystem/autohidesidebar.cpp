// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#include "autohidesidebar.h"

#include "ads_globals_p.h"
#include "autohidedockcontainer.h"
#include "autohidetab.h"
#include "dockareawidget.h"
#include "dockcontainerwidget.h"
#include "dockfocuscontroller.h"
#include "dockingstatereader.h"
#include "dockwidgettab.h"

#include <QBoxLayout>
#include <QPainter>
#include <QResizeEvent>
#include <QStyleOption>
#include <QXmlStreamWriter>

namespace ADS {

class TabsWidget;

/**
 * Private data class of CSideTabBar class (pimpl)
 */
struct AutoHideSideBarPrivate
{
    /**
     * Private data constructor
     */
    AutoHideSideBarPrivate(AutoHideSideBar *parent);

    AutoHideSideBar *q;
    DockContainerWidget *m_containerWidget;
    TabsWidget *m_tabsContainerWidget;
    QBoxLayout *m_tabsLayout;
    Qt::Orientation m_orientation;
    SideBarLocation m_sideTabArea = SideBarLocation::SideBarLeft;

    /**
     * Convenience function to check if this is a horizontal side bar
     */
    bool isHorizontal() const { return Qt::Horizontal == m_orientation; }

    /**
     * Called from viewport to forward event handling to this
     */
    void handleViewportEvent(QEvent* e);
}; // struct AutoHideSideBarPrivate

/**
 * This widget stores the tab buttons
 */
class TabsWidget : public QWidget
{
public:
    using QWidget::QWidget;
    using Super = QWidget;
    AutoHideSideBarPrivate *eventHandler;

    /**
     * Returns the size hint as minimum size hint
     */
    virtual QSize minimumSizeHint() const override { return Super::sizeHint(); }

    /**
     * Forward event handling to EventHandler
     */
    virtual bool event(QEvent *e) override
    {
        eventHandler->handleViewportEvent(e);
        return Super::event(e);
    }
};

AutoHideSideBarPrivate::AutoHideSideBarPrivate(AutoHideSideBar *parent)
    : q(parent)
{}

void AutoHideSideBarPrivate::handleViewportEvent(QEvent* e)
{
    switch (e->type()) {
    case QEvent::ChildRemoved:
        if (m_tabsLayout->isEmpty())
            q->hide();
        break;

    default:
        break;
    }
}

AutoHideSideBar::AutoHideSideBar(DockContainerWidget *parent, SideBarLocation area)
    : Super(parent)
    , d(new AutoHideSideBarPrivate(this))
{
    d->m_sideTabArea = area;
    d->m_containerWidget = parent;
    d->m_orientation = (area == SideBarLocation::SideBarBottom
                        || area == SideBarLocation::SideBarTop)
                           ? Qt::Horizontal
                           : Qt::Vertical;

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setFrameStyle(QFrame::NoFrame);
    setWidgetResizable(true);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    d->m_tabsContainerWidget = new TabsWidget();
    d->m_tabsContainerWidget->eventHandler = d;
    d->m_tabsContainerWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    d->m_tabsContainerWidget->setObjectName("sideTabsContainerWidget");

    d->m_tabsLayout = new QBoxLayout(d->m_orientation == Qt::Vertical ? QBoxLayout::TopToBottom
                                                                      : QBoxLayout::LeftToRight);
    d->m_tabsLayout->setContentsMargins(0, 0, 0, 0);
    d->m_tabsLayout->setSpacing(12);
    d->m_tabsLayout->addStretch(1);
    d->m_tabsContainerWidget->setLayout(d->m_tabsLayout);
    setWidget(d->m_tabsContainerWidget);

    setFocusPolicy(Qt::NoFocus);
    if (d->isHorizontal())
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    else
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    hide();
}

AutoHideSideBar::~AutoHideSideBar()
{
    qCInfo(adsLog) << Q_FUNC_INFO;
    // The SideTabeBar is not the owner of the tabs and to prevent deletion
    // we set the parent here to nullptr to remove it from the children
    auto tabs = findChildren<AutoHideTab *>(QString(), Qt::FindDirectChildrenOnly);
    for (auto tab : tabs)
        tab->setParent(nullptr);

    delete d;
}

void AutoHideSideBar::insertTab(int index, AutoHideTab *sideTab)
{
    sideTab->setSideBar(this);
    sideTab->installEventFilter(this);
    // Default insertion is append
    if (index < 0)
        d->m_tabsLayout->insertWidget(d->m_tabsLayout->count() - 1, sideTab);
    else
        d->m_tabsLayout->insertWidget(index, sideTab);

    show();
}

AutoHideDockContainer *AutoHideSideBar::insertDockWidget(int index, DockWidget *dockWidget)
{
    auto autoHideContainer = new AutoHideDockContainer(dockWidget,
                                                       d->m_sideTabArea,
                                                       d->m_containerWidget);
    dockWidget->dockManager()->dockFocusController()->clearDockWidgetFocus(dockWidget);
    auto tab = autoHideContainer->autoHideTab();
    dockWidget->setSideTabWidget(tab);
    insertTab(index, tab);
    return autoHideContainer;
}

void AutoHideSideBar::removeAutoHideWidget(AutoHideDockContainer *autoHideWidget)
{
    autoHideWidget->autoHideTab()->removeFromSideBar();
    auto dockContainer = autoHideWidget->dockContainer();
    if (dockContainer)
        dockContainer->removeAutoHideWidget(autoHideWidget);

    autoHideWidget->setParent(nullptr);
}

void AutoHideSideBar::addAutoHideWidget(AutoHideDockContainer *autoHideWidget, int index)
{
    auto sideBar = autoHideWidget->autoHideTab()->sideBar();
    if (sideBar == this) {
        // If we move to the same tab index or if we insert before the next tab index, then we will
        // end at the same tab position and can leave.
        if (autoHideWidget->tabIndex() == index || (autoHideWidget->tabIndex() + 1) == index)
            return;

        // We remove this auto hide widget from the sidebar in the code below and therefore need
        // to correct the TabIndex here.
        if (autoHideWidget->tabIndex() < index)
            --index;
    }

    if (sideBar)
        sideBar->removeAutoHideWidget(autoHideWidget);

    autoHideWidget->setParent(d->m_containerWidget);
    autoHideWidget->setSideBarLocation(d->m_sideTabArea);
    d->m_containerWidget->registerAutoHideWidget(autoHideWidget);
    insertTab(index, autoHideWidget->autoHideTab());
}

void AutoHideSideBar::removeTab(AutoHideTab *sideTab)
{
    sideTab->removeEventFilter(this);
    d->m_tabsLayout->removeWidget(sideTab);
    if (d->m_tabsLayout->isEmpty())
        hide();
}

bool AutoHideSideBar::eventFilter(QObject *watched, QEvent *event)
{
    auto tab = qobject_cast<AutoHideTab *>(watched);
    if (!tab)
        return false;

    switch (event->type()) {
    case QEvent::ShowToParent:
        show();
        break;

    case QEvent::HideToParent:
        if (!hasVisibleTabs())
            hide();
        break;

    default:
        break;
    }

    return false;
}

Qt::Orientation AutoHideSideBar::orientation() const
{
    return d->m_orientation;
}

AutoHideTab *AutoHideSideBar::tab(int index) const
{
    return qobject_cast<AutoHideTab *>(d->m_tabsLayout->itemAt(index)->widget());
}

int AutoHideSideBar::tabAt(const QPoint &pos) const
{
    if (!isVisible())
        return TabInvalidIndex;

    if (orientation() == Qt::Horizontal) {
        if (pos.x() < tab(0)->geometry().x())
            return -1;
    } else {
        if (pos.y() < tab(0)->geometry().y())
            return -1;
    }

    for (int i = 0; i < count(); ++i) {
        if (tab(i)->geometry().contains(pos))
            return i;
    }

    return count();
}

int AutoHideSideBar::tabInsertIndexAt(const QPoint &pos) const
{
    int index = tabAt(pos);
    if (index == TabInvalidIndex)
        return TabDefaultInsertIndex;
    else
        return (index < 0) ? 0 : index;
}

int AutoHideSideBar::indexOfTab(const AutoHideTab &autoHideTab) const
{
    for (auto i = 0; i < count(); i++) {
        if (tab(i) == &autoHideTab)
            return i;
    }

    return -1;
}

int AutoHideSideBar::count() const
{
    return d->m_tabsLayout->count() - 1;
}

int AutoHideSideBar::visibleTabCount() const
{
    int c = 0;
    auto parent = parentWidget();
    for (auto i = 0; i < count(); i++) {
        if (tab(i)->isVisibleTo(parent))
            c++;
    }

    return c;
}

bool AutoHideSideBar::hasVisibleTabs() const
{
    auto parent = parentWidget();
    for (auto i = 0; i < count(); i++) {
        if (tab(i)->isVisibleTo(parent))
            return true;
    }

    return false;
}

SideBarLocation AutoHideSideBar::sideBarLocation() const
{
    return d->m_sideTabArea;
}

void AutoHideSideBar::saveState(QXmlStreamWriter &s) const
{
    if (!count())
        return;

    s.writeStartElement("sideBar");
    s.writeAttribute("area", QString::number(sideBarLocation()));
    s.writeAttribute("tabs", QString::number(count()));

    for (auto i = 0; i < count(); ++i) {
        auto currentTab = tab(i);
        if (!currentTab)
            continue;

        currentTab->dockWidget()->autoHideDockContainer()->saveState(s);
    }

    s.writeEndElement();
}

QSize AutoHideSideBar::minimumSizeHint() const
{
    QSize size = sizeHint();
    size.setWidth(10);
    return size;
}

QSize AutoHideSideBar::sizeHint() const
{
    return d->m_tabsContainerWidget->sizeHint();
}

int AutoHideSideBar::spacing() const
{
    return d->m_tabsLayout->spacing();
}

void AutoHideSideBar::setSpacing(int spacing)
{
    d->m_tabsLayout->setSpacing(spacing);
}

DockContainerWidget *AutoHideSideBar::dockContainer() const
{
    return d->m_containerWidget;
}

} // namespace ADS
