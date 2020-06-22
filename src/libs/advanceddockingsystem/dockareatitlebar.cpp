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

#include "dockareatitlebar.h"

#include "ads_globals.h"
#include "dockareatabbar.h"
#include "dockareawidget.h"
#include "dockmanager.h"
#include "dockoverlay.h"
#include "dockwidget.h"
#include "dockwidgettab.h"
#include "floatingdockcontainer.h"
#include "floatingdragpreview.h"
#include "iconprovider.h"
#include "dockcomponentsfactory.h"

#include <QBoxLayout>
#include <QLoggingCategory>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QStyle>

#include <iostream>

static Q_LOGGING_CATEGORY(adsLog, "qtc.qmldesigner.advanceddockingsystem", QtWarningMsg)

namespace ADS
{
    /**
     * Private data class of DockAreaTitleBar class (pimpl)
     */
    class DockAreaTitleBarPrivate
    {
    public:
        DockAreaTitleBar *q;
        QPointer<TitleBarButtonType> m_tabsMenuButton;
        QPointer<TitleBarButtonType> m_undockButton;
        QPointer<TitleBarButtonType> m_closeButton;
        QBoxLayout *m_layout = nullptr;
        DockAreaWidget *m_dockArea = nullptr;
        DockAreaTabBar *m_tabBar = nullptr;
        bool m_menuOutdated = true;
        QList<TitleBarButtonType *> m_dockWidgetActionsButtons;

        QPoint m_dragStartMousePos;
        eDragState m_dragState = DraggingInactive;
        AbstractFloatingWidget *m_floatingWidget = nullptr;

        /**
         * Private data constructor
         */
        DockAreaTitleBarPrivate(DockAreaTitleBar *parent);

        /**
         * Creates the title bar close and menu buttons
         */
        void createButtons();

        /**
         * Creates the internal TabBar
         */
        void createTabBar();

        /**
         * Convenience function for DockManager access
         */
        DockManager *dockManager() const { return m_dockArea->dockManager(); }

        /**
         * Returns true if the given config flag is set
         * Convenience function to ease config flag testing
         */
        static bool testConfigFlag(DockManager::eConfigFlag flag)
        {
            return DockManager::testConfigFlag(flag);
        }

        /**
         * Test function for current drag state
         */
        bool isDraggingState(eDragState dragState) const { return this->m_dragState == dragState; }


        /**
         * Starts floating
         */
        void startFloating(const QPoint &offset);

        /**
         * Makes the dock area floating
         */
        AbstractFloatingWidget *makeAreaFloating(const QPoint &offset, eDragState dragState);
    }; // class DockAreaTitleBarPrivate

    DockAreaTitleBarPrivate::DockAreaTitleBarPrivate(DockAreaTitleBar *parent)
        : q(parent)
    {}

    void DockAreaTitleBarPrivate::createButtons()
    {
        const QSize iconSize(14, 14);
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        // Tabs menu button
        m_tabsMenuButton = new TitleBarButton(testConfigFlag(DockManager::DockAreaHasTabsMenuButton));
        m_tabsMenuButton->setObjectName("tabsMenuButton");
        m_tabsMenuButton->setAutoRaise(true);
        m_tabsMenuButton->setPopupMode(QToolButton::InstantPopup);
        internal::setButtonIcon(m_tabsMenuButton,
                                QStyle::SP_TitleBarUnshadeButton,
                                ADS::DockAreaMenuIcon);
        QMenu *tabsMenu = new QMenu(m_tabsMenuButton);
#ifndef QT_NO_TOOLTIP
        tabsMenu->setToolTipsVisible(true);
#endif
        QObject::connect(tabsMenu, &QMenu::aboutToShow, q, &DockAreaTitleBar::onTabsMenuAboutToShow);
        m_tabsMenuButton->setMenu(tabsMenu);
        internal::setToolTip(m_tabsMenuButton, QObject::tr("List All Tabs"));
        m_tabsMenuButton->setSizePolicy(sizePolicy);
        m_tabsMenuButton->setIconSize(iconSize);
        m_layout->addWidget(m_tabsMenuButton, 0);
        QObject::connect(m_tabsMenuButton->menu(),
                         &QMenu::triggered,
                         q,
                         &DockAreaTitleBar::onTabsMenuActionTriggered);

        // Undock button
        m_undockButton = new TitleBarButton(testConfigFlag(DockManager::DockAreaHasUndockButton));
        m_undockButton->setObjectName("detachGroupButton");
        m_undockButton->setAutoRaise(true);
        internal::setToolTip(m_undockButton, QObject::tr("Detach Group"));
        internal::setButtonIcon(m_undockButton,
                                QStyle::SP_TitleBarNormalButton,
                                ADS::DockAreaUndockIcon);
        m_undockButton->setSizePolicy(sizePolicy);
        m_undockButton->setIconSize(iconSize);
        m_layout->addWidget(m_undockButton, 0);
        QObject::connect(m_undockButton,
                         &QToolButton::clicked,
                         q,
                         &DockAreaTitleBar::onUndockButtonClicked);

        // Close button
        m_closeButton = new TitleBarButton(testConfigFlag(DockManager::DockAreaHasCloseButton));
        m_closeButton->setObjectName("dockAreaCloseButton");
        m_closeButton->setAutoRaise(true);
        internal::setButtonIcon(m_closeButton,
                                QStyle::SP_TitleBarCloseButton,
                                ADS::DockAreaCloseIcon);
        if (testConfigFlag(DockManager::DockAreaCloseButtonClosesTab))
            internal::setToolTip(m_closeButton, QObject::tr("Close Active Tab"));
        else
            internal::setToolTip(m_closeButton, QObject::tr("Close Group"));

        m_closeButton->setSizePolicy(sizePolicy);
        m_closeButton->setIconSize(iconSize);
        m_layout->addWidget(m_closeButton, 0);
        QObject::connect(m_closeButton,
                         &QToolButton::clicked,
                         q,
                         &DockAreaTitleBar::onCloseButtonClicked);
    }

    void DockAreaTitleBarPrivate::createTabBar()
    {
        m_tabBar = componentsFactory()->createDockAreaTabBar(m_dockArea);
        m_tabBar->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        m_layout->addWidget(m_tabBar);
        QObject::connect(m_tabBar,
                         &DockAreaTabBar::tabClosed,
                         q,
                         &DockAreaTitleBar::markTabsMenuOutdated);
        QObject::connect(m_tabBar,
                         &DockAreaTabBar::tabOpened,
                         q,
                         &DockAreaTitleBar::markTabsMenuOutdated);
        QObject::connect(m_tabBar,
                         &DockAreaTabBar::tabInserted,
                         q,
                         &DockAreaTitleBar::markTabsMenuOutdated);
        QObject::connect(m_tabBar,
                         &DockAreaTabBar::removingTab,
                         q,
                         &DockAreaTitleBar::markTabsMenuOutdated);
        QObject::connect(m_tabBar,
                         &DockAreaTabBar::tabMoved,
                         q,
                         &DockAreaTitleBar::markTabsMenuOutdated);
        QObject::connect(m_tabBar,
                         &DockAreaTabBar::currentChanged,
                         q,
                         &DockAreaTitleBar::onCurrentTabChanged);
        QObject::connect(m_tabBar,
                         &DockAreaTabBar::tabBarClicked,
                         q,
                         &DockAreaTitleBar::tabBarClicked);
        QObject::connect(m_tabBar,
                         &DockAreaTabBar::elidedChanged,
                         q,
                         &DockAreaTitleBar::markTabsMenuOutdated);
    }

    AbstractFloatingWidget *DockAreaTitleBarPrivate::makeAreaFloating(const QPoint &offset,
                                                                      eDragState dragState)
    {
        QSize size = m_dockArea->size();
        m_dragState = dragState;
        bool opaqueUndocking = DockManager::testConfigFlag(DockManager::OpaqueUndocking)
                               || (DraggingFloatingWidget != dragState);
        FloatingDockContainer *floatingDockContainer = nullptr;
        AbstractFloatingWidget *floatingWidget;
        if (opaqueUndocking) {
            floatingWidget = floatingDockContainer = new FloatingDockContainer(m_dockArea);
        } else {
            auto w = new FloatingDragPreview(m_dockArea);
            QObject::connect(w, &FloatingDragPreview::draggingCanceled, [=]() {
                m_dragState = DraggingInactive;
            });
            floatingWidget = w;
        }

        floatingWidget->startFloating(offset, size, dragState, nullptr);
        if (floatingDockContainer) {
            auto topLevelDockWidget = floatingDockContainer->topLevelDockWidget();
            if (topLevelDockWidget) {
                topLevelDockWidget->emitTopLevelChanged(true);
            }
        }

        return floatingWidget;
    }

    void DockAreaTitleBarPrivate::startFloating(const QPoint &offset)
    {
        m_floatingWidget = makeAreaFloating(offset, DraggingFloatingWidget);
    }

    TitleBarButton::TitleBarButton(bool visible, QWidget *parent)
        : TitleBarButtonType(parent)
        , m_visible(visible)
        , m_hideWhenDisabled(DockAreaTitleBarPrivate::testConfigFlag(DockManager::DockAreaHideDisabledButtons))
    {}

    void TitleBarButton::setVisible(bool visible)
    {
        // 'visible' can stay 'true' if and only if this button is configured to generaly visible:
        visible = visible && m_visible;

        // 'visible' can stay 'true' unless: this button is configured to be invisible when it
        // is disabled and it is currently disabled:
        if (visible && m_hideWhenDisabled)
            visible = isEnabled();

        Super::setVisible(visible);
    }

    bool TitleBarButton::event(QEvent *event)
    {
        if (QEvent::EnabledChange == event->type() && m_hideWhenDisabled) {
            // force setVisible() call
            // Calling setVisible() directly here doesn't work well when button is expected to be shown first time
            QMetaObject::invokeMethod(this, "setVisible", Qt::QueuedConnection, Q_ARG(bool, isEnabled()));
        }

        return Super::event(event);
    }

    SpacerWidget::SpacerWidget(QWidget *parent)
        : QWidget(parent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setStyleSheet("border: none; background: none;");
    }

    DockAreaTitleBar::DockAreaTitleBar(DockAreaWidget *parent)
        : QFrame(parent)
        , d(new DockAreaTitleBarPrivate(this))
    {
        d->m_dockArea = parent;

        setObjectName("dockAreaTitleBar");
        d->m_layout = new QBoxLayout(QBoxLayout::LeftToRight);
        d->m_layout->setContentsMargins(0, 0, 0, 0);
        d->m_layout->setSpacing(0);
        setLayout(d->m_layout);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

        d->createTabBar();
        d->m_layout->addWidget(new SpacerWidget(this));
        d->createButtons();
    }

    DockAreaTitleBar::~DockAreaTitleBar() {
        if (!d->m_closeButton.isNull())
            delete d->m_closeButton;

        if (!d->m_tabsMenuButton.isNull())
            delete d->m_tabsMenuButton;

        if (!d->m_undockButton.isNull())
            delete d->m_undockButton;

        delete d;
    }

    DockAreaTabBar *DockAreaTitleBar::tabBar() const { return d->m_tabBar; }

    void DockAreaTitleBar::markTabsMenuOutdated() {
        if (DockAreaTitleBarPrivate::testConfigFlag(DockManager::DockAreaDynamicTabsMenuButtonVisibility)) {
            bool hasElidedTabTitle = false;
            for (int i = 0; i < d->m_tabBar->count(); ++i) {
                if (!d->m_tabBar->isTabOpen(i))
                    continue;

                DockWidgetTab* tab = d->m_tabBar->tab(i);
                if (tab->isTitleElided()) {
                    hasElidedTabTitle = true;
                    break;
                }
            }
            bool visible = (hasElidedTabTitle && (d->m_tabBar->count() > 1));
            QMetaObject::invokeMethod(d->m_tabsMenuButton, "setVisible", Qt::QueuedConnection, Q_ARG(bool, visible));
        }
        d->m_menuOutdated = true;
    }

    void DockAreaTitleBar::onTabsMenuAboutToShow()
    {
        if (!d->m_menuOutdated)
            return;

        QMenu *menu = d->m_tabsMenuButton->menu();
        menu->clear();
        for (int i = 0; i < d->m_tabBar->count(); ++i) {
            if (!d->m_tabBar->isTabOpen(i))
                continue;

            auto tab = d->m_tabBar->tab(i);
            QAction *action = menu->addAction(tab->icon(), tab->text());
            internal::setToolTip(action, tab->toolTip());
            action->setData(i);
        }

        d->m_menuOutdated = false;
    }

    void DockAreaTitleBar::onCloseButtonClicked()
    {
        qCInfo(adsLog) << Q_FUNC_INFO;
        if (d->testConfigFlag(DockManager::DockAreaCloseButtonClosesTab))
            d->m_tabBar->closeTab(d->m_tabBar->currentIndex());
        else
            d->m_dockArea->closeArea();
    }

    void DockAreaTitleBar::onUndockButtonClicked()
    {
        if (d->m_dockArea->features().testFlag(DockWidget::DockWidgetFloatable))
            d->makeAreaFloating(mapFromGlobal(QCursor::pos()), DraggingInactive);
    }

    void DockAreaTitleBar::onTabsMenuActionTriggered(QAction *action)
    {
        int index = action->data().toInt();
        d->m_tabBar->setCurrentIndex(index);
        emit tabBarClicked(index);
    }

    void DockAreaTitleBar::updateDockWidgetActionsButtons()
    {
        DockWidget* dockWidget = d->m_tabBar->currentTab()->dockWidget();
        if (!d->m_dockWidgetActionsButtons.isEmpty()) {
            for (auto button : d->m_dockWidgetActionsButtons) {
                d->m_layout->removeWidget(button);
                delete button;
            }
            d->m_dockWidgetActionsButtons.clear();
        }

        auto actions = dockWidget->titleBarActions();
        if (actions.isEmpty())
            return;

        int insertIndex = indexOf(d->m_tabsMenuButton);
        for (auto action : actions) {
            auto button = new TitleBarButton(true, this);
            button->setDefaultAction(action);
            button->setAutoRaise(true);
            button->setPopupMode(QToolButton::InstantPopup);
            button->setObjectName(action->objectName());
            d->m_layout->insertWidget(insertIndex++, button, 0);
            d->m_dockWidgetActionsButtons.append(button);
        }
    }

    void DockAreaTitleBar::onCurrentTabChanged(int index)
    {
        if (index < 0)
            return;

        if (d->testConfigFlag(DockManager::DockAreaCloseButtonClosesTab)) {
            DockWidget *dockWidget = d->m_tabBar->tab(index)->dockWidget();
            d->m_closeButton->setEnabled(
                dockWidget->features().testFlag(DockWidget::DockWidgetClosable));
        }

        updateDockWidgetActionsButtons();
    }

    QAbstractButton *DockAreaTitleBar::button(eTitleBarButton which) const
    {
        switch (which) {
        case TitleBarButtonTabsMenu:
            return d->m_tabsMenuButton;
        case TitleBarButtonUndock:
            return d->m_undockButton;
        case TitleBarButtonClose:
            return d->m_closeButton;
        }
        return nullptr;
    }

    void DockAreaTitleBar::setVisible(bool visible)
    {
        Super::setVisible(visible);
        markTabsMenuOutdated();
    }


    void DockAreaTitleBar::mousePressEvent(QMouseEvent *event)
    {
        if (event->button() == Qt::LeftButton) {
            event->accept();
            d->m_dragStartMousePos = event->pos();
            d->m_dragState = DraggingMousePressed;
            if (DockManager::testConfigFlag(DockManager::FocusHighlighting))
                d->m_tabBar->currentTab()->setFocus(Qt::OtherFocusReason);

            return;
        }
        Super::mousePressEvent(event);
    }

    void DockAreaTitleBar::mouseReleaseEvent(QMouseEvent *event)
    {
        if (event->button() == Qt::LeftButton) {
            qCInfo(adsLog) << Q_FUNC_INFO;
            event->accept();
            auto CurrentDragState = d->m_dragState;
            d->m_dragStartMousePos = QPoint();
            d->m_dragState = DraggingInactive;
            if (DraggingFloatingWidget == CurrentDragState)
                d->m_floatingWidget->finishDragging();

            return;
        }
        Super::mouseReleaseEvent(event);
    }

    void DockAreaTitleBar::mouseMoveEvent(QMouseEvent *event)
    {
        Super::mouseMoveEvent(event);
        if (!(event->buttons() & Qt::LeftButton) || d->isDraggingState(DraggingInactive)) {
            d->m_dragState = DraggingInactive;
            return;
        }

        // move floating window
        if (d->isDraggingState(DraggingFloatingWidget)) {
            d->m_floatingWidget->moveFloating();
            return;
        }

        // If this is the last dock area in a floating dock container it does not make
        // sense to move it to a new floating widget and leave this one empty
        if (d->m_dockArea->dockContainer()->isFloating()
            && d->m_dockArea->dockContainer()->visibleDockAreaCount() == 1) {
            return;
        }

        // If one single dock widget in this area is not floatable then the whole
        // area is not floatable
        // If we do non opaque undocking, then we can create the floating drag
        // preview if the dock widget is movable
        auto features = d->m_dockArea->features();
        if (!features.testFlag(DockWidget::DockWidgetFloatable)
            && !(features.testFlag(DockWidget::DockWidgetMovable)
            && !DockManager::testConfigFlag(DockManager::OpaqueUndocking))) {
            return;
        }

        int dragDistance = (d->m_dragStartMousePos - event->pos()).manhattanLength();
        if (dragDistance >= DockManager::startDragDistance()) {
            qCInfo(adsLog) << "DockAreaTitlBar::startFloating";
            d->startFloating(d->m_dragStartMousePos);
            auto overlay = d->m_dockArea->dockManager()->containerOverlay();
            overlay->setAllowedAreas(OuterDockAreas);
        }

        return;
    }

    void DockAreaTitleBar::mouseDoubleClickEvent(QMouseEvent *event)
    {
        // If this is the last dock area in a dock container it does not make
        // sense to move it to a new floating widget and leave this one empty
        if (d->m_dockArea->dockContainer()->isFloating()
            && d->m_dockArea->dockContainer()->dockAreaCount() == 1)
            return;

        if (!d->m_dockArea->features().testFlag(DockWidget::DockWidgetFloatable))
            return;

        d->makeAreaFloating(event->pos(), DraggingInactive);
    }

    void DockAreaTitleBar::contextMenuEvent(QContextMenuEvent *event)
    {
        event->accept();
        if (d->isDraggingState(DraggingFloatingWidget))
            return;

        QMenu menu(this);
        auto action = menu.addAction(tr("Detach Area"),
                                     this,
                                     &DockAreaTitleBar::onUndockButtonClicked);
        action->setEnabled(d->m_dockArea->features().testFlag(DockWidget::DockWidgetFloatable));
        menu.addSeparator();
        action = menu.addAction(tr("Close Area"), this, &DockAreaTitleBar::onCloseButtonClicked);
        action->setEnabled(d->m_dockArea->features().testFlag(DockWidget::DockWidgetClosable));
        menu.addAction(tr("Close Other Areas"), d->m_dockArea, &DockAreaWidget::closeOtherAreas);
        menu.exec(event->globalPos());
    }

    void DockAreaTitleBar::insertWidget(int index, QWidget *widget)
    {
        d->m_layout->insertWidget(index, widget);
    }

    int DockAreaTitleBar::indexOf(QWidget *widget) const
    {
        return d->m_layout->indexOf(widget);
    }

} // namespace ADS
