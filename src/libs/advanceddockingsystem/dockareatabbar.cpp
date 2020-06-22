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

#include "dockareatabbar.h"

#include "dockareawidget.h"
#include "dockmanager.h"
#include "dockoverlay.h"
#include "dockwidget.h"
#include "dockwidgettab.h"
#include "floatingdockcontainer.h"
#include "floatingdragpreview.h"

#include <QApplication>
#include <QBoxLayout>
#include <QLoggingCategory>
#include <QMouseEvent>
#include <QScrollBar>
#include <QtGlobal>

#include <iostream>

static Q_LOGGING_CATEGORY(adsLog, "qtc.qmldesigner.advanceddockingsystem", QtWarningMsg)

namespace ADS
{
    /**
     * Private data class of DockAreaTabBar class (pimpl)
     */
    class DockAreaTabBarPrivate
    {
    public:
        DockAreaTabBar *q;
        DockAreaWidget *m_dockArea = nullptr;
        QWidget *m_tabsContainerWidget = nullptr;
        QBoxLayout *m_tabsLayout = nullptr;
        int m_currentIndex = -1;

        /**
         * Private data constructor
         */
        DockAreaTabBarPrivate(DockAreaTabBar *parent);

        /**
         * Update tabs after current index changed or when tabs are removed.
         * The function reassigns the stylesheet to update the tabs
         */
        void updateTabs();

        /**
         * Convenience function to access first tab
         */
        DockWidgetTab *firstTab() const { return q->tab(0); }

        /**
         * Convenience function to access last tab
         */
        DockWidgetTab *lastTab() const { return q->tab(q->count() - 1); }
    }; // class DockAreaTabBarPrivate

    DockAreaTabBarPrivate::DockAreaTabBarPrivate(DockAreaTabBar *parent)
        : q(parent)
    {}

    void DockAreaTabBarPrivate::updateTabs()
    {
        // Set active TAB and update all other tabs to be inactive
        for (int i = 0; i < q->count(); ++i) {
            auto tabWidget = q->tab(i);
            if (!tabWidget)
                continue;

            if (i == m_currentIndex) {
                tabWidget->show();
                tabWidget->setActiveTab(true);
                q->ensureWidgetVisible(tabWidget);
            } else {
                tabWidget->setActiveTab(false);
            }
        }
    }

    DockAreaTabBar::DockAreaTabBar(DockAreaWidget *parent)
        : QScrollArea(parent)
        , d(new DockAreaTabBarPrivate(this))
    {
        d->m_dockArea = parent;
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        setFrameStyle(QFrame::NoFrame);
        setWidgetResizable(true);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        d->m_tabsContainerWidget = new QWidget();
        d->m_tabsContainerWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        d->m_tabsContainerWidget->setObjectName("tabsContainerWidget");
        d->m_tabsLayout = new QBoxLayout(QBoxLayout::LeftToRight);
        d->m_tabsLayout->setContentsMargins(0, 0, 0, 0);
        d->m_tabsLayout->setSpacing(0);
        d->m_tabsLayout->addStretch(1);
        d->m_tabsContainerWidget->setLayout(d->m_tabsLayout);
        setWidget(d->m_tabsContainerWidget);
    }

    DockAreaTabBar::~DockAreaTabBar() { delete d; }

    void DockAreaTabBar::wheelEvent(QWheelEvent *event)
    {
        event->accept();
        const int direction = event->angleDelta().y();
        if (direction < 0)
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() + 20);
        else
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() - 20);
    }

    void DockAreaTabBar::setCurrentIndex(int index)
    {
        if (index == d->m_currentIndex)
            return;

        if (index < -1 || index > (count() - 1)) {
            qWarning() << Q_FUNC_INFO << "Invalid index" << index;
            return;
        }

        emit currentChanging(index);
        d->m_currentIndex = index;
        d->updateTabs();
        updateGeometry();
        emit currentChanged(index);
    }

    int DockAreaTabBar::count() const
    {
        // The tab bar contains a stretch item as last item
        return d->m_tabsLayout->count() - 1;
    }

    void DockAreaTabBar::insertTab(int index, DockWidgetTab *dockWidgetTab)
    {
        d->m_tabsLayout->insertWidget(index, dockWidgetTab);
        connect(dockWidgetTab, &DockWidgetTab::clicked, this, &DockAreaTabBar::onTabClicked);
        connect(dockWidgetTab,
                &DockWidgetTab::closeRequested,
                this,
                &DockAreaTabBar::onTabCloseRequested);
        connect(dockWidgetTab,
                &DockWidgetTab::closeOtherTabsRequested,
                this,
                &DockAreaTabBar::onCloseOtherTabsRequested);
        connect(dockWidgetTab, &DockWidgetTab::moved, this, &DockAreaTabBar::onTabWidgetMoved);
        connect(dockWidgetTab,
                &DockWidgetTab::elidedChanged,
                this,
                &DockAreaTabBar::elidedChanged);
        dockWidgetTab->installEventFilter(this);
        emit tabInserted(index);
        if (index <= d->m_currentIndex)
            setCurrentIndex(d->m_currentIndex + 1);
        else if (d->m_currentIndex == -1)
            setCurrentIndex(index);

        updateGeometry();
    }

    void DockAreaTabBar::removeTab(DockWidgetTab *dockWidgetTab)
    {
        if (!count())
            return;

        qCInfo(adsLog) << Q_FUNC_INFO;
        int newCurrentIndex = currentIndex();
        int removeIndex = d->m_tabsLayout->indexOf(dockWidgetTab);
        if (count() == 1)
            newCurrentIndex = -1;

        if (newCurrentIndex > removeIndex) {
            newCurrentIndex--;
        } else if (newCurrentIndex == removeIndex) {
            newCurrentIndex = -1;
            // First we walk to the right to search for the next visible tab
            for (int i = (removeIndex + 1); i < count(); ++i) {
                if (tab(i)->isVisibleTo(this)) {
                    newCurrentIndex = i - 1;
                    break;
                }
            }

            // If there is no visible tab right to this tab then we walk to
            // the left to find a visible tab
            if (newCurrentIndex < 0) {
                for (int i = (removeIndex - 1); i >= 0; --i) {
                    if (tab(i)->isVisibleTo(this)) {
                        newCurrentIndex = i;
                        break;
                    }
                }
            }
        }

        emit removingTab(removeIndex);
        d->m_tabsLayout->removeWidget(dockWidgetTab);
        dockWidgetTab->disconnect(this);
        dockWidgetTab->removeEventFilter(this);
        qCInfo(adsLog) << "NewCurrentIndex " << newCurrentIndex;
        if (newCurrentIndex != d->m_currentIndex)
            setCurrentIndex(newCurrentIndex);
        else
            d->updateTabs();

        updateGeometry();
    }

    int DockAreaTabBar::currentIndex() const { return d->m_currentIndex; }

    DockWidgetTab *DockAreaTabBar::currentTab() const
    {
        if (d->m_currentIndex < 0)
            return nullptr;
        else
            return qobject_cast<DockWidgetTab *>(
                d->m_tabsLayout->itemAt(d->m_currentIndex)->widget());
    }

    void DockAreaTabBar::onTabClicked()
    {
        DockWidgetTab *tab = qobject_cast<DockWidgetTab *>(sender());
        if (!tab)
            return;

        int index = d->m_tabsLayout->indexOf(tab);
        if (index < 0)
            return;

        setCurrentIndex(index);
        emit tabBarClicked(index);
    }

    void DockAreaTabBar::onTabCloseRequested()
    {
        DockWidgetTab *tab = qobject_cast<DockWidgetTab *>(sender());
        int index = d->m_tabsLayout->indexOf(tab);
        closeTab(index);
    }

    void DockAreaTabBar::onCloseOtherTabsRequested()
    {
        auto senderTab = qobject_cast<DockWidgetTab *>(sender());
        for (int i = 0; i < count(); ++i) {
            auto currentTab = tab(i);
            if (currentTab->isClosable() && !currentTab->isHidden() && currentTab != senderTab) {
                // If the dock widget is deleted with the closeTab() call, its tab it will no longer
                // be in the layout, and thus the index needs to be updated to not skip any tabs
                int offset = currentTab->dockWidget()->features().testFlag(
                                 DockWidget::DockWidgetDeleteOnClose)
                                 ? 1
                                 : 0;
                closeTab(i);
                // If the the dock widget blocks closing, i.e. if the flag
                // CustomCloseHandling is set, and the dock widget is still open,
                // then we do not need to correct the index
                if (currentTab->dockWidget()->isClosed())
                    i -= offset;
            }
        }
    }

    DockWidgetTab *DockAreaTabBar::tab(int index) const
    {
        if (index >= count() || index < 0)
            return nullptr;

        return qobject_cast<DockWidgetTab *>(d->m_tabsLayout->itemAt(index)->widget());
    }

    void DockAreaTabBar::onTabWidgetMoved(const QPoint &globalPosition)
    {
        DockWidgetTab *movingTab = qobject_cast<DockWidgetTab *>(sender());
        if (!movingTab)
            return;

        int fromIndex = d->m_tabsLayout->indexOf(movingTab);
        auto mousePos = mapFromGlobal(globalPosition);
        mousePos.rx() = qMax(d->firstTab()->geometry().left(), mousePos.x());
        mousePos.rx() = qMin(d->lastTab()->geometry().right(), mousePos.x());
        int toIndex = -1;
        // Find tab under mouse
        for (int i = 0; i < count(); ++i) {
            DockWidgetTab *dropTab = tab(i);
            if (dropTab == movingTab || !dropTab->isVisibleTo(this)
                || !dropTab->geometry().contains(mousePos))
                continue;

            toIndex = d->m_tabsLayout->indexOf(dropTab);
            if (toIndex == fromIndex)
                toIndex = -1;

            break;
        }

        if (toIndex > -1) {
            d->m_tabsLayout->removeWidget(movingTab);
            d->m_tabsLayout->insertWidget(toIndex, movingTab);
            qCInfo(adsLog) << "tabMoved from" << fromIndex << "to" << toIndex;
            emit tabMoved(fromIndex, toIndex);
            setCurrentIndex(toIndex);
        } else {
            // Ensure that the moved tab is reset to its start position
            d->m_tabsLayout->update();
        }
    }

    void DockAreaTabBar::closeTab(int index)
    {
        if (index < 0 || index >= count())
            return;

        auto dockWidgetTab = tab(index);
        if (dockWidgetTab->isHidden())
            return;

        emit tabCloseRequested(index);
    }

    bool DockAreaTabBar::eventFilter(QObject *watched, QEvent *event)
    {
        bool result = Super::eventFilter(watched, event);
        DockWidgetTab *dockWidgetTab = qobject_cast<DockWidgetTab *>(watched);
        if (!dockWidgetTab)
            return result;

        switch (event->type()) {
        case QEvent::Hide:
            emit tabClosed(d->m_tabsLayout->indexOf(dockWidgetTab));
            updateGeometry();
            break;
        case QEvent::Show:
            emit tabOpened(d->m_tabsLayout->indexOf(dockWidgetTab));
            updateGeometry();
            break;
        default:
            break;
        }

        return result;
    }

    bool DockAreaTabBar::isTabOpen(int index) const
    {
        if (index < 0 || index >= count())
            return false;

        return !tab(index)->isHidden();
    }

    QSize DockAreaTabBar::minimumSizeHint() const
    {
        QSize size = sizeHint();
        size.setWidth(10);
        return size;
    }

    QSize DockAreaTabBar::sizeHint() const
    {
        return d->m_tabsContainerWidget->sizeHint();
    }

} // namespace ADS
