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

#include "dockwidget.h"

#include "ads_globals.h"
#include "dockareawidget.h"
#include "dockcomponentsfactory.h"
#include "dockcontainerwidget.h"
#include "dockmanager.h"
#include "docksplitter.h"
#include "dockwidgettab.h"
#include "floatingdockcontainer.h"

#include <QAction>
#include <QBoxLayout>
#include <QEvent>
#include <QLoggingCategory>
#include <QPointer>
#include <QScrollArea>
#include <QSplitter>
#include <QStack>
#include <QTextStream>
#include <QToolBar>
#include <QXmlStreamWriter>
#include <QWindow>

static Q_LOGGING_CATEGORY(adsLog, "qtc.qmldesigner.advanceddockingsystem", QtWarningMsg)

namespace ADS
{
    /**
     * Private data class of DockWidget class (pimpl)
     */
    class DockWidgetPrivate
    {
    public:
        DockWidget *q = nullptr;
        QBoxLayout *m_layout = nullptr;
        QWidget *m_widget = nullptr;
        DockWidgetTab *m_tabWidget = nullptr;
        DockWidget::DockWidgetFeatures m_features = DockWidget::DefaultDockWidgetFeatures;
        DockManager *m_dockManager = nullptr;
        DockAreaWidget *m_dockArea = nullptr;
        QAction *m_toggleViewAction = nullptr;
        bool m_closed = false;
        QScrollArea *m_scrollArea = nullptr;
        QToolBar *m_toolBar = nullptr;
        Qt::ToolButtonStyle m_toolBarStyleDocked = Qt::ToolButtonIconOnly;
        Qt::ToolButtonStyle m_toolBarStyleFloating = Qt::ToolButtonTextUnderIcon;
        QSize m_toolBarIconSizeDocked = QSize(16, 16);
        QSize m_toolBarIconSizeFloating = QSize(24, 24);
        bool m_isFloatingTopLevel = false;
        QList<QAction *> m_titleBarActions;
        DockWidget::eMinimumSizeHintMode m_minimumSizeHintMode = DockWidget::MinimumSizeHintFromDockWidget;

        /**
         * Private data constructor
         */
        DockWidgetPrivate(DockWidget *parent);

        /**
         * Show dock widget
         */
        void showDockWidget();

        /**
         * Hide dock widget.
         */
        void hideDockWidget();

        /**
         * Hides a dock area if all dock widgets in the area are closed.
         * This function updates the current selected tab and hides the parent
         * dock area if it is empty
         */
        void updateParentDockArea();

        /**
         * Setup the top tool bar
         */
        void setupToolBar();

        /**
         * Setup the main scroll area
         */
        void setupScrollArea();
    }; // class DockWidgetPrivate

    DockWidgetPrivate::DockWidgetPrivate(DockWidget *parent)
        : q(parent)
    {}

    void DockWidgetPrivate::showDockWidget()
    {
        if (!m_dockArea) {
            FloatingDockContainer *floatingWidget = new FloatingDockContainer(q);
            floatingWidget->resize(q->size());
            m_tabWidget->show();
            floatingWidget->show();
        } else {
            m_dockArea->setCurrentDockWidget(q);
            m_dockArea->toggleView(true);
            m_tabWidget->show();
            QSplitter *splitter = internal::findParent<QSplitter *>(m_dockArea);
            while (splitter && !splitter->isVisible()) {
                splitter->show();
                splitter = internal::findParent<QSplitter *>(splitter);
            }

            DockContainerWidget *container = m_dockArea->dockContainer();
            if (container->isFloating()) {
                FloatingDockContainer *floatingWidget
                    = internal::findParent<FloatingDockContainer *>(container);
                floatingWidget->show();
            }
        }
    }

    void DockWidgetPrivate::hideDockWidget()
    {
        m_tabWidget->hide();
        updateParentDockArea();
    }

    void DockWidgetPrivate::updateParentDockArea()
    {
        if (!m_dockArea)
            return;

        // we don't need to change the current tab if the current DockWidget is not the one being closed
        if (m_dockArea->currentDockWidget() != q)
            return;

        auto nextDockWidget = m_dockArea->nextOpenDockWidget(q);
        if (nextDockWidget)
            m_dockArea->setCurrentDockWidget(nextDockWidget);
        else
            m_dockArea->hideAreaWithNoVisibleContent();
    }

    void DockWidgetPrivate::setupToolBar()
    {
        m_toolBar = new QToolBar(q);
        m_toolBar->setObjectName("dockWidgetToolBar");
        m_layout->insertWidget(0, m_toolBar);
        m_toolBar->setIconSize(QSize(16, 16));
        m_toolBar->toggleViewAction()->setEnabled(false);
        m_toolBar->toggleViewAction()->setVisible(false);
        QObject::connect(q, &DockWidget::topLevelChanged, q, &DockWidget::setToolbarFloatingStyle);
    }

    void DockWidgetPrivate::setupScrollArea()
    {
        m_scrollArea = new QScrollArea(q);
        m_scrollArea->setObjectName("dockWidgetScrollArea");
        m_scrollArea->setWidgetResizable(true);
        m_layout->addWidget(m_scrollArea);
    }

    DockWidget::DockWidget(const QString &uniqueId, QWidget *parent)
        : QFrame(parent)
        , d(new DockWidgetPrivate(this))
    {
        d->m_layout = new QBoxLayout(QBoxLayout::TopToBottom);
        d->m_layout->setContentsMargins(0, 0, 0, 0);
        d->m_layout->setSpacing(0);
        setLayout(d->m_layout);
        setWindowTitle(uniqueId); // temporarily use unique id as title
        setObjectName(uniqueId);

        d->m_tabWidget = componentsFactory()->createDockWidgetTab(this);
        d->m_toggleViewAction = new QAction(uniqueId, this);
        d->m_toggleViewAction->setCheckable(true);
        connect(d->m_toggleViewAction, &QAction::triggered, this, &DockWidget::toggleView);
        setToolbarFloatingStyle(false);

        if (DockManager::testConfigFlag(DockManager::FocusHighlighting))
            setFocusPolicy(Qt::ClickFocus);
    }

    DockWidget::~DockWidget()
    {
        qCInfo(adsLog) << Q_FUNC_INFO;
        delete d;
    }

    void DockWidget::setToggleViewActionChecked(bool checked)
    {
        QAction *action = d->m_toggleViewAction;
        //action->blockSignals(true);
        action->setChecked(checked);
        //action->blockSignals(false);
    }

    void DockWidget::setWidget(QWidget *widget, eInsertMode insertMode)
    {
        if (d->m_widget)
            takeWidget();

        auto scrollAreaWidget = qobject_cast<QAbstractScrollArea *>(widget);
        if (scrollAreaWidget || ForceNoScrollArea == insertMode) {
            d->m_layout->addWidget(widget);
            if (scrollAreaWidget && scrollAreaWidget->viewport())
                scrollAreaWidget->viewport()->setProperty("dockWidgetContent", true);
        } else {
            d->setupScrollArea();
            d->m_scrollArea->setWidget(widget);
        }

        d->m_widget = widget;
        d->m_widget->setProperty("dockWidgetContent", true);
    }

    QWidget *DockWidget::takeWidget()
    {
        QWidget *w = nullptr;
        if (d->m_scrollArea) {
            d->m_layout->removeWidget(d->m_scrollArea);
            w = d->m_scrollArea->takeWidget();
            delete d->m_scrollArea;
            d->m_scrollArea = nullptr;
            d->m_widget = nullptr;
        } else if (d->m_widget) {
            d->m_layout->removeWidget(d->m_widget);
            w = d->m_widget;
            d->m_widget = nullptr;
        }

        if (w)
            w->setParent(nullptr);

        return w;
    }

    QWidget *DockWidget::widget() const { return d->m_widget; }

    DockWidgetTab *DockWidget::tabWidget() const { return d->m_tabWidget; }

    void DockWidget::setFeatures(DockWidgetFeatures features)
    {
        if (d->m_features == features)
            return;

        d->m_features = features;
        emit featuresChanged(d->m_features);
        d->m_tabWidget->onDockWidgetFeaturesChanged();
    }

    void DockWidget::setFeature(DockWidgetFeature flag, bool on)
    {
        auto currentFeatures = features();
        internal::setFlag(currentFeatures, flag, on);
        setFeatures(currentFeatures);
    }

    DockWidget::DockWidgetFeatures DockWidget::features() const { return d->m_features; }

    DockManager *DockWidget::dockManager() const { return d->m_dockManager; }

    void DockWidget::setDockManager(DockManager *dockManager) { d->m_dockManager = dockManager; }

    DockContainerWidget *DockWidget::dockContainer() const
    {
        if (d->m_dockArea)
            return d->m_dockArea->dockContainer();
        else
            return nullptr;
    }

    DockAreaWidget *DockWidget::dockAreaWidget() const { return d->m_dockArea; }

    bool DockWidget::isFloating() const
    {
        if (!isInFloatingContainer())
            return false;

        return dockContainer()->topLevelDockWidget() == this;
    }

    bool DockWidget::isInFloatingContainer() const
    {
        auto container = dockContainer();
        if (!container)
            return false;

        if (!container->isFloating())
            return false;

        return true;
    }

    bool DockWidget::isClosed() const { return d->m_closed; }

    QAction *DockWidget::toggleViewAction() const { return d->m_toggleViewAction; }

    void DockWidget::setToggleViewActionMode(eToggleViewActionMode mode)
    {
        if (ActionModeToggle == mode) {
            d->m_toggleViewAction->setCheckable(true);
            d->m_toggleViewAction->setIcon(QIcon());
        } else {
            d->m_toggleViewAction->setCheckable(false);
            d->m_toggleViewAction->setIcon(d->m_tabWidget->icon());
        }
    }

    void DockWidget::setMinimumSizeHintMode(eMinimumSizeHintMode mode)
    {
        d->m_minimumSizeHintMode = mode;
    }

    void DockWidget::toggleView(bool open)
    {
        // If the toggle view action mode is ActionModeShow, then Open is always
        // true if the sender is the toggle view action
        QAction *action = qobject_cast<QAction *>(sender());
        if (action == d->m_toggleViewAction && !d->m_toggleViewAction->isCheckable())
            open = true;

        // If the dock widget state is different, then we really need to toggle
        // the state. If we are in the right state, then we simply make this
        // dock widget the current dock widget
        if (d->m_closed != !open)
            toggleViewInternal(open);
        else if (open && d->m_dockArea)
            d->m_dockArea->setCurrentDockWidget(this);
    }

    void DockWidget::toggleViewInternal(bool open)
    {
        const DockContainerWidget *const beforeDockContainerWidget = dockContainer();
        DockWidget *topLevelDockWidgetBefore = beforeDockContainerWidget
                                                   ? beforeDockContainerWidget->topLevelDockWidget()
                                                   : nullptr;

        if (open)
            d->showDockWidget();
        else
            d->hideDockWidget();

        d->m_closed = !open;
        //d->m_toggleViewAction->blockSignals(true);
        d->m_toggleViewAction->setChecked(open);
        //d->m_toggleViewAction->blockSignals(false);
        if (d->m_dockArea)
            d->m_dockArea->toggleDockWidgetView(this, open);

        if (open && topLevelDockWidgetBefore)
            DockWidget::emitTopLevelEventForWidget(topLevelDockWidgetBefore, false);

        // Here we need to call the dockContainer() function again, because if
        // this dock widget was unassigned before the call to showDockWidget() then
        // it has a dock container now
        const DockContainerWidget *const dockContainerWidget = dockContainer();
        DockWidget *topLevelDockWidgetAfter = dockContainerWidget
                                                  ? dockContainerWidget->topLevelDockWidget()
                                                  : nullptr;
        DockWidget::emitTopLevelEventForWidget(topLevelDockWidgetAfter, true);
        FloatingDockContainer *floatingContainer = dockContainerWidget
                                                       ? dockContainerWidget->floatingWidget()
                                                       : nullptr;
        if (floatingContainer)
            floatingContainer->updateWindowTitle();

        if (!open)
            emit closed();

        emit viewToggled(open);
    }

    void DockWidget::setDockArea(DockAreaWidget *dockArea)
    {
        d->m_dockArea = dockArea;
        d->m_toggleViewAction->setChecked(dockArea != nullptr && !this->isClosed());
    }

    void DockWidget::saveState(QXmlStreamWriter &stream) const
    {
        stream.writeStartElement("widget");
        stream.writeAttribute("name", objectName());
        stream.writeAttribute("closed", QVariant::fromValue(d->m_closed).toString());
        stream.writeEndElement();
    }

    void DockWidget::flagAsUnassigned()
    {
        d->m_closed = true;
        setParent(d->m_dockManager);
        setVisible(false);
        setDockArea(nullptr);
        tabWidget()->setParent(this);
    }

    bool DockWidget::event(QEvent *event)
    {
        switch (event->type()) {
        case QEvent::Hide:
            emit visibilityChanged(false);
            break;

        case QEvent::Show:
            emit visibilityChanged(geometry().right() >= 0 && geometry().bottom() >= 0);
            break;

        case QEvent::WindowTitleChange :
            {
                const auto title = windowTitle();
                if (d->m_tabWidget) {
                    d->m_tabWidget->setText(title);
                }
                if (d->m_toggleViewAction) {
                    d->m_toggleViewAction->setText(title);
                }
                if (d->m_dockArea) {
                    d->m_dockArea->markTitleBarMenuOutdated(); // update tabs menu
                }
                emit titleChanged(title);
            }
            break;

        default:
            break;
        }

        return Super::event(event);
    }

#ifndef QT_NO_TOOLTIP
    void DockWidget::setTabToolTip(const QString &text)
    {
        if (d->m_tabWidget)
            d->m_tabWidget->setToolTip(text);

        if (d->m_toggleViewAction)
            d->m_toggleViewAction->setToolTip(text);

        if (d->m_dockArea)
            d->m_dockArea->markTitleBarMenuOutdated(); // update tabs menu
    }
#endif

    void DockWidget::setIcon(const QIcon &icon)
    {
        d->m_tabWidget->setIcon(icon);
        if (!d->m_toggleViewAction->isCheckable())
            d->m_toggleViewAction->setIcon(icon);
    }

    QIcon DockWidget::icon() const { return d->m_tabWidget->icon(); }

    QToolBar *DockWidget::toolBar() const { return d->m_toolBar; }

    QToolBar *DockWidget::createDefaultToolBar()
    {
        if (!d->m_toolBar)
            d->setupToolBar();

        return d->m_toolBar;
    }

    void DockWidget::setToolBar(QToolBar *toolBar)
    {
        if (d->m_toolBar)
            delete d->m_toolBar;

        d->m_toolBar = toolBar;
        d->m_layout->insertWidget(0, d->m_toolBar);
        connect(this, &DockWidget::topLevelChanged, this, &DockWidget::setToolbarFloatingStyle);
        setToolbarFloatingStyle(isFloating());
    }

    void DockWidget::setToolBarStyle(Qt::ToolButtonStyle style, eState state)
    {
        if (StateFloating == state)
            d->m_toolBarStyleFloating = style;
        else
            d->m_toolBarStyleDocked = style;

        setToolbarFloatingStyle(isFloating());
    }

    Qt::ToolButtonStyle DockWidget::toolBarStyle(eState state) const
    {
        if (StateFloating == state)
            return d->m_toolBarStyleFloating;
        else
            return d->m_toolBarStyleDocked;
    }

    void DockWidget::setToolBarIconSize(const QSize &iconSize, eState state)
    {
        if (StateFloating == state)
            d->m_toolBarIconSizeFloating = iconSize;
        else
            d->m_toolBarIconSizeDocked = iconSize;

        setToolbarFloatingStyle(isFloating());
    }

    QSize DockWidget::toolBarIconSize(eState state) const
    {
        if (StateFloating == state)
            return d->m_toolBarIconSizeFloating;
        else
            return d->m_toolBarIconSizeDocked;
    }

    void DockWidget::setToolbarFloatingStyle(bool floating)
    {
        if (!d->m_toolBar)
            return;

        auto iconSize = floating ? d->m_toolBarIconSizeFloating : d->m_toolBarIconSizeDocked;
        if (iconSize != d->m_toolBar->iconSize())
            d->m_toolBar->setIconSize(iconSize);

        auto buttonStyle = floating ? d->m_toolBarStyleFloating : d->m_toolBarStyleDocked;
        if (buttonStyle != d->m_toolBar->toolButtonStyle())
            d->m_toolBar->setToolButtonStyle(buttonStyle);
    }

    void DockWidget::emitTopLevelEventForWidget(DockWidget *topLevelDockWidget, bool floating)
    {
        if (topLevelDockWidget) {
            topLevelDockWidget->dockAreaWidget()->updateTitleBarVisibility();
            topLevelDockWidget->emitTopLevelChanged(floating);
        }
    }

    void DockWidget::emitTopLevelChanged(bool floating)
    {
        if (floating != d->m_isFloatingTopLevel) {
            d->m_isFloatingTopLevel = floating;
            emit topLevelChanged(d->m_isFloatingTopLevel);
        }
    }

    void DockWidget::setClosedState(bool closed) { d->m_closed = closed; }

    QSize DockWidget::minimumSizeHint() const
    {
        if (d->m_minimumSizeHintMode == DockWidget::MinimumSizeHintFromDockWidget || !d->m_widget)
            return QSize(60, 40);
        else
            return d->m_widget->minimumSizeHint();
    }

    void DockWidget::setFloating()
    {
        if (isClosed())
            return;

        d->m_tabWidget->detachDockWidget();
    }

    void DockWidget::deleteDockWidget()
    {
        dockManager()->removeDockWidget(this);
        deleteLater();
        d->m_closed = true;
    }

    void DockWidget::closeDockWidget()
    {
        closeDockWidgetInternal(true);
    }

    bool DockWidget::closeDockWidgetInternal(bool forceClose)
    {
        if (!forceClose)
            emit closeRequested();

        if (!forceClose && features().testFlag(DockWidget::CustomCloseHandling))
            return false;

        if (features().testFlag(DockWidget::DockWidgetDeleteOnClose)) {
            // If the dock widget is floating, then we check if we also need to
            // delete the floating widget
            if (isFloating()) {
                FloatingDockContainer* floatingWidget = internal::findParent<
                        FloatingDockContainer *>(this);
                if (floatingWidget->dockWidgets().count() == 1)
                    floatingWidget->deleteLater();
                else
                    floatingWidget->hide();
            }
            deleteDockWidget();
            emit closed();
        } else {
            toggleView(false);
        }

        return true;
    }

    void DockWidget::setTitleBarActions(QList<QAction *> actions)
    {
        d->m_titleBarActions = actions;
    }

    QList<QAction *> DockWidget::titleBarActions() const
    {
        return d->m_titleBarActions;
    }

    void DockWidget::showFullScreen()
    {
        if (isFloating())
            dockContainer()->floatingWidget()->showFullScreen();
        else
            Super::showFullScreen();
    }

    void DockWidget::showNormal()
    {
        if (isFloating())
            dockContainer()->floatingWidget()->showNormal();
        else
            Super::showNormal();
    }

    bool DockWidget::isFullScreen() const
    {
        if (isFloating())
            return dockContainer()->floatingWidget()->isFullScreen();
        else
            return Super::isFullScreen();
    }

    void DockWidget::setAsCurrentTab()
    {
        if (d->m_dockArea && !isClosed())
            d->m_dockArea->setCurrentDockWidget(this);
    }

    bool DockWidget::isTabbed() const
    {
        return d->m_dockArea && (d->m_dockArea->openDockWidgetsCount() > 1);
    }

    bool DockWidget::isCurrentTab() const
    {
        return d->m_dockArea && (d->m_dockArea->currentDockWidget() == this);
    }

    void DockWidget::raise()
    {
        if (isClosed())
            return;

        setAsCurrentTab();
        if (isInFloatingContainer())
        {
            auto floatingWindow = window();
            floatingWindow->raise();
            floatingWindow->activateWindow();
        }
    }

} // namespace ADS
