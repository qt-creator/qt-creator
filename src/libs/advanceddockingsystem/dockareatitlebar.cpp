// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#include "dockareatitlebar.h"

#include "ads_globals.h"
#include "ads_globals_p.h"
#include "advanceddockingsystemtr.h"
#include "dockareatabbar.h"
#include "dockareawidget.h"
#include "dockcomponentsfactory.h"
#include "dockmanager.h"
#include "dockoverlay.h"
#include "dockwidget.h"
#include "dockwidgettab.h"
#include "floatingdockcontainer.h"
#include "floatingdragpreview.h"

#include "autohidedockcontainer.h"
#include "dockfocuscontroller.h"
#include "elidinglabel.h"

#include <utils/qtcassert.h>

#include <QApplication>
#include <QBoxLayout>
#include <QLoggingCategory>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QStyle>

#include <iostream>

namespace ADS {

static const char *const g_locationProperty = "Location";

/**
 * Private data class of DockAreaTitleBar class (pimpl)
 */
class DockAreaTitleBarPrivate
{
public:
    DockAreaTitleBar *q;
    QPointer<TitleBarButton> m_tabsMenuButton;
    QPointer<TitleBarButton> m_autoHideButton;
    QPointer<TitleBarButton> m_undockButton;
    QPointer<TitleBarButton> m_closeButton;
    QBoxLayout *m_layout = nullptr;
    DockAreaWidget *m_dockArea = nullptr;
    DockAreaTabBar *m_tabBar = nullptr;
    ElidingLabel *m_autoHideTitleLabel;
    bool m_menuOutdated = true;
    QMenu *m_tabsMenu;
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
     * Creates the auto hide title label, only displayed when the dock area is overlayed
     */
    void createAutoHideTitleLabel();

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
     * Returns true if the given config flag is set
     * Convenience function to ease config flag testing
     */
    static bool testAutoHideConfigFlag(DockManager::eAutoHideFlag flag)
    {
        return DockManager::testAutoHideConfigFlag(flag);
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

    /**
     * Helper function to create and initialize the menu entries for
     * the "Auto Hide Group To..." menu
     */
    QAction *createAutoHideToAction(const QString &title, SideBarLocation location, QMenu *menu)
    {
        auto action = menu->addAction(title);
        action->setProperty("Location", location);
        QObject::connect(action,
                         &QAction::triggered,
                         q,
                         &DockAreaTitleBar::onAutoHideToActionClicked);
        return action;
    }
}; // class DockAreaTitleBarPrivate

DockAreaTitleBarPrivate::DockAreaTitleBarPrivate(DockAreaTitleBar *parent)
    : q(parent)
{}

void DockAreaTitleBarPrivate::createButtons()
{
    const QSize iconSize(11, 11);
    const QSize buttonSize(17, 17);
    QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    // Tabs menu button
    m_tabsMenuButton = new TitleBarButton(testConfigFlag(DockManager::DockAreaHasTabsMenuButton));
    m_tabsMenuButton->setObjectName("tabsMenuButton");
    //m_tabsMenuButton->setAutoRaise(true);
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
    internal::setToolTip(m_tabsMenuButton, Tr::tr("List All Tabs"));
    m_tabsMenuButton->setSizePolicy(sizePolicy);
    m_tabsMenuButton->setIconSize(iconSize);
    m_tabsMenuButton->setFixedSize(buttonSize);
    m_layout->addWidget(m_tabsMenuButton, 0);
    QObject::connect(m_tabsMenuButton->menu(),
                     &QMenu::triggered,
                     q,
                     &DockAreaTitleBar::onTabsMenuActionTriggered);

    // Undock button
    m_undockButton = new TitleBarButton(testConfigFlag(DockManager::DockAreaHasUndockButton));
    m_undockButton->setObjectName("detachGroupButton");
    //m_undockButton->setAutoRaise(true);
    internal::setToolTip(m_undockButton, Tr::tr("Detach Group"));
    internal::setButtonIcon(m_undockButton,
                            QStyle::SP_TitleBarNormalButton,
                            ADS::DockAreaUndockIcon);
    m_undockButton->setSizePolicy(sizePolicy);
    m_undockButton->setIconSize(iconSize);
    m_undockButton->setFixedSize(buttonSize);
    m_layout->addWidget(m_undockButton, 0);
    QObject::connect(m_undockButton,
                     &QToolButton::clicked,
                     q,
                     &DockAreaTitleBar::onUndockButtonClicked);

    // AutoHide Button
    const auto autoHideEnabled = testAutoHideConfigFlag(DockManager::AutoHideFeatureEnabled);
    m_autoHideButton = new TitleBarButton(
        testAutoHideConfigFlag(DockManager::DockAreaHasAutoHideButton) && autoHideEnabled);
    m_autoHideButton->setObjectName("dockAreaAutoHideButton");
    //m_autoHideButton->setAutoRaise(true);
    internal::setToolTip(m_autoHideButton, q->titleBarButtonToolTip(TitleBarButtonAutoHide));
    internal::setButtonIcon(m_autoHideButton, QStyle::SP_DialogOkButton, ADS::AutoHideIcon);
    m_autoHideButton->setSizePolicy(sizePolicy);
    m_autoHideButton->setCheckable(testAutoHideConfigFlag(DockManager::AutoHideButtonCheckable));
    m_autoHideButton->setChecked(false);
    m_layout->addWidget(m_autoHideButton, 0);
    QObject::connect(m_autoHideButton,
                     &QToolButton::clicked,
                     q,
                     &DockAreaTitleBar::onAutoHideButtonClicked);

    // Close button
    m_closeButton = new TitleBarButton(testConfigFlag(DockManager::DockAreaHasCloseButton));
    m_closeButton->setObjectName("dockAreaCloseButton");
    //m_closeButton->setAutoRaise(true);
    internal::setButtonIcon(m_closeButton, QStyle::SP_TitleBarCloseButton, ADS::DockAreaCloseIcon);
    if (testConfigFlag(DockManager::DockAreaCloseButtonClosesTab))
        internal::setToolTip(m_closeButton, Tr::tr("Close Active Tab"));
    else
        internal::setToolTip(m_closeButton, Tr::tr("Close Group"));

    m_closeButton->setSizePolicy(sizePolicy);
    m_closeButton->setIconSize(iconSize);
    m_closeButton->setFixedSize(buttonSize);
    m_layout->addWidget(m_closeButton, 0);
    QObject::connect(m_closeButton,
                     &QToolButton::clicked,
                     q,
                     &DockAreaTitleBar::onCloseButtonClicked);

    m_layout->addSpacing(1);
}

void DockAreaTitleBarPrivate::createAutoHideTitleLabel()
{
    m_autoHideTitleLabel = new ElidingLabel("");
    m_autoHideTitleLabel->setObjectName("autoHideTitleLabel");
    m_layout->addWidget(m_autoHideTitleLabel);
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
    QObject::connect(m_tabBar, &DockAreaTabBar::tabBarClicked, q, &DockAreaTitleBar::tabBarClicked);
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
    bool createFloatingDockContainer = (DraggingFloatingWidget != dragState);
    FloatingDockContainer *floatingDockContainer = nullptr;
    AbstractFloatingWidget *floatingWidget;
    if (createFloatingDockContainer) {
        if (m_dockArea->autoHideDockContainer())
            m_dockArea->autoHideDockContainer()->cleanupAndDelete();
        floatingWidget = floatingDockContainer = new FloatingDockContainer(m_dockArea);
    } else {
        auto w = new FloatingDragPreview(m_dockArea);
        QObject::connect(w, &FloatingDragPreview::draggingCanceled, q, [this] {
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
    if (m_dockArea->autoHideDockContainer())
        m_dockArea->autoHideDockContainer()->hide();

    m_floatingWidget = makeAreaFloating(offset, DraggingFloatingWidget);
    qApp->postEvent(m_dockArea, new QEvent((QEvent::Type) internal::g_dockedWidgetDragStartEvent));
}

TitleBarButton::TitleBarButton(bool showInTitleBar, QWidget *parent)
    : TitleBarButtonType(parent)
    , m_showInTitleBar(showInTitleBar)
    , m_hideWhenDisabled(
          DockAreaTitleBarPrivate::testConfigFlag(DockManager::DockAreaHideDisabledButtons))
{
    setFocusPolicy(Qt::NoFocus);
}

void TitleBarButton::setVisible(bool visible)
{
    // 'visible' can stay 'true' if and only if this button is configured to generally visible:
    visible = visible && m_showInTitleBar;

    // 'visible' can stay 'true' unless: this button is configured to be invisible when it
    // is disabled and it is currently disabled:
    if (visible && m_hideWhenDisabled)
        visible = isEnabled();

    Super::setVisible(visible);
}

void TitleBarButton::setShowInTitleBar(bool show)
{
    m_showInTitleBar = show;

    if (!show)
        setVisible(false);
}

bool TitleBarButton::event(QEvent *event)
{
    if (QEvent::EnabledChange == event->type() && m_hideWhenDisabled) {
        // force setVisible() call
        // Calling setVisible() directly here doesn't work well when button is expected to be shown first time
        const bool visible = isEnabled();
        QMetaObject::invokeMethod(
            this, [this, visible] { setVisible(visible); }, Qt::QueuedConnection);
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
    d->createAutoHideTitleLabel();
    d->m_autoHideTitleLabel->setVisible(false); // Default hidden
    d->m_layout->addWidget(new SpacerWidget(this));
    d->createButtons();
    setFocusPolicy(Qt::NoFocus);
}

DockAreaTitleBar::~DockAreaTitleBar()
{
    if (!d->m_closeButton.isNull())
        delete d->m_closeButton;

    if (!d->m_tabsMenuButton.isNull())
        delete d->m_tabsMenuButton;

    if (!d->m_undockButton.isNull())
        delete d->m_undockButton;

    delete d;
}

DockAreaTabBar *DockAreaTitleBar::tabBar() const
{
    return d->m_tabBar;
}

void DockAreaTitleBar::markTabsMenuOutdated()
{
    if (DockAreaTitleBarPrivate::testConfigFlag(
            DockManager::DockAreaDynamicTabsMenuButtonVisibility)) {
        bool hasElidedTabTitle = false;
        for (int i = 0; i < d->m_tabBar->count(); ++i) {
            if (!d->m_tabBar->isTabOpen(i))
                continue;

            DockWidgetTab *tab = d->m_tabBar->tab(i);
            if (tab->isTitleElided()) {
                hasElidedTabTitle = true;
                break;
            }
        }
        const bool visible = (hasElidedTabTitle && (d->m_tabBar->count() > 1));
        QMetaObject::invokeMethod(
            this,
            [this, visible] { d->m_tabsMenuButton->setVisible(visible); },
            Qt::QueuedConnection);
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
    if (DockManager::testAutoHideConfigFlag(DockManager::AutoHideCloseButtonCollapsesDock)
        && d->m_dockArea->autoHideDockContainer())
        d->m_dockArea->autoHideDockContainer()->collapseView(true);
    else if (DockAreaTitleBarPrivate::testConfigFlag(DockManager::DockAreaCloseButtonClosesTab))
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
    auto tab = d->m_tabBar->currentTab();
    if (!tab)
        return;

    QTC_ASSERT(tab, return);
    QTC_ASSERT(tab->dockWidget(), return);
    DockWidget *dockWidget = tab->dockWidget();
    if (!d->m_dockWidgetActionsButtons.isEmpty()) {
        for (auto button : std::as_const(d->m_dockWidgetActionsButtons)) {
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

    if (DockAreaTitleBarPrivate::testConfigFlag(DockManager::DockAreaCloseButtonClosesTab)) {
        DockWidget *dockWidget = d->m_tabBar->tab(index)->dockWidget();
        d->m_closeButton->setEnabled(
            dockWidget->features().testFlag(DockWidget::DockWidgetClosable));
    }

    updateDockWidgetActionsButtons();
}

void DockAreaTitleBar::onAutoHideButtonClicked()
{
    if (DockManager::testAutoHideConfigFlag(DockManager::AutoHideButtonTogglesArea)
        || qApp->keyboardModifiers().testFlag(Qt::ControlModifier))
        d->m_dockArea->toggleAutoHide();
    else
        d->m_dockArea->currentDockWidget()->toggleAutoHide();
}

void DockAreaTitleBar::onAutoHideDockAreaActionClicked()
{
    d->m_dockArea->toggleAutoHide();
}

void DockAreaTitleBar::onAutoHideToActionClicked()
{
    int location = sender()->property(g_locationProperty).toInt();
    d->m_dockArea->toggleAutoHide((SideBarLocation) location);
}

TitleBarButton *DockAreaTitleBar::button(eTitleBarButton which) const
{
    switch (which) {
    case TitleBarButtonTabsMenu:
        return d->m_tabsMenuButton;
    case TitleBarButtonUndock:
        return d->m_undockButton;
    case TitleBarButtonClose:
        return d->m_closeButton;
    case TitleBarButtonAutoHide:
        return d->m_autoHideButton;
    }
    return nullptr;
}

ElidingLabel *DockAreaTitleBar::autoHideTitleLabel() const
{
    return d->m_autoHideTitleLabel;
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
            d->dockManager()->dockFocusController()->setDockWidgetTabFocused(
                d->m_tabBar->currentTab());

        return;
    }
    Super::mousePressEvent(event);
}

void DockAreaTitleBar::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        qCInfo(adsLog) << Q_FUNC_INFO;
        event->accept();
        auto currentDragState = d->m_dragState;
        d->m_dragStartMousePos = QPoint();
        d->m_dragState = DraggingInactive;
        if (DraggingFloatingWidget == currentDragState)
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
        && d->m_dockArea->dockContainer()->visibleDockAreaCount() == 1
        && !d->m_dockArea->isAutoHide()) {
        return;
    }

    // If one single dock widget in this area is not floatable then the whole
    // area is not floatable
    // We can create the floating drag preview if the dock widget is movable
    auto features = d->m_dockArea->features();
    if (!features.testFlag(DockWidget::DockWidgetFloatable)
        && !(features.testFlag(DockWidget::DockWidgetMovable))) {
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

    const bool isAutoHide = d->m_dockArea->isAutoHide();
    const bool isTopLevelArea = d->m_dockArea->isTopLevelArea();

    QMenu menu(this);
    if (!isTopLevelArea) {
        QAction *detachAction = menu.addAction(isAutoHide ? Tr::tr("Detach")
                                                          : Tr::tr("Detach Group"));
        detachAction->connect(detachAction,
                              &QAction::triggered,
                              this,
                              &DockAreaTitleBar::onUndockButtonClicked);
        detachAction->setEnabled(
            d->m_dockArea->features().testFlag(DockWidget::DockWidgetFloatable));

        if (DockManager::testAutoHideConfigFlag(DockManager::AutoHideFeatureEnabled)) {
            QAction *pinAction = menu.addAction(isAutoHide ? Tr::tr("Unpin (Dock)")
                                                           : Tr::tr("Pin Group"));
            pinAction->connect(pinAction,
                               &QAction::triggered,
                               this,
                               &DockAreaTitleBar::onAutoHideDockAreaActionClicked);

            auto areaIsPinnable = d->m_dockArea->features().testFlag(DockWidget::DockWidgetPinnable);
            pinAction->setEnabled(areaIsPinnable);
            if (!isAutoHide) {
                auto tmp = menu.addMenu(Tr::tr("Pin Group To..."));
                tmp->setEnabled(areaIsPinnable);
                d->createAutoHideToAction(Tr::tr("Top"), SideBarTop, tmp);
                d->createAutoHideToAction(Tr::tr("Left"), SideBarLeft, tmp);
                d->createAutoHideToAction(Tr::tr("Right"), SideBarRight, tmp);
                d->createAutoHideToAction(Tr::tr("Bottom"), SideBarBottom, tmp);
            }
        }
        menu.addSeparator();
    }
    QAction *closeAction = menu.addAction(isAutoHide ? Tr::tr("Close") : Tr::tr("Close Group"));
    closeAction->connect(closeAction,
                         &QAction::triggered,
                         this,
                         &DockAreaTitleBar::onCloseButtonClicked);
    closeAction->setEnabled(d->m_dockArea->features().testFlag(DockWidget::DockWidgetClosable));

    if (!isAutoHide && !isTopLevelArea) {
        QAction *closeOthersAction = menu.addAction(Tr::tr("Close Other Groups"));
        closeOthersAction->connect(closeOthersAction,
                                   &QAction::triggered,
                                   d->m_dockArea,
                                   &DockAreaWidget::closeOtherAreas);
    }

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

QString DockAreaTitleBar::titleBarButtonToolTip(eTitleBarButton button) const
{
    switch (button) {
    case TitleBarButtonAutoHide:
        if (d->m_dockArea->isAutoHide())
            return Tr::tr("Unpin (Dock)");

        if (DockManager::testAutoHideConfigFlag(DockManager::AutoHideButtonTogglesArea))
            return Tr::tr("Pin Group");
        else
            return Tr::tr("Pin Active Tab (Press Ctrl to Pin Group)");
        break;

    case TitleBarButtonClose:
        if (d->m_dockArea->isAutoHide())
            return Tr::tr("Close");

        if (DockManager::testConfigFlag(DockManager::DockAreaCloseButtonClosesTab))
            return Tr::tr("Close Active Tab");
        else
            return Tr::tr("Close Group");
        break;

    default:
        break;
    }
    return QString();
}

void DockAreaTitleBar::setAreaFloating()
{
    // If this is the last dock area in a dock container it does not make  sense to move it to
    // a new floating widget and leave this one empty.
    auto dockContainer = d->m_dockArea->dockContainer();
    if (dockContainer->isFloating() && dockContainer->dockAreaCount() == 1
        && !d->m_dockArea->isAutoHide())
        return;

    if (!d->m_dockArea->features().testFlag(DockWidget::DockWidgetFloatable))
        return;

    d->makeAreaFloating(mapFromGlobal(QCursor::pos()), DraggingInactive);
}

} // namespace ADS
