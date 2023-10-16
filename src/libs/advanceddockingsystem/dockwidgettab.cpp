// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#include "dockwidgettab.h"

#include "ads_globals.h"
#include "ads_globals_p.h"
#include "advanceddockingsystemtr.h"
#include "dockareawidget.h"
#include "dockfocuscontroller.h"
#include "dockmanager.h"
#include "dockoverlay.h"
#include "dockwidget.h"
#include "elidinglabel.h"
#include "floatingdockcontainer.h"
#include "floatingdragpreview.h"

#include <utils/theme/theme.h>

#include <QApplication>
#include <QBoxLayout>
#include <QLabel>
#include <QLoggingCategory>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSplitter>
#include <QStyle>
#include <QStyleOption>
#include <QStylePainter>
#include <QToolButton>

#include <iostream>

namespace ADS {

static const char *const g_locationProperty = "Location";
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
    TabButton *m_closeButton = nullptr;
    QPoint m_tabDragStartPosition;
    QSize m_iconSize;

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
    TabButton *createCloseButton() const { return new TabButton(); }

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
     * Update the close button visibility from current feature/config
     */
    void updateCloseButtonVisibility(bool active)
    {
        bool dockWidgetClosable = m_dockWidget->features().testFlag(DockWidget::DockWidgetClosable);
        bool activeTabHasCloseButton = testConfigFlag(DockManager::ActiveTabHasCloseButton);
        bool allTabsHaveCloseButton = testConfigFlag(DockManager::AllTabsHaveCloseButton);
        bool tabHasCloseButton = (activeTabHasCloseButton && active) | allTabsHaveCloseButton;

        m_closeButton->setVisible(dockWidgetClosable && tabHasCloseButton);
    }

    /**
     * Update the size policy of the close button depending on the
     * RetainTabSizeWhenCloseButtonHidden feature
     */
    void updateCloseButtonSizePolicy()
    {
        auto features = m_dockWidget->features();
        auto sizePolicy = m_closeButton->sizePolicy();
        sizePolicy.setRetainSizeWhenHidden(
            features.testFlag(DockWidget::DockWidgetClosable)
            && testConfigFlag(DockManager::RetainTabSizeWhenCloseButtonHidden));
        m_closeButton->setSizePolicy(sizePolicy);
    }

    /**
     * Saves the drag start position in global and local coordinates
     */
    void saveDragStartMousePosition(const QPoint &globalPos)
    {
        m_globalDragStartMousePosition = globalPos;
        m_dragStartMousePosition = q->mapFromGlobal(globalPos);
    }

    /**
     * Update the icon in case the icon size changed
     */
    void updateIcon()
    {
        if (!m_iconLabel || m_icon.isNull())
            return;

        if (m_iconSize.isValid())
            m_iconLabel->setPixmap(m_icon.pixmap(m_iconSize));
        else
            m_iconLabel->setPixmap(
                m_icon.pixmap(q->style()->pixelMetric(QStyle::PM_SmallIconSize, nullptr, q)));

        m_iconLabel->setVisible(true);
    }

    /**
     * Convenience function for access to the dock manager dock focus controller
     */
    DockFocusController *focusController() const
    {
        return m_dockWidget->dockManager()->dockFocusController();
    }

    /**
     * Helper function to create and initialize the menu entries for
     * the "Auto Hide Group To..." menu
     */
    QAction *createAutoHideToAction(const QString &title, SideBarLocation location, QMenu *menu)
    {
        auto action = menu->addAction(title);
        action->setProperty("Location", location);
        QObject::connect(action, &QAction::triggered, q, &DockWidgetTab::onAutoHideToActionClicked);
        return action;
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
    QObject::connect(m_titleLabel, &ElidingLabel::elidedChanged, q, &DockWidgetTab::elidedChanged);

    m_closeButton = createCloseButton();
    m_closeButton->setObjectName("tabCloseButton");
    internal::setButtonIcon(m_closeButton, QStyle::SP_TitleBarCloseButton, TabCloseIcon);
    m_closeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_closeButton->setIconSize(QSize(11, 11));
    m_closeButton->setFixedSize(QSize(17, 17));
    m_closeButton->setFocusPolicy(Qt::NoFocus);
    updateCloseButtonSizePolicy();
    internal::setToolTip(m_closeButton, Tr::tr("Close Tab"));
    QObject::connect(m_closeButton, &QAbstractButton::clicked, q, &DockWidgetTab::closeRequested);

    QFontMetrics fontMetrics(m_titleLabel->font());
    int spacing = qRound(fontMetrics.height() / 4.0);

    // Fill the layout
    QBoxLayout *boxLayout = new QBoxLayout(QBoxLayout::LeftToRight);
    boxLayout->setContentsMargins(2 * spacing, 0, 0, 0);
    boxLayout->setSpacing(0);
    q->setLayout(boxLayout);
    boxLayout->addWidget(m_titleLabel, 1, Qt::AlignVCenter);
    boxLayout->addSpacing(qRound(spacing * 4.0 / 3.0));
    boxLayout->addWidget(m_closeButton, 0, Qt::AlignVCenter);
    boxLayout->addSpacing(1);
    boxLayout->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);

    if (DockManager::testConfigFlag(DockManager::FocusHighlighting))
        m_closeButton->setCheckable(true);

    m_titleLabel->setVisible(true);
}

void DockWidgetTabPrivate::moveTab(QMouseEvent *event)
{
    event->accept();
    QPoint distance = event->globalPosition().toPoint() - m_globalDragStartMousePosition;
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
    // If this is the last dock widget inside of this floating widget, then it does not make any
    // sense, to make it floating because it is already floating.
    if (dockContainer->isFloating() && (dockContainer->visibleDockAreaCount() == 1)
        && (m_dockWidget->dockAreaWidget()->dockWidgetsCount() == 1))
        return false;

    m_dragState = draggingState;
    AbstractFloatingWidget *floatingWidget = nullptr;
    bool createContainer = (DraggingFloatingWidget != draggingState);

    // If section widget has multiple tabs, we take only one tab. If it has only one single tab,
    // we can move the complete dock area into floating widget.
    QSize size;
    if (m_dockArea->dockWidgetsCount() > 1) {
        floatingWidget = createFloatingWidget(m_dockWidget, createContainer);
        size = m_dockWidget->size();
    } else {
        floatingWidget = createFloatingWidget(m_dockArea, createContainer);
        size = m_dockArea->size();
    }

    if (DraggingFloatingWidget == draggingState) {
        floatingWidget->startFloating(m_dragStartMousePosition, size, DraggingFloatingWidget, q);
        auto overlay = m_dockWidget->dockManager()->containerOverlay();
        overlay->setAllowedAreas(OuterDockAreas);
        m_floatingWidget = floatingWidget;
        qApp->postEvent(m_dockWidget,
                        new QEvent((QEvent::Type) internal::g_dockedWidgetDragStartEvent));
    } else {
        floatingWidget->startFloating(m_dragStartMousePosition, size, DraggingInactive, nullptr);
    }

    return true;
}

TabButton::TabButton(QWidget *parent)
    : TabButtonType(parent)
    , m_active(false)
    , m_focus(false)
{}

void TabButton::setActive(bool value)
{
    m_active = value;
}
void TabButton::setFocus(bool value)
{
    m_focus = value;
}

void TabButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QStylePainter p(this);
    QStyleOptionToolButton opt;
    initStyleOption(&opt);
    opt.icon = QIcon(); // set to null icon otherwise it is drawn twice
    p.drawComplexControl(QStyle::CC_ToolButton, opt);

    QIcon::Mode mode = QIcon::Mode::Normal;
    if (m_active)
        mode = QIcon::Mode::Active;
    if (m_focus)
        mode = QIcon::Mode::Selected;

    const QPoint iconPosition = rect().center()
                                - QPoint(iconSize().width() * 0.5, iconSize().height() * 0.5);

    p.drawPixmap(iconPosition, icon().pixmap(iconSize(), mode));
}

DockWidgetTab::DockWidgetTab(DockWidget *dockWidget, QWidget *parent)
    : QFrame(parent)
    , d(new DockWidgetTabPrivate(this))
{
    setAttribute(Qt::WA_NoMousePropagation, true);
    d->m_dockWidget = dockWidget;
    d->createLayout();
    setFocusPolicy(Qt::NoFocus);
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
        d->saveDragStartMousePosition(event->globalPosition().toPoint());
        d->m_dragState = DraggingMousePressed;
        if (DockManager::testConfigFlag(DockManager::FocusHighlighting)) {
            d->focusController()->setDockWidgetTabPressed(true);
            d->focusController()->setDockWidgetTabFocused(this);
        }
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
                event->accept();
                emit moved(event->globalPosition().toPoint());
            }
            break;

        case DraggingFloatingWidget:
            event->accept();
            d->m_floatingWidget->finishDragging();
            break;

        default:
            if (DockManager::testConfigFlag(DockManager::FocusHighlighting))
                d->focusController()->setDockWidgetTabPressed(false);
            break;
        }
    } else if (event->button() == Qt::MiddleButton) {
        if (DockManager::testConfigFlag(DockManager::MiddleMouseButtonClosesTab)
            && d->m_dockWidget->features().testFlag(DockWidget::DockWidgetClosable)) {
            // Only attempt to close if the mouse is still
            // on top of the widget, to allow the user to cancel.
            if (rect().contains(mapFromGlobal(QCursor::pos()))) {
                event->accept();
                emit closeRequested();
            }
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

    // Move floating window
    if (d->isDraggingState(DraggingFloatingWidget)) {
        d->m_floatingWidget->moveFloating();
        Super::mouseMoveEvent(event);
        return;
    }

    // Move tab
    if (d->isDraggingState(DraggingTab)) {
        // Moving the tab is always allowed because it does not mean moving the dock widget around
        d->moveTab(event);
    }

    auto mappedPos = mapToParent(event->pos());
    bool mouseOutsideBar = (mappedPos.x() < 0) || (mappedPos.x() > parentWidget()->rect().right());
    // Maybe a fixed drag distance is better here ?
    int dragDistanceY = qAbs(d->m_globalDragStartMousePosition.y()
                             - event->globalPosition().toPoint().y());
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
        // We can create the drag preview if the widget is movable.
        auto features = d->m_dockWidget->features();
        if (features.testFlag(DockWidget::DockWidgetFloatable)
            || (features.testFlag(DockWidget::DockWidgetMovable))) {
            // If we undock, we need to restore the initial position of this
            // tab because it looks strange if it remains on its dragged position
            if (d->isDraggingState(DraggingTab))
                parentWidget()->layout()->update();

            d->startFloating();
        }
        return;
    } else if (d->m_dockArea->openDockWidgetsCount() > 1
               && (event->globalPosition().toPoint() - d->m_globalDragStartMousePosition)
                          .manhattanLength()
                      >= QApplication::startDragDistance()) // Wait a few pixels before start moving
    {
        // If we start dragging the tab, we save its initial position to restore it later
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

    const bool isFloatable = d->m_dockWidget->features().testFlag(DockWidget::DockWidgetFloatable);
    const bool isNotOnlyTabInContainer = !d->m_dockArea->dockContainer()->hasTopLevelDockWidget();
    const bool isTopLevelArea = d->m_dockArea->isTopLevelArea();
    const bool isDetachable = isFloatable && isNotOnlyTabInContainer;

    QMenu menu(this);

    if (!isTopLevelArea) {
        QAction *detachAction = menu.addAction(Tr::tr("Detach"));
        detachAction->connect(detachAction,
                              &QAction::triggered,
                              this,
                              &DockWidgetTab::detachDockWidget);
        detachAction->setEnabled(isDetachable);

        if (DockManager::testAutoHideConfigFlag(DockManager::AutoHideFeatureEnabled)) {
            QAction *pinAction = menu.addAction(Tr::tr("Pin"));
            pinAction->connect(pinAction,
                               &QAction::triggered,
                               this,
                               &DockWidgetTab::autoHideDockWidget);

            auto isPinnable = d->m_dockWidget->features().testFlag(DockWidget::DockWidgetPinnable);
            pinAction->setEnabled(isPinnable);

            auto subMenu = menu.addMenu(Tr::tr("Pin To..."));
            subMenu->setEnabled(isPinnable);
            d->createAutoHideToAction(Tr::tr("Top"), SideBarTop, subMenu);
            d->createAutoHideToAction(Tr::tr("Left"), SideBarLeft, subMenu);
            d->createAutoHideToAction(Tr::tr("Right"), SideBarRight, subMenu);
            d->createAutoHideToAction(Tr::tr("Bottom"), SideBarBottom, subMenu);
        }
    }

    menu.addSeparator();

    QAction *closeAction = menu.addAction(Tr::tr("Close"));
    closeAction->connect(closeAction, &QAction::triggered, this, &DockWidgetTab::closeRequested);
    closeAction->setEnabled(isClosable());

    if (d->m_dockArea->openDockWidgetsCount() > 1) {
        QAction *closeOthersAction = menu.addAction(Tr::tr("Close Others"));
        closeOthersAction->connect(closeOthersAction,
                                   &QAction::triggered,
                                   this,
                                   &DockWidgetTab::closeOtherTabsRequested);
    }
    menu.exec(event->globalPos());
}

bool DockWidgetTab::isActiveTab() const
{
    return d->m_isActiveTab;
}

void DockWidgetTab::setActiveTab(bool active)
{
    d->updateCloseButtonVisibility(active);

    d->m_closeButton->setActive(active); // TODO

    // Focus related stuff
    if (DockManager::testConfigFlag(DockManager::FocusHighlighting)
        && !d->m_dockWidget->dockManager()->isRestoringState()) {
        bool updateFocusStyle = false;
        if (active && !hasFocus()) {
            d->focusController()->setDockWidgetTabFocused(this);
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

DockWidget *DockWidgetTab::dockWidget() const
{
    return d->m_dockWidget;
}

void DockWidgetTab::setDockAreaWidget(DockAreaWidget *dockArea)
{
    d->m_dockArea = dockArea;
}

DockAreaWidget *DockWidgetTab::dockAreaWidget() const
{
    return d->m_dockArea;
}

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

const QIcon &DockWidgetTab::icon() const
{
    return d->m_icon;
}

QString DockWidgetTab::text() const
{
    return d->m_titleLabel->text();
}

void DockWidgetTab::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // If this is the last dock area in a dock container it does not make
        // sense to move it to a new floating widget and leave this one empty
        if ((!d->m_dockArea->dockContainer()->isFloating() || d->m_dockArea->dockWidgetsCount() > 1)
            && d->m_dockWidget->features().testFlag(DockWidget::DockWidgetFloatable)) {
            event->accept();
            d->saveDragStartMousePosition(event->globalPosition().toPoint());
            d->startFloating(DraggingInactive);
        }
    }

    Super::mouseDoubleClickEvent(event);
}

void DockWidgetTab::setVisible(bool visible)
{
    visible &= !d->m_dockWidget->features().testFlag(DockWidget::NoTab);
    Super::setVisible(visible);
}

void DockWidgetTab::setText(const QString &title)
{
    d->m_titleLabel->setText(title);
}
bool DockWidgetTab::isTitleElided() const
{
    return d->m_titleLabel->isElided();
}

bool DockWidgetTab::isClosable() const
{
    return d->m_dockWidget && d->m_dockWidget->features().testFlag(DockWidget::DockWidgetClosable);
}

void DockWidgetTab::detachDockWidget()
{
    if (!d->m_dockWidget->features().testFlag(DockWidget::DockWidgetFloatable))
        return;

    d->saveDragStartMousePosition(QCursor::pos());
    d->startFloating(DraggingInactive);
}

void DockWidgetTab::autoHideDockWidget()
{
    d->m_dockWidget->setAutoHide(true);
}

void DockWidgetTab::onAutoHideToActionClicked()
{
    int location = sender()->property(g_locationProperty).toInt();
    d->m_dockWidget->toggleAutoHide((SideBarLocation) location);
}

bool DockWidgetTab::event(QEvent *event)
{
#ifndef QT_NO_TOOLTIP
    if (event->type() == QEvent::ToolTipChange) {
        const auto text = toolTip();
        d->m_titleLabel->setToolTip(text);
        if (d->m_iconLabel)
            d->m_iconLabel->setToolTip(text);
    }
#endif

    if (event->type() == QEvent::StyleChange)
        d->updateIcon();

    return Super::event(event);
}

void DockWidgetTab::onDockWidgetFeaturesChanged()
{
    d->updateCloseButtonSizePolicy();
    d->updateCloseButtonVisibility(isActiveTab());
}

void DockWidgetTab::setElideMode(Qt::TextElideMode mode)
{
    d->m_titleLabel->setElideMode(mode);
}

void DockWidgetTab::updateStyle()
{
    if (DockManager::testConfigFlag(DockManager::FocusHighlighting))
        d->m_closeButton->setFocus(property("focused").toBool());

    internal::repolishStyle(this, internal::RepolishDirectChildren);
}

QSize DockWidgetTab::iconSize() const
{
    return d->m_iconSize;
}

void DockWidgetTab::setIconSize(const QSize &size)
{
    if (size == d->m_iconSize)
        return;

    d->m_iconSize = size;
    d->updateIcon();
    emit iconSizeChanged();
}

} // namespace ADS
