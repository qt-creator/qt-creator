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

#include "dockareawidget.h"

#include "dockareatabbar.h"
#include "dockareatitlebar.h"
#include "dockcomponentsfactory.h"
#include "dockcontainerwidget.h"
#include "dockmanager.h"
#include "dockoverlay.h"
#include "docksplitter.h"
#include "dockwidget.h"
#include "dockwidgettab.h"
#include "floatingdockcontainer.h"

#include <QList>
#include <QLoggingCategory>
#include <QMenu>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>
#include <QStackedLayout>
#include <QStyle>
#include <QVector>
#include <QWheelEvent>
#include <QXmlStreamWriter>

#include <iostream>

static Q_LOGGING_CATEGORY(adsLog, "qtc.qmldesigner.advanceddockingsystem", QtWarningMsg)

namespace ADS
{
    static const char *const INDEX_PROPERTY = "index";
    static const char *const ACTION_PROPERTY = "action";

    /**
     * Internal dock area layout mimics stack layout but only inserts the current
     * widget into the internal QLayout object.
     * \warning Only the current widget has a parent. All other widgets
     * do not have a parent. That means, a widget that is in this layout may
     * return nullptr for its parent() function if it is not the current widget.
     */
    class DockAreaLayout
    {
    private:
        QBoxLayout *m_parentLayout;
        QList<QWidget *> m_widgets;
        int m_currentIndex = -1;
        QWidget *m_currentWidget = nullptr;

    public:
    /**
      * Creates an instance with the given parent layout
      */
        DockAreaLayout(QBoxLayout *parentLayout)
            : m_parentLayout(parentLayout)
        {}

        /**
         * Returns the number of widgets in this layout
         */
        int count() const { return m_widgets.count(); }

        /**
         * Inserts the widget at the given index position into the internal widget
         * list
         */
        void insertWidget(int index, QWidget *widget)
        {
            widget->setParent(nullptr);
            if (index < 0)
                index = m_widgets.count();

            m_widgets.insert(index, widget);
            if (m_currentIndex < 0) {
                setCurrentIndex(index);
            } else {
                if (index <= m_currentIndex)
                    ++m_currentIndex;
            }
        }

        /**
         * Removes the given widget from the layout
         */
        void removeWidget(QWidget *widget)
        {
            if (currentWidget() == widget) {
                auto layoutItem = m_parentLayout->takeAt(1);
                if (layoutItem)
                    layoutItem->widget()->setParent(nullptr);

                m_currentWidget = nullptr;
                m_currentIndex = -1;
            } else if (indexOf(widget) < m_currentIndex) {
                --m_currentIndex;
            }
            m_widgets.removeOne(widget);
        }

        /**
         * Returns the current selected widget
         */
        QWidget *currentWidget() const { return m_currentWidget; }

        /**
         * Activates the widget with the give index.
         */
        void setCurrentIndex(int index)
        {
            QWidget *prev = currentWidget();
            QWidget *next = widget(index);
            if (!next || (next == prev && !m_currentWidget))
                return;

            bool reenableUpdates = false;
            QWidget *parent = m_parentLayout->parentWidget();

            if (parent && parent->updatesEnabled()) {
                reenableUpdates = true;
                parent->setUpdatesEnabled(false);
            }

            if (auto layoutItem = m_parentLayout->takeAt(1))
                layoutItem->widget()->setParent(nullptr);

            m_parentLayout->addWidget(next);
            if (prev)
                prev->hide();

            m_currentIndex = index;
            m_currentWidget = next;

            if (reenableUpdates)
                parent->setUpdatesEnabled(true);
        }

        /**
         * Returns the index of the current active widget
         */
        int currentIndex() const { return m_currentIndex; }

        /**
         * Returns true if there are no widgets in the layout
         */
        bool isEmpty() const { return m_widgets.empty(); }

        /**
         * Returns the index of the given widget
         */
        int indexOf(QWidget *widget) const { return m_widgets.indexOf(widget); }

        /**
         * Returns the widget for the given index
         */
        QWidget *widget(int index) const
        {
            return (index < m_widgets.size()) ? m_widgets.at(index) : nullptr;
        }

        /**
         * Returns the geometry of the current active widget
         */
        QRect geometry() const { return m_widgets.empty() ? QRect() : currentWidget()->geometry(); }
    };

    /**
     * Private data class of DockAreaWidget class (pimpl)
     */
    struct DockAreaWidgetPrivate
    {
        DockAreaWidget *q = nullptr;
        QBoxLayout *m_layout = nullptr;
        DockAreaLayout *m_contentsLayout = nullptr;
        DockAreaTitleBar *m_titleBar = nullptr;
        DockManager *m_dockManager = nullptr;
        bool m_updateTitleBarButtons = false;
        DockWidgetAreas m_allowedAreas = AllDockAreas;
        QSize m_minSizeHint;

        /**
         * Private data constructor
         */
        DockAreaWidgetPrivate(DockAreaWidget *parent);

        /**
         * Creates the layout for top area with tabs and close button
         */
        void createTitleBar();

        /**
         * Returns the dock widget with the given index
         */
        DockWidget *dockWidgetAt(int index)
        {
            return qobject_cast<DockWidget *>(m_contentsLayout->widget(index));
        }

        /**
         * Convenience function to ease title widget access by index
         */
        DockWidgetTab *tabWidgetAt(int index) { return dockWidgetAt(index)->tabWidget(); }

        /**
         * Returns the tab action of the given dock widget
         */
        QAction *dockWidgetTabAction(DockWidget *dockWidget) const
        {
            return qvariant_cast<QAction *>(dockWidget->property(ACTION_PROPERTY));
        }

        /**
         * Returns the index of the given dock widget
         */
        int dockWidgetIndex(DockWidget *dockWidget) const
        {
            return dockWidget->property(INDEX_PROPERTY).toInt();
        }

        /**
         * Convenience function for tabbar access
         */
        DockAreaTabBar *tabBar() const { return m_titleBar->tabBar(); }

        /**
         * Updates the enable state of the close and detach button
         */
        void updateTitleBarButtonStates();

        /**
         * Scans all contained dock widgets for the max. minimum size hint
         */
        void updateMinimumSizeHint()
        {
            m_minSizeHint = QSize();
            for (int i = 0; i < m_contentsLayout->count(); ++i)
            {
                auto widget = m_contentsLayout->widget(i);
                m_minSizeHint.setHeight(qMax(m_minSizeHint.height(),
                                             widget->minimumSizeHint().height()));
                m_minSizeHint.setWidth(qMax(m_minSizeHint.width(),
                                            widget->minimumSizeHint().width()));
            }
        }
    };
    // struct DockAreaWidgetPrivate

    DockAreaWidgetPrivate::DockAreaWidgetPrivate(DockAreaWidget *parent)
        : q(parent)
    {}

    void DockAreaWidgetPrivate::createTitleBar()
    {
        m_titleBar = componentsFactory()->createDockAreaTitleBar(q);
        m_layout->addWidget(m_titleBar);
        QObject::connect(tabBar(),
                         &DockAreaTabBar::tabCloseRequested,
                         q,
                         &DockAreaWidget::onTabCloseRequested);
        QObject::connect(m_titleBar,
                         &DockAreaTitleBar::tabBarClicked,
                         q,
                         &DockAreaWidget::setCurrentIndex);
        QObject::connect(tabBar(), &DockAreaTabBar::tabMoved, q, &DockAreaWidget::reorderDockWidget);
    }

    void DockAreaWidgetPrivate::updateTitleBarButtonStates()
    {
        if (q->isHidden()) {
            m_updateTitleBarButtons = true;
            return;
        }

        m_titleBar->button(TitleBarButtonClose)
            ->setEnabled(q->features().testFlag(DockWidget::DockWidgetClosable));
        m_titleBar->button(TitleBarButtonUndock)
            ->setEnabled(q->features().testFlag(DockWidget::DockWidgetFloatable));
        m_titleBar->updateDockWidgetActionsButtons();
        m_updateTitleBarButtons = false;
    }

    DockAreaWidget::DockAreaWidget(DockManager *dockManager, DockContainerWidget *parent)
        : QFrame(parent)
        , d(new DockAreaWidgetPrivate(this))
    {
        d->m_dockManager = dockManager;
        d->m_layout = new QBoxLayout(QBoxLayout::TopToBottom);
        d->m_layout->setContentsMargins(0, 0, 0, 0);
        d->m_layout->setSpacing(0);
        setLayout(d->m_layout);

        d->createTitleBar();
        d->m_contentsLayout = new DockAreaLayout(d->m_layout);
        if (d->m_dockManager)
            emit d->m_dockManager->dockAreaCreated(this);
    }

    DockAreaWidget::~DockAreaWidget()
    {
        qCInfo(adsLog) << Q_FUNC_INFO;
        delete d->m_contentsLayout;
        delete d;
    }

    DockManager *DockAreaWidget::dockManager() const { return d->m_dockManager; }

    DockContainerWidget *DockAreaWidget::dockContainer() const
    {
        return internal::findParent<DockContainerWidget *>(this);
    }

    void DockAreaWidget::addDockWidget(DockWidget *dockWidget)
    {
        insertDockWidget(d->m_contentsLayout->count(), dockWidget);
    }

    void DockAreaWidget::insertDockWidget(int index, DockWidget *dockWidget, bool activate)
    {
        d->m_contentsLayout->insertWidget(index, dockWidget);
        dockWidget->setDockArea(this);
        dockWidget->tabWidget()->setDockAreaWidget(this);
        auto tabWidget = dockWidget->tabWidget();
        // Inserting the tab will change the current index which in turn will
        // make the tab widget visible in the slot
        d->tabBar()->blockSignals(true);
        d->tabBar()->insertTab(index, tabWidget);
        d->tabBar()->blockSignals(false);
        tabWidget->setVisible(!dockWidget->isClosed());
        dockWidget->setProperty(INDEX_PROPERTY, index);
        d->m_minSizeHint.setHeight(qMax(d->m_minSizeHint.height(),
                                        dockWidget->minimumSizeHint().height()));
        d->m_minSizeHint.setWidth(qMax(d->m_minSizeHint.width(),
                                       dockWidget->minimumSizeHint().width()));
        if (activate)
            setCurrentIndex(index);

        // If this dock area is hidden, then we need to make it visible again
        // by calling dockWidget->toggleViewInternal(true);
        if (!this->isVisible() && d->m_contentsLayout->count() > 1 && !dockManager()->isRestoringState())
            dockWidget->toggleViewInternal(true);

        d->updateTitleBarButtonStates();
    }

    void DockAreaWidget::removeDockWidget(DockWidget *dockWidget)
    {
        qCInfo(adsLog) << Q_FUNC_INFO;
        auto nextOpen = nextOpenDockWidget(dockWidget);

        d->m_contentsLayout->removeWidget(dockWidget);
        auto tabWidget = dockWidget->tabWidget();
        tabWidget->hide();
        d->tabBar()->removeTab(tabWidget);
        DockContainerWidget *dockContainerWidget = dockContainer();
        if (nextOpen) {
            setCurrentDockWidget(nextOpen);
        } else if (d->m_contentsLayout->isEmpty() && dockContainerWidget->dockAreaCount() >= 1) {
            qCInfo(adsLog) << "Dock Area empty";
            dockContainerWidget->removeDockArea(this);
            this->deleteLater();
        } else {
            // if contents layout is not empty but there are no more open dock
            // widgets, then we need to hide the dock area because it does not
            // contain any visible content
            hideAreaWithNoVisibleContent();
        }

        d->updateTitleBarButtonStates();
        updateTitleBarVisibility();
        d->updateMinimumSizeHint();
        auto topLevelDockWidget = dockContainerWidget->topLevelDockWidget();
        if (topLevelDockWidget)
            topLevelDockWidget->emitTopLevelChanged(true);

#if (ADS_DEBUG_LEVEL > 0)
        dockContainerWidget->dumpLayout();
#endif
    }

    void DockAreaWidget::hideAreaWithNoVisibleContent()
    {
        this->toggleView(false);

        // Hide empty parent splitters
        auto splitter = internal::findParent<DockSplitter *>(this);
        internal::hideEmptyParentSplitters(splitter);

        //Hide empty floating widget
        DockContainerWidget *container = this->dockContainer();
        if (!container->isFloating()
            && DockManager::testConfigFlag(DockManager::HideSingleCentralWidgetTitleBar))
            return;

        updateTitleBarVisibility();
        auto topLevelWidget = container->topLevelDockWidget();
        auto floatingWidget = container->floatingWidget();
        if (topLevelWidget) {
            if (floatingWidget)
                floatingWidget->updateWindowTitle();

            DockWidget::emitTopLevelEventForWidget(topLevelWidget, true);
        } else if (container->openedDockAreas().isEmpty() && floatingWidget) {
            floatingWidget->hide();
        }
    }

    void DockAreaWidget::onTabCloseRequested(int index)
    {
        qCInfo(adsLog) << Q_FUNC_INFO << "index" << index;
        auto *currentDockWidget = dockWidget(index);
        if (currentDockWidget->features().testFlag(DockWidget::DockWidgetDeleteOnClose))
            currentDockWidget->closeDockWidgetInternal();
        else
            currentDockWidget->toggleView(false);
    }

    DockWidget *DockAreaWidget::currentDockWidget() const
    {
        int currentIdx = currentIndex();
        if (currentIdx < 0)
            return nullptr;

        return dockWidget(currentIdx);
    }

    void DockAreaWidget::setCurrentDockWidget(DockWidget *dockWidget)
    {
        if (dockManager()->isRestoringState())
            return;

        internalSetCurrentDockWidget(dockWidget);
    }

    void DockAreaWidget::internalSetCurrentDockWidget(DockWidget *dockWidget)
    {
        int index = indexOf(dockWidget);
        if (index < 0)
            return;

        setCurrentIndex(index);
    }

    void DockAreaWidget::setCurrentIndex(int index)
    {
        auto currentTabBar = d->tabBar();
        if (index < 0 || index > (currentTabBar->count() - 1)) {
            qWarning() << Q_FUNC_INFO << "Invalid index" << index;
            return;
        }

        auto cw = d->m_contentsLayout->currentWidget();
        auto nw = d->m_contentsLayout->widget(index);
        if (cw == nw && !nw->isHidden())
            return;

        emit currentChanging(index);
        currentTabBar->setCurrentIndex(index);
        d->m_contentsLayout->setCurrentIndex(index);
        d->m_contentsLayout->currentWidget()->show();
        emit currentChanged(index);
    }

    int DockAreaWidget::currentIndex() const { return d->m_contentsLayout->currentIndex(); }

    QRect DockAreaWidget::titleBarGeometry() const { return d->m_titleBar->geometry(); }

    QRect DockAreaWidget::contentAreaGeometry() const { return d->m_contentsLayout->geometry(); }

    int DockAreaWidget::indexOf(DockWidget *dockWidget)
    {
        return d->m_contentsLayout->indexOf(dockWidget);
    }

    QList<DockWidget *> DockAreaWidget::dockWidgets() const
    {
        QList<DockWidget *> dockWidgetList;
        for (int i = 0; i < d->m_contentsLayout->count(); ++i)
            dockWidgetList.append(dockWidget(i));

        return dockWidgetList;
    }

    int DockAreaWidget::openDockWidgetsCount() const
    {
        int count = 0;
        for (int i = 0; i < d->m_contentsLayout->count(); ++i) {
            if (!dockWidget(i)->isClosed())
                ++count;
        }
        return count;
    }

    QList<DockWidget *> DockAreaWidget::openedDockWidgets() const
    {
        QList<DockWidget *> dockWidgetList;
        for (int i = 0; i < d->m_contentsLayout->count(); ++i) {
            DockWidget *currentDockWidget = dockWidget(i);
            if (!currentDockWidget->isClosed())
                dockWidgetList.append(dockWidget(i));
        }
        return dockWidgetList;
    }

    int DockAreaWidget::indexOfFirstOpenDockWidget() const
    {
        for (int i = 0; i < d->m_contentsLayout->count(); ++i) {
            if (!dockWidget(i)->isClosed())
                return i;
        }

        return - 1;
    }

    int DockAreaWidget::dockWidgetsCount() const { return d->m_contentsLayout->count(); }

    DockWidget *DockAreaWidget::dockWidget(int index) const
    {
        return qobject_cast<DockWidget *>(d->m_contentsLayout->widget(index));
    }

    void DockAreaWidget::reorderDockWidget(int fromIndex, int toIndex)
    {
        qCInfo(adsLog) << Q_FUNC_INFO;
        if (fromIndex >= d->m_contentsLayout->count() || fromIndex < 0
            || toIndex >= d->m_contentsLayout->count() || toIndex < 0 || fromIndex == toIndex) {
            qCInfo(adsLog) << "Invalid index for tab movement" << fromIndex << toIndex;
            return;
        }

        auto widget = d->m_contentsLayout->widget(fromIndex);
        d->m_contentsLayout->removeWidget(widget);
        d->m_contentsLayout->insertWidget(toIndex, widget);
        setCurrentIndex(toIndex);
    }

    void DockAreaWidget::toggleDockWidgetView(DockWidget *dockWidget, bool open)
    {
        Q_UNUSED(dockWidget)
        Q_UNUSED(open)
        updateTitleBarVisibility();
    }

    void DockAreaWidget::updateTitleBarVisibility()
    {
        DockContainerWidget *container = dockContainer();
        if (!container)
            return;

        if (DockManager::testConfigFlag(DockManager::AlwaysShowTabs))
            return;

        if (d->m_titleBar) {
            bool hidden = container->hasTopLevelDockWidget() && (container->isFloating()
                          || DockManager::testConfigFlag(DockManager::HideSingleCentralWidgetTitleBar));
            d->m_titleBar->setVisible(!hidden);
        }
    }

    void DockAreaWidget::markTitleBarMenuOutdated()
    {
        if (d->m_titleBar)
            d->m_titleBar->markTabsMenuOutdated();
    }

    void DockAreaWidget::saveState(QXmlStreamWriter &stream) const
    {
        stream.writeStartElement("area");
        stream.writeAttribute("tabs", QString::number(d->m_contentsLayout->count()));
        auto localDockWidget = currentDockWidget();
        QString name = localDockWidget ? localDockWidget->objectName() : "";
        stream.writeAttribute("current", name);
        qCInfo(adsLog) << Q_FUNC_INFO << "TabCount: " << d->m_contentsLayout->count()
                       << " Current: " << name;
        for (int i = 0; i < d->m_contentsLayout->count(); ++i)
            dockWidget(i)->saveState(stream);

        stream.writeEndElement();
    }

    DockWidget *DockAreaWidget::nextOpenDockWidget(DockWidget *dockWidget) const
    {
        auto openDockWidgets = openedDockWidgets();
        if (openDockWidgets.count() > 1
            || (openDockWidgets.count() == 1 && openDockWidgets[0] != dockWidget)) {
            DockWidget *nextDockWidget;
            if (openDockWidgets.last() == dockWidget) {
                nextDockWidget = openDockWidgets[openDockWidgets.count() - 2];
            } else {
                int nextIndex = openDockWidgets.indexOf(dockWidget) + 1;
                nextDockWidget = openDockWidgets[nextIndex];
            }

            return nextDockWidget;
        } else {
            return nullptr;
        }
    }

    DockWidget::DockWidgetFeatures DockAreaWidget::features(eBitwiseOperator mode) const
    {
        if (BitwiseAnd == mode) {
            DockWidget::DockWidgetFeatures features(DockWidget::AllDockWidgetFeatures);
            for (const auto dockWidget : dockWidgets())
                features &= dockWidget->features();

            return features;
        } else {
            DockWidget::DockWidgetFeatures features(DockWidget::NoDockWidgetFeatures);
            for (const auto dockWidget : dockWidgets())
                features |= dockWidget->features();

            return features;
        }
    }

    void DockAreaWidget::toggleView(bool open)
    {
        setVisible(open);

        emit viewToggled(open);
    }

    void DockAreaWidget::setVisible(bool visible)
    {
        Super::setVisible(visible);
        if (d->m_updateTitleBarButtons)
            d->updateTitleBarButtonStates();
    }

    void DockAreaWidget::setAllowedAreas(DockWidgetAreas areas)
    {
        d->m_allowedAreas = areas;
    }

    DockWidgetAreas DockAreaWidget::allowedAreas() const
    {
        return d->m_allowedAreas;
    }

    QAbstractButton *DockAreaWidget::titleBarButton(eTitleBarButton which) const
    {
        return d->m_titleBar->button(which);
    }

    void DockAreaWidget::closeArea()
    {
        // If there is only one single dock widget and this widget has the
        // DeleteOnClose feature, then we delete the dock widget now
        auto openDockWidgets = openedDockWidgets();
        if (openDockWidgets.count() == 1
            && openDockWidgets[0]->features().testFlag(DockWidget::DockWidgetDeleteOnClose)) {
            openDockWidgets[0]->closeDockWidgetInternal();
        } else {
            for (auto dockWidget : openedDockWidgets())
                dockWidget->toggleView(false);
        }
    }

    void DockAreaWidget::closeOtherAreas() { dockContainer()->closeOtherAreas(this); }

    DockAreaTitleBar *DockAreaWidget::titleBar() const { return d->m_titleBar; }

    QSize DockAreaWidget::minimumSizeHint() const
    {
        return d->m_minSizeHint.isValid() ? d->m_minSizeHint : Super::minimumSizeHint();
    }

} // namespace ADS
