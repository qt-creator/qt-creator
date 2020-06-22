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

#include "dockwidgettab.h"

#include "ads_globals.h"
#include "dockareawidget.h"
#include "dockmanager.h"
#include "dockoverlay.h"
#include "dockwidget.h"
#include "elidinglabel.h"
#include "floatingdockcontainer.h"
#include "floatingdragpreview.h"
#include "iconprovider.h"

#include <QApplication>
#include <QBoxLayout>
#include <QLabel>
#include <QLoggingCategory>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QSplitter>
#include <QStyle>
#include <QToolButton>

#include <iostream>

static Q_LOGGING_CATEGORY(adsLog, "qtc.qmldesigner.advanceddockingsystem", QtWarningMsg)

namespace ADS
{
    using TabLabelType = ElidingLabel;

    /**
     * Private data class of DockWidgetTab class (pimpl)
     */
    class DockWidgetTabPrivate
    {
    public:
        DockWidgetTab *q;
        DockWidget *m_dockWidget = nullptr;
        QLabel *m_iconLabel = nullptr;
        TabLabelType *m_titleLabel = nullptr;
        QPoint m_globalDragStartMousePosition;
        QPoint m_dragStartMousePosition;
        bool m_isActiveTab = false;
        DockAreaWidget *m_dockArea = nullptr;
        eDragState m_dragState = DraggingInactive;
        AbstractFloatingWidget *m_floatingWidget = nullptr;
        QIcon m_icon;
        QAbstractButton *m_closeButton = nullptr;
        QPoint m_tabDragStartPosition;

        /**
         * Private data constructor
         */
        DockWidgetTabPrivate(DockWidgetTab *parent);

        /**
         * Creates the complete layout including all controls
         */
        void createLayout();

        /**
         * Moves the tab depending on the position in the given mouse event
         */
        void moveTab(QMouseEvent *event);

        /**
         * Test function for current drag state
         */
        bool isDraggingState(eDragState dragState) const { return this->m_dragState == dragState; }

        /**
         * Starts floating of the dock widget that belongs to this title bar
         * Returns true, if floating has been started and false if floating
         * is not possible for any reason
         */
        bool startFloating(eDragState draggingState = DraggingFloatingWidget);

        /**
         * Returns true if the given config flag is set
         */
        bool testConfigFlag(DockManager::eConfigFlag flag) const
        {
            return DockManager::testConfigFlag(flag);
        }

        /**
         * Creates the close button as QPushButton or as QToolButton
         */
        QAbstractButton *createCloseButton() const
        {
            if (testConfigFlag(DockManager::TabCloseButtonIsToolButton)) {
                auto button = new QToolButton();
                button->setAutoRaise(true);
                return button;
            } else {
                return new QPushButton();
            }
        }

        template<typename T>
        AbstractFloatingWidget *createFloatingWidget(T *widget, bool opaqueUndocking)
        {
            if (opaqueUndocking) {
                return new FloatingDockContainer(widget);
            } else {
                auto w = new FloatingDragPreview(widget);
                QObject::connect(w, &FloatingDragPreview::draggingCanceled, q, [=]() {
                    m_dragState = DraggingInactive;
                });
                return w;
            }
        }

        /**
         * Saves the drag start position in global and local coordinates
         */
        void saveDragStartMousePosition(const QPoint &globalPos)
        {
            m_globalDragStartMousePosition = globalPos;
            m_dragStartMousePosition = q->mapFromGlobal(globalPos);
        }
    }; // class DockWidgetTabPrivate

    DockWidgetTabPrivate::DockWidgetTabPrivate(DockWidgetTab *parent)
        : q(parent)
    {}

    void DockWidgetTabPrivate::createLayout()
    {
        m_titleLabel = new TabLabelType();
        m_titleLabel->setElideMode(Qt::ElideRight);
        m_titleLabel->setText(m_dockWidget->windowTitle());
        m_titleLabel->setObjectName("dockWidgetTabLabel");
        m_titleLabel->setAlignment(Qt::AlignCenter);
        QObject::connect(m_titleLabel,
                         &ElidingLabel::elidedChanged,
                         q,
                         &DockWidgetTab::elidedChanged);

        m_closeButton = createCloseButton();
        m_closeButton->setObjectName("tabCloseButton");
        internal::setButtonIcon(m_closeButton,
                                QStyle::SP_TitleBarCloseButton,
                                TabCloseIcon);
        m_closeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_closeButton->setIconSize(QSize(14, 14));
        q->onDockWidgetFeaturesChanged();
        internal::setToolTip(m_closeButton, QObject::tr("Close Tab"));
        QObject::connect(m_closeButton,
                         &QAbstractButton::clicked,
                         q,
                         &DockWidgetTab::closeRequested);

        QFontMetrics fontMetrics(m_titleLabel->font());
        int spacing = qRound(fontMetrics.height() / 4.0);

        // Fill the layout
        QBoxLayout *boxLayout = new QBoxLayout(QBoxLayout::LeftToRight);
        boxLayout->setContentsMargins(2 * spacing, 0, 0, 0);
        boxLayout->setSpacing(0);
        q->setLayout(boxLayout);
        boxLayout->addWidget(m_titleLabel, 1, Qt::AlignVCenter);
        boxLayout->addSpacing(spacing);
        boxLayout->addWidget(m_closeButton, 0, Qt::AlignVCenter);
        boxLayout->addSpacing(qRound(spacing * 4.0 / 3.0));
        boxLayout->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);

        m_titleLabel->setVisible(true);
    }

    void DockWidgetTabPrivate::moveTab(QMouseEvent *event)
    {
        event->accept();
        QPoint distance = event->globalPos() - m_globalDragStartMousePosition;
        distance.setY(0);
        auto targetPos = distance + m_tabDragStartPosition;
        targetPos.rx() = qMax(targetPos.x(), 0);
        targetPos.rx() = qMin(q->parentWidget()->rect().right() - q->width() + 1, targetPos.rx());
        q->move(targetPos);
        q->raise();
    }

    bool DockWidgetTabPrivate::startFloating(eDragState draggingState)
    {
        auto dockContainer = m_dockWidget->dockContainer();
        qCInfo(adsLog) << "isFloating " << dockContainer->isFloating();
        qCInfo(adsLog) << "areaCount " << dockContainer->dockAreaCount();
        qCInfo(adsLog) << "widgetCount " << m_dockWidget->dockAreaWidget()->dockWidgetsCount();
        // if this is the last dock widget inside of this floating widget,
        // then it does not make any sense, to make it floating because
        // it is already floating
        if (dockContainer->isFloating() && (dockContainer->visibleDockAreaCount() == 1)
            && (m_dockWidget->dockAreaWidget()->dockWidgetsCount() == 1)) {
            return false;
        }

        qCInfo(adsLog) << "startFloating";
        m_dragState = draggingState;
        AbstractFloatingWidget *floatingWidget = nullptr;
        bool opaqueUndocking = DockManager::testConfigFlag(DockManager::OpaqueUndocking)
                               || (DraggingFloatingWidget != draggingState);

        // If section widget has multiple tabs, we take only one tab
        // If it has only one single tab, we can move the complete
        // dock area into floating widget
        QSize size;
        if (m_dockArea->dockWidgetsCount() > 1) {
            floatingWidget = createFloatingWidget(m_dockWidget, opaqueUndocking);
            size = m_dockWidget->size();
        } else {
            floatingWidget = createFloatingWidget(m_dockArea, opaqueUndocking);
            size = m_dockArea->size();
        }

        if (DraggingFloatingWidget == draggingState) {
            floatingWidget->startFloating(m_dragStartMousePosition, size, DraggingFloatingWidget, q);
            auto Overlay = m_dockWidget->dockManager()->containerOverlay();
            Overlay->setAllowedAreas(OuterDockAreas);
            this->m_floatingWidget = floatingWidget;
        } else {
            floatingWidget->startFloating(m_dragStartMousePosition, size, DraggingInactive, nullptr);
        }

        return true;
    }

    DockWidgetTab::DockWidgetTab(DockWidget *dockWidget, QWidget *parent)
        : QFrame(parent)
        , d(new DockWidgetTabPrivate(this))
    {
        setAttribute(Qt::WA_NoMousePropagation, true);
        d->m_dockWidget = dockWidget;
        d->createLayout();
        if (DockManager::testConfigFlag(DockManager::FocusHighlighting))
            setFocusPolicy(Qt::ClickFocus);
    }

    DockWidgetTab::~DockWidgetTab()
    {
        qCInfo(adsLog) << Q_FUNC_INFO;
        delete d;
    }

    void DockWidgetTab::mousePressEvent(QMouseEvent *event)
    {
        if (event->button() == Qt::LeftButton) {
            event->accept();
            d->saveDragStartMousePosition(event->globalPos());
            d->m_dragState = DraggingMousePressed;
            emit clicked();
            return;
        }
        Super::mousePressEvent(event);
    }

    void DockWidgetTab::mouseReleaseEvent(QMouseEvent *event)
    {
        if (event->button() == Qt::LeftButton) {
            auto currentDragState = d->m_dragState;
            d->m_globalDragStartMousePosition = QPoint();
            d->m_dragStartMousePosition = QPoint();
            d->m_dragState = DraggingInactive;

            switch (currentDragState) {
            case DraggingTab:
                // End of tab moving, emit signal
                if (d->m_dockArea) {
                    emit moved(event->globalPos());
                }
                break;

            case DraggingFloatingWidget:
                d->m_floatingWidget->finishDragging();
                break;

            default:; // do nothing
            }
        }

        Super::mouseReleaseEvent(event);
    }

    void DockWidgetTab::mouseMoveEvent(QMouseEvent *event)
    {
        if (!(event->buttons() & Qt::LeftButton) || d->isDraggingState(DraggingInactive)) {
            d->m_dragState = DraggingInactive;
            Super::mouseMoveEvent(event);
            return;
        }

        // move floating window
        if (d->isDraggingState(DraggingFloatingWidget)) {
            d->m_floatingWidget->moveFloating();
            Super::mouseMoveEvent(event);
            return;
        }

        // move tab
        if (d->isDraggingState(DraggingTab)) {
            // Moving the tab is always allowed because it does not mean moving the
            // dock widget around
            d->moveTab(event);
        }

        auto mappedPos = mapToParent(event->pos());
        bool mouseOutsideBar = (mappedPos.x() < 0) || (mappedPos.x() > parentWidget()->rect().right());
        // Maybe a fixed drag distance is better here ?
        int dragDistanceY = qAbs(d->m_globalDragStartMousePosition.y() - event->globalPos().y());
        if (dragDistanceY >= DockManager::startDragDistance() || mouseOutsideBar) {
            // If this is the last dock area in a dock container with only
            // one single dock widget it does not make  sense to move it to a new
            // floating widget and leave this one empty
            if (d->m_dockArea->dockContainer()->isFloating()
                && d->m_dockArea->openDockWidgetsCount() == 1
                && d->m_dockArea->dockContainer()->visibleDockAreaCount() == 1) {
                return;
            }

            // Floating is only allowed for widgets that are floatable
            // If we do non opaque undocking, then can create the drag preview
            // if the widget is movable.
            auto features = d->m_dockWidget->features();
            if (features.testFlag(DockWidget::DockWidgetFloatable)
                || (features.testFlag(DockWidget::DockWidgetMovable)
                && !DockManager::testConfigFlag(DockManager::OpaqueUndocking))) {
                // If we undock, we need to restore the initial position of this
                // tab because it looks strange if it remains on its dragged position
                if (d->isDraggingState(DraggingTab)
                    && !DockManager::testConfigFlag(DockManager::OpaqueUndocking))
                    parentWidget()->layout()->update();

                d->startFloating();
            }
            return;
        } else if (d->m_dockArea->openDockWidgetsCount() > 1
                   && (event->globalPos() - d->m_globalDragStartMousePosition).manhattanLength()
                          >= QApplication::startDragDistance()) // Wait a few pixels before start moving
        {
            // If we start dragging the tab, we save its initial position to
            // restore it later
            if (DraggingTab != d->m_dragState)
                d->m_tabDragStartPosition = this->pos();

            d->m_dragState = DraggingTab;
            return;
        }

        Super::mouseMoveEvent(event);
    }

    void DockWidgetTab::contextMenuEvent(QContextMenuEvent *event)
    {
        event->accept();
        if (d->isDraggingState(DraggingFloatingWidget))
            return;

        d->saveDragStartMousePosition(event->globalPos());
        QMenu menu(this);

        const bool isFloatable = d->m_dockWidget->features().testFlag(DockWidget::DockWidgetFloatable);
        const bool isNotOnlyTabInContainer =  !d->m_dockArea->dockContainer()->hasTopLevelDockWidget();
        const bool isDetachable = isFloatable && isNotOnlyTabInContainer;

        auto action = menu.addAction(tr("Detach"), this, &DockWidgetTab::detachDockWidget);
        action->setEnabled(isDetachable);
        menu.addSeparator();
        action = menu.addAction(tr("Close"), this, &DockWidgetTab::closeRequested);
        action->setEnabled(isClosable());
        menu.addAction(tr("Close Others"), this, &DockWidgetTab::closeOtherTabsRequested);
        menu.exec(event->globalPos());
    }

    bool DockWidgetTab::isActiveTab() const { return d->m_isActiveTab; }

    void DockWidgetTab::setActiveTab(bool active)
    {
        bool dockWidgetClosable = d->m_dockWidget->features().testFlag(
            DockWidget::DockWidgetClosable);
        bool activeTabHasCloseButton = d->testConfigFlag(DockManager::ActiveTabHasCloseButton);
        bool allTabsHaveCloseButton = d->testConfigFlag(DockManager::AllTabsHaveCloseButton);
        bool tabHasCloseButton = (activeTabHasCloseButton && active) | allTabsHaveCloseButton;
        d->m_closeButton->setVisible(dockWidgetClosable && tabHasCloseButton);

        // Focus related stuff
        if (DockManager::testConfigFlag(DockManager::FocusHighlighting)
            && !d->m_dockWidget->dockManager()->isRestoringState()) {
            bool updateFocusStyle = false;
            if (active && !hasFocus()) {
                setFocus(Qt::OtherFocusReason);
                updateFocusStyle = true;
            }

            if (d->m_isActiveTab == active) {
                if (updateFocusStyle)
                    updateStyle();
                return;
            }
        } else if (d->m_isActiveTab == active) {
            return;
        }

        d->m_isActiveTab = active;
        updateStyle();
        update();
        updateGeometry();

        emit activeTabChanged();
    }

    DockWidget *DockWidgetTab::dockWidget() const { return d->m_dockWidget; }

    void DockWidgetTab::setDockAreaWidget(DockAreaWidget *dockArea) { d->m_dockArea = dockArea; }

    DockAreaWidget *DockWidgetTab::dockAreaWidget() const { return d->m_dockArea; }

    void DockWidgetTab::setIcon(const QIcon &icon)
    {
        QBoxLayout *boxLayout = qobject_cast<QBoxLayout *>(layout());
        if (!d->m_iconLabel && icon.isNull())
            return;

        if (!d->m_iconLabel) {
            d->m_iconLabel = new QLabel();
            d->m_iconLabel->setAlignment(Qt::AlignVCenter);
            d->m_iconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
            internal::setToolTip(d->m_iconLabel, d->m_titleLabel->toolTip());
            boxLayout->insertWidget(0, d->m_iconLabel, Qt::AlignVCenter);
            boxLayout->insertSpacing(1, qRound(1.5 * boxLayout->contentsMargins().left() / 2.0));
        } else if (icon.isNull()) {
            // Remove icon label and spacer item
            boxLayout->removeWidget(d->m_iconLabel);
            boxLayout->removeItem(boxLayout->itemAt(0));
            delete d->m_iconLabel;
            d->m_iconLabel = nullptr;
        }

        d->m_icon = icon;
        if (d->m_iconLabel) {
            d->m_iconLabel->setPixmap(
                icon.pixmap(style()->pixelMetric(QStyle::PM_SmallIconSize, nullptr, this)));
            d->m_iconLabel->setVisible(true);
        }
    }

    const QIcon &DockWidgetTab::icon() const { return d->m_icon; }

    QString DockWidgetTab::text() const { return d->m_titleLabel->text(); }

    void DockWidgetTab::mouseDoubleClickEvent(QMouseEvent *event)
    {
        // If this is the last dock area in a dock container it does not make
        // sense to move it to a new floating widget and leave this one empty
        if ((!d->m_dockArea->dockContainer()->isFloating() || d->m_dockArea->dockWidgetsCount() > 1)
            && d->m_dockWidget->features().testFlag(DockWidget::DockWidgetFloatable)) {
            d->saveDragStartMousePosition(event->globalPos());
            d->startFloating(DraggingInactive);
        }

        Super::mouseDoubleClickEvent(event);
    }

    void DockWidgetTab::setVisible(bool visible)
    {
        // Just here for debugging to insert debug output
        Super::setVisible(visible);
    }

    void DockWidgetTab::setText(const QString &title) { d->m_titleLabel->setText(title); }
    bool DockWidgetTab::isTitleElided() const { return d->m_titleLabel->isElided(); }

    bool DockWidgetTab::isClosable() const
    {
        return d->m_dockWidget
               && d->m_dockWidget->features().testFlag(DockWidget::DockWidgetClosable);
    }

    void DockWidgetTab::detachDockWidget()
    {
        if (!d->m_dockWidget->features().testFlag(DockWidget::DockWidgetFloatable))
            return;

        d->saveDragStartMousePosition(QCursor::pos());
        d->startFloating(DraggingInactive);
    }

    bool DockWidgetTab::event(QEvent *event)
    {
#ifndef QT_NO_TOOLTIP
        if (event->type() == QEvent::ToolTipChange) {
            const auto text = toolTip();
            d->m_titleLabel->setToolTip(text);
        }
#endif
        return Super::event(event);
    }

    void DockWidgetTab::onDockWidgetFeaturesChanged()
    {
        auto features = d->m_dockWidget->features();
        auto sizePolicy = d->m_closeButton->sizePolicy();
        sizePolicy.setRetainSizeWhenHidden(
            features.testFlag(DockWidget::DockWidgetClosable)
            && d->testConfigFlag(DockManager::RetainTabSizeWhenCloseButtonHidden));
        d->m_closeButton->setSizePolicy(sizePolicy);
    }

    void DockWidgetTab::setElideMode(Qt::TextElideMode mode)
    {
        d->m_titleLabel->setElideMode(mode);
    }

    void DockWidgetTab::updateStyle()
    {
        internal::repolishStyle(this, internal::RepolishDirectChildren);
    }

} // namespace ADS
