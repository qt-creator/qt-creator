// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#include "dockareawidget.h"

#include "ads_globals_p.h"
#include "autohidedockcontainer.h"
#include "dockareatabbar.h"
#include "dockareatitlebar.h"
#include "dockcomponentsfactory.h"
#include "dockcontainerwidget.h"
#include "dockingstatereader.h"
#include "dockmanager.h"
#include "docksplitter.h"
#include "dockwidget.h"
#include "dockwidgettab.h"
#include "elidinglabel.h"
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

namespace ADS {

static const char *const g_indexProperty = "index";
static const char *const g_actionProperty = "action";

/**
 * Check, if auto hide is enabled
 */
static bool isAutoHideFeatureEnabled()
{
    return DockManager::testAutoHideConfigFlag(DockManager::AutoHideFeatureEnabled);
}

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

            delete layoutItem;
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

        auto layoutItem = m_parentLayout->takeAt(1);

        if (layoutItem)
            layoutItem->widget()->setParent(nullptr);

        delete layoutItem;

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

static const DockWidgetAreas DefaultAllowedAreas = AllDockAreas;

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
    AutoHideDockContainer *m_autoHideDockContainer = nullptr;
    bool m_updateTitleBarButtons = false;
    DockWidgetAreas m_allowedAreas = AllDockAreas;
    QSize m_minSizeHint;
    DockAreaWidget::DockAreaFlags m_flags{DockAreaWidget::DefaultFlags};

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
        return qvariant_cast<QAction *>(dockWidget->property(g_actionProperty));
    }

    /**
     * Returns the index of the given dock widget
     */
    int dockWidgetIndex(DockWidget *dockWidget) const
    {
        return dockWidget->property(g_indexProperty).toInt();
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
     * Updates the visibility of the close and detach button
     */
    void updateTitleBarButtonVisibility(bool isTopLevel);

    /**
     * Scans all contained dock widgets for the max. minimum size hint
     */
    void updateMinimumSizeHint()
    {
        m_minSizeHint = QSize();
        for (int i = 0; i < m_contentsLayout->count(); ++i) {
            auto widget = m_contentsLayout->widget(i);
            m_minSizeHint.setHeight(
                qMax(m_minSizeHint.height(), widget->minimumSizeHint().height()));
            m_minSizeHint.setWidth(qMax(m_minSizeHint.width(), widget->minimumSizeHint().width()));
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
    m_titleBar->button(TitleBarButtonAutoHide)
        ->setEnabled(q->features().testFlag(DockWidget::DockWidgetPinnable));
    m_titleBar->updateDockWidgetActionsButtons();
    m_updateTitleBarButtons = false;
}

void DockAreaWidgetPrivate::updateTitleBarButtonVisibility(bool isTopLevel)
{
    auto *const container = q->dockContainer();
    if (!container)
        return;

    if (isTopLevel) {
        m_titleBar->button(TitleBarButtonClose)->setVisible(!container->isFloating());
        m_titleBar->button(TitleBarButtonAutoHide)->setVisible(!container->isFloating());
        // Undock and tabs should never show when auto hidden
        m_titleBar->button(TitleBarButtonUndock)
            ->setVisible(!container->isFloating() && !q->isAutoHide());
        m_titleBar->button(TitleBarButtonTabsMenu)->setVisible(!q->isAutoHide());
    } else {
        m_titleBar->button(TitleBarButtonClose)->setVisible(true);
        m_titleBar->button(TitleBarButtonAutoHide)->setVisible(true);
        m_titleBar->button(TitleBarButtonUndock)->setVisible(!q->isAutoHide());
        m_titleBar->button(TitleBarButtonTabsMenu)->setVisible(!q->isAutoHide());
    }
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

DockManager *DockAreaWidget::dockManager() const
{
    return d->m_dockManager;
}

DockContainerWidget *DockAreaWidget::dockContainer() const
{
    return internal::findParent<DockContainerWidget *>(this);
}

AutoHideDockContainer *DockAreaWidget::autoHideDockContainer() const
{
    return d->m_autoHideDockContainer;
}

bool DockAreaWidget::isAutoHide() const
{
    return d->m_autoHideDockContainer != nullptr;
}

void DockAreaWidget::setAutoHideDockContainer(AutoHideDockContainer *autoHideDockContainer)
{
    d->m_autoHideDockContainer = autoHideDockContainer;
    updateAutoHideButtonCheckState();
    updateTitleBarButtonsToolTips();
    d->m_titleBar->button(TitleBarButtonAutoHide)->setShowInTitleBar(true);
}

void DockAreaWidget::addDockWidget(DockWidget *dockWidget)
{
    insertDockWidget(d->m_contentsLayout->count(), dockWidget);
}

void DockAreaWidget::insertDockWidget(int index, DockWidget *dockWidget, bool activate)
{
    if (index < 0 || index > d->m_contentsLayout->count())
        index = d->m_contentsLayout->count();

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
    d->m_titleBar->autoHideTitleLabel()->setText(dockWidget->windowTitle());
    dockWidget->setProperty(g_indexProperty, index);
    d->m_minSizeHint.setHeight(
        qMax(d->m_minSizeHint.height(), dockWidget->minimumSizeHint().height()));
    d->m_minSizeHint.setWidth(qMax(d->m_minSizeHint.width(), dockWidget->minimumSizeHint().width()));
    if (activate) {
        setCurrentIndex(index);
        dockWidget->setClosedState(
            false); // Set current index can show the widget without changing the close state, added to keep the close state consistent
    }

    // If this dock area is hidden, then we need to make it visible again
    // by calling dockWidget->toggleViewInternal(true);
    if (!this->isVisible() && d->m_contentsLayout->count() > 1 && !dockManager()->isRestoringState())
        dockWidget->toggleViewInternal(true);

    d->updateTitleBarButtonStates();
    updateTitleBarVisibility();
}

void DockAreaWidget::removeDockWidget(DockWidget *dockWidget)
{
    qCInfo(adsLog) << Q_FUNC_INFO;
    if (!dockWidget)
        return;

    // If this dock area is in a auto hide container, then we can delete
    // the auto hide container now
    if (isAutoHide()) {
        autoHideDockContainer()->cleanupAndDelete();
        return;
    }

    auto current = currentDockWidget();
    auto nextOpen = (dockWidget == current) ? nextOpenDockWidget(dockWidget) : nullptr;

    d->m_contentsLayout->removeWidget(dockWidget);
    auto tabWidget = dockWidget->tabWidget();
    tabWidget->hide();
    d->tabBar()->removeTab(tabWidget);
    tabWidget->setParent(dockWidget);
    dockWidget->setDockArea(nullptr);
    DockContainerWidget *dockContainerWidget = dockContainer();
    if (nextOpen) {
        setCurrentDockWidget(nextOpen);
    } else if (d->m_contentsLayout->isEmpty() && dockContainerWidget->dockAreaCount() >= 1) {
        qCInfo(adsLog) << "Dock Area empty";
        dockContainerWidget->removeDockArea(this);
        this->deleteLater();
        if (dockContainerWidget->dockAreaCount() == 0) {
            if (FloatingDockContainer *floatingDockContainer
                = dockContainerWidget->floatingWidget()) {
                floatingDockContainer->hide();
                floatingDockContainer->deleteLater();
            }
        }
    } else if (dockWidget == current) {
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

    if (isAutoHide())
        autoHideDockContainer()->hide();
}

void DockAreaWidget::onTabCloseRequested(int index)
{
    qCInfo(adsLog) << Q_FUNC_INFO << "index" << index;
    dockWidget(index)->requestCloseDockWidget();
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
    dockWidget->setClosedState(
        false); // Set current index can show the widget without changing the close state, added to keep the close state consistent
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

int DockAreaWidget::currentIndex() const
{
    return d->m_contentsLayout->currentIndex();
}

QRect DockAreaWidget::titleBarGeometry() const
{
    return d->m_titleBar->geometry();
}

QRect DockAreaWidget::contentAreaGeometry() const
{
    return d->m_contentsLayout->geometry();
}

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

    return -1;
}

int DockAreaWidget::dockWidgetsCount() const
{
    return d->m_contentsLayout->count();
}

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

    if (!d->m_titleBar)
        return;

    bool autoHide = isAutoHide();

    if (!DockManager::testConfigFlag(DockManager::AlwaysShowTabs)) {
        bool hidden = container->hasTopLevelDockWidget()
                      && (container->isFloating()
                          || DockManager::testConfigFlag(
                              DockManager::HideSingleCentralWidgetTitleBar));
        hidden |= (d->m_flags.testFlag(HideSingleWidgetTitleBar) && openDockWidgetsCount() == 1);
        hidden &= !autoHide; // Titlebar must always be visible when auto hidden so it can be dragged
        d->m_titleBar->setVisible(!hidden);
    }

    if (isAutoHideFeatureEnabled()) {
        auto tabBar = d->m_titleBar->tabBar();
        tabBar->setVisible(!autoHide); // Never show tab bar when auto hidden
        d->m_titleBar->autoHideTitleLabel()->setVisible(
            autoHide);                 // Always show when auto hidden, never otherwise
        updateTitleBarButtonVisibility(container->topLevelDockArea() == this);
    }
}

void DockAreaWidget::markTitleBarMenuOutdated()
{
    if (d->m_titleBar)
        d->m_titleBar->markTabsMenuOutdated();
}

void DockAreaWidget::updateAutoHideButtonCheckState()
{
    auto autoHideButton = titleBarButton(TitleBarButtonAutoHide);
    autoHideButton->blockSignals(true);
    autoHideButton->setChecked(isAutoHide());
    autoHideButton->blockSignals(false);
}

void DockAreaWidget::updateTitleBarButtonVisibility(bool isTopLevel) const
{
    d->updateTitleBarButtonVisibility(isTopLevel);
}

void DockAreaWidget::updateTitleBarButtonsToolTips()
{
    internal::setToolTip(titleBarButton(TitleBarButtonClose),
                         titleBar()->titleBarButtonToolTip(TitleBarButtonClose));
    internal::setToolTip(titleBarButton(TitleBarButtonAutoHide),
                         titleBar()->titleBarButtonToolTip(TitleBarButtonAutoHide));
}

void DockAreaWidget::saveState(QXmlStreamWriter &stream) const
{
    stream.writeStartElement("area");
    stream.writeAttribute("tabs", QString::number(d->m_contentsLayout->count()));
    auto localDockWidget = currentDockWidget();
    QString name = localDockWidget ? localDockWidget->objectName() : "";
    stream.writeAttribute("current", name);

    if (d->m_allowedAreas != DefaultAllowedAreas)
        stream.writeAttribute("allowedAreas", QString::number(d->m_allowedAreas, 16));

    if (d->m_flags != DefaultFlags)
        stream.writeAttribute("flags", QString::number(d->m_flags, 16));

    qCInfo(adsLog) << Q_FUNC_INFO << "TabCount: " << d->m_contentsLayout->count()
                   << " Current: " << name;
    for (int i = 0; i < d->m_contentsLayout->count(); ++i)
        dockWidget(i)->saveState(stream);

    stream.writeEndElement();
}

bool DockAreaWidget::restoreState(DockingStateReader &stateReader,
                                  DockAreaWidget *&createdWidget,
                                  bool testing,
                                  DockContainerWidget *container)
{
    QString currentDockWidget = stateReader.attributes().value("current").toString();

#ifdef ADS_DEBUG_PRINT
    bool ok;
    int tabs = stateReader.attributes().value("tabs").toInt(&ok);
    if (!ok)
        return false;

    qCInfo(adsLog) << "Restore NodeDockArea Tabs: " << tabs << " Current: " << currentDockWidget;
#endif

    auto dockManager = container->dockManager();
    DockAreaWidget *dockArea = nullptr;
    if (!testing) {
        dockArea = new DockAreaWidget(dockManager, container);
        const auto allowedAreasAttribute = stateReader.attributes().value("allowedAreas");
        if (!allowedAreasAttribute.isEmpty())
            dockArea->setAllowedAreas((DockWidgetArea) allowedAreasAttribute.toInt(nullptr, 16));

        const auto flagsAttribute = stateReader.attributes().value("flags");
        if (!flagsAttribute.isEmpty())
            dockArea->setDockAreaFlags(
                (DockAreaWidget::DockAreaFlags) flagsAttribute.toInt(nullptr, 16));
    }

    while (stateReader.readNextStartElement()) {
        if (stateReader.name() != QLatin1String("widget"))
            continue;

        auto objectName = stateReader.attributes().value("name");

        if (objectName.isEmpty()) {
            qCInfo(adsLog) << "Error: Empty name!";
            return false;
        }

        QVariant closedVar = QVariant(stateReader.attributes().value("closed").toString());
        if (!closedVar.canConvert<bool>())
            return false;

        bool closed = closedVar.value<bool>();

        stateReader.skipCurrentElement();
        DockWidget *dockWidget = dockManager->findDockWidget(objectName.toString());

        if (!dockWidget || testing)
            continue;

        qCInfo(adsLog) << "Dock Widget found" << dockWidget->objectName() << "parent"
                       << dockWidget->parent();

        if (dockWidget->autoHideDockContainer())
            dockWidget->autoHideDockContainer()->cleanupAndDelete();

        // We hide the DockArea here to prevent the short display (the flashing)
        // of the dock areas during application startup
        dockArea->hide();
        dockArea->addDockWidget(dockWidget);
        dockWidget->setToggleViewActionChecked(!closed);
        dockWidget->setClosedState(closed);
        dockWidget->setProperty(internal::g_closedProperty, closed);
        dockWidget->setProperty(internal::g_dirtyProperty, false);
    }

    if (testing)
        return true;

    if (!dockArea->dockWidgetsCount()) {
        delete dockArea;
        dockArea = nullptr;
    } else {
        dockArea->setProperty("currentDockWidget", currentDockWidget);
    }

    createdWidget = dockArea;
    return true;
}

DockWidget *DockAreaWidget::nextOpenDockWidget(DockWidget *dockWidget) const
{
    auto openDockWidgets = openedDockWidgets();
    if (openDockWidgets.count() > 1
        || (openDockWidgets.count() == 1 && openDockWidgets[0] != dockWidget)) {
        if (openDockWidgets.last() == dockWidget) {
            DockWidget *nextDockWidget = openDockWidgets[openDockWidgets.count() - 2];
            // search backwards for widget with tab
            for (int i = openDockWidgets.count() - 2; i >= 0; --i) {
                auto dw = openDockWidgets[i];
                if (!dw->features().testFlag(DockWidget::NoTab))
                    return dw;
            }

            // return widget without tab
            return nextDockWidget;
        } else {
            int indexOfDockWidget = openDockWidgets.indexOf(dockWidget);
            DockWidget *nextDockWidget = openDockWidgets[indexOfDockWidget + 1];
            // search forwards for widget with tab
            for (int i = indexOfDockWidget + 1; i < openDockWidgets.count(); ++i) {
                auto dw = openDockWidgets[i];
                if (!dw->features().testFlag(DockWidget::NoTab))
                    return dw;
            }

            // search backwards for widget with tab
            for (int i = indexOfDockWidget - 1; i >= 0; --i) {
                auto dw = openDockWidgets[i];
                if (!dw->features().testFlag(DockWidget::NoTab))
                    return dw;
            }

            // return widget without tab
            return nextDockWidget;
        }
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

DockAreaWidget::DockAreaFlags DockAreaWidget::dockAreaFlags() const
{
    return d->m_flags;
}

void DockAreaWidget::setDockAreaFlags(DockAreaFlags flags)
{
    auto changedFlags = d->m_flags ^ flags;
    d->m_flags = flags;
    if (changedFlags.testFlag(HideSingleWidgetTitleBar))
        updateTitleBarVisibility();
}

void DockAreaWidget::setDockAreaFlag(eDockAreaFlag flag, bool on)
{
    auto flags = dockAreaFlags();
    internal::setFlag(flags, flag, on);
    setDockAreaFlags(flags);
}

QAbstractButton *DockAreaWidget::titleBarButton(eTitleBarButton which) const
{
    return d->m_titleBar->button(which);
}

void DockAreaWidget::closeArea()
{
    // If there is only one single dock widget and this widget has the
    // DeleteOnClose feature or CustomCloseHandling, then we delete the dock widget now;
    // in the case of CustomCloseHandling, the CDockWidget class will emit its
    // closeRequested signal and not actually delete unless the signal is handled in a way that deletes it
    auto openDockWidgets = openedDockWidgets();
    if (openDockWidgets.count() == 1
        && (openDockWidgets[0]->features().testFlag(DockWidget::DockWidgetDeleteOnClose)
            || openDockWidgets[0]->features().testFlag(DockWidget::CustomCloseHandling))
        && !isAutoHide()) {
        openDockWidgets[0]->closeDockWidgetInternal();
    } else {
        for (auto dockWidget : openedDockWidgets()) {
            if ((dockWidget->features().testFlag(DockWidget::DockWidgetDeleteOnClose)
                 && dockWidget->features().testFlag(DockWidget::DockWidgetForceCloseWithArea))
                || dockWidget->features().testFlag(DockWidget::CustomCloseHandling)) {
                dockWidget->closeDockWidgetInternal();
            } else if (dockWidget->features().testFlag(DockWidget::DockWidgetDeleteOnClose)
                       && isAutoHide()) {
                dockWidget->closeDockWidgetInternal();
            } else {
                dockWidget->toggleView(false);
            }
        }
    }
}

enum eBorderLocation {
    BorderNone = 0,
    BorderLeft = 0x01,
    BorderRight = 0x02,
    BorderTop = 0x04,
    BorderBottom = 0x08,
    BorderVertical = BorderLeft | BorderRight,
    BorderHorizontal = BorderTop | BorderBottom,
    BorderTopLeft = BorderTop | BorderLeft,
    BorderTopRight = BorderTop | BorderRight,
    BorderBottomLeft = BorderBottom | BorderLeft,
    BorderBottomRight = BorderBottom | BorderRight,
    BorderVerticalBottom = BorderVertical | BorderBottom,
    BorderVerticalTop = BorderVertical | BorderTop,
    BorderHorizontalLeft = BorderHorizontal | BorderLeft,
    BorderHorizontalRight = BorderHorizontal | BorderRight,
    BorderAll = BorderVertical | BorderHorizontal
};

SideBarLocation DockAreaWidget::calculateSideTabBarArea() const
{
    auto container = dockContainer();
    auto contentRect = container->contentRect();

    int borders = BorderNone; // contains all borders that are touched by the dock ware
    auto dockAreaTopLeft = mapTo(container, rect().topLeft());
    auto dockAreaRect = rect();
    dockAreaRect.moveTo(dockAreaTopLeft);
    const qreal aspectRatio = dockAreaRect.width() / (qMax(1, dockAreaRect.height()) * 1.0);
    const qreal sizeRatio = (qreal) contentRect.width() / dockAreaRect.width();
    static const int minBorderDistance = 16;
    bool horizontalOrientation = (aspectRatio > 1.0) && (sizeRatio < 3.0);

    // measure border distances - a distance less than 16 px means we touch the border
    int borderDistance[4];

    int distance = qAbs(contentRect.topLeft().y() - dockAreaRect.topLeft().y());
    borderDistance[SideBarLocation::SideBarTop] = (distance < minBorderDistance) ? 0 : distance;
    if (!borderDistance[SideBarLocation::SideBarTop]) {
        borders |= BorderTop;
    }

    distance = qAbs(contentRect.bottomRight().y() - dockAreaRect.bottomRight().y());
    borderDistance[SideBarLocation::SideBarBottom] = (distance < minBorderDistance) ? 0 : distance;
    if (!borderDistance[SideBarLocation::SideBarBottom]) {
        borders |= BorderBottom;
    }

    distance = qAbs(contentRect.topLeft().x() - dockAreaRect.topLeft().x());
    borderDistance[SideBarLocation::SideBarLeft] = (distance < minBorderDistance) ? 0 : distance;
    if (!borderDistance[SideBarLocation::SideBarLeft]) {
        borders |= BorderLeft;
    }

    distance = qAbs(contentRect.bottomRight().x() - dockAreaRect.bottomRight().x());
    borderDistance[SideBarLocation::SideBarRight] = (distance < minBorderDistance) ? 0 : distance;
    if (!borderDistance[SideBarLocation::SideBarRight]) {
        borders |= BorderRight;
    }

    auto sideTab = SideBarLocation::SideBarRight;
    switch (borders) {
    // 1. It's touching all borders
    case BorderAll:
        sideTab = horizontalOrientation ? SideBarLocation::SideBarBottom
                                        : SideBarLocation::SideBarRight;
        break;

    // 2. It's touching 3 borders
    case BorderVerticalBottom:
        sideTab = SideBarLocation::SideBarBottom;
        break;
    case BorderVerticalTop:
        sideTab = SideBarLocation::SideBarTop;
        break;
    case BorderHorizontalLeft:
        sideTab = SideBarLocation::SideBarLeft;
        break;
    case BorderHorizontalRight:
        sideTab = SideBarLocation::SideBarRight;
        break;

    // 3. Its touching horizontal or vertical borders
    case BorderVertical:
        sideTab = SideBarLocation::SideBarBottom;
        break;
    case BorderHorizontal:
        sideTab = SideBarLocation::SideBarRight;
        break;

    // 4. Its in a corner
    case BorderTopLeft:
        sideTab = horizontalOrientation ? SideBarLocation::SideBarTop
                                        : SideBarLocation::SideBarLeft;
        break;
    case BorderTopRight:
        sideTab = horizontalOrientation ? SideBarLocation::SideBarTop
                                        : SideBarLocation::SideBarRight;
        break;
    case BorderBottomLeft:
        sideTab = horizontalOrientation ? SideBarLocation::SideBarBottom
                                        : SideBarLocation::SideBarLeft;
        break;
    case BorderBottomRight:
        sideTab = horizontalOrientation ? SideBarLocation::SideBarBottom
                                        : SideBarLocation::SideBarRight;
        break;

    // 5 Ists touching only one border
    case BorderLeft:
        sideTab = SideBarLocation::SideBarLeft;
        break;
    case BorderRight:
        sideTab = SideBarLocation::SideBarRight;
        break;
    case BorderTop:
        sideTab = SideBarLocation::SideBarTop;
        break;
    case BorderBottom:
        sideTab = SideBarLocation::SideBarBottom;
        break;
    }

    return sideTab;
}

void DockAreaWidget::setAutoHide(bool enable, SideBarLocation location, int tabIndex)
{
    if (!isAutoHideFeatureEnabled())
        return;

    if (!enable) {
        if (isAutoHide())
            d->m_autoHideDockContainer->moveContentsToParent();

        return;
    }

    // If this is already an auto hide container, then move it to new location.
    if (isAutoHide()) {
        d->m_autoHideDockContainer->moveToNewSideBarLocation(location, tabIndex);
        return;
    }

    auto area = (SideBarNone == location) ? calculateSideTabBarArea() : location;
    for (const auto dockWidget : openedDockWidgets()) {
        if (enable == isAutoHide())
            continue;

        if (!dockWidget->features().testFlag(DockWidget::DockWidgetPinnable))
            continue;

        dockContainer()->createAndSetupAutoHideContainer(area, dockWidget, tabIndex++);
    }
}

void DockAreaWidget::toggleAutoHide(SideBarLocation location)
{
    if (!isAutoHideFeatureEnabled())
        return;

    setAutoHide(!isAutoHide(), location);
}

void DockAreaWidget::closeOtherAreas()
{
    dockContainer()->closeOtherAreas(this);
}

void DockAreaWidget::setFloating()
{
    d->m_titleBar->setAreaFloating();
}

DockAreaTitleBar *DockAreaWidget::titleBar() const
{
    return d->m_titleBar;
}

bool DockAreaWidget::isCentralWidgetArea() const
{
    if (dockWidgetsCount() != 1)
        return false;

    return dockManager()->centralWidget() == dockWidgets().constFirst();
}

bool DockAreaWidget::containsCentralWidget() const
{
    auto centralWidget = dockManager()->centralWidget();
    for (const auto &dockWidget : dockWidgets()) {
        if (dockWidget == centralWidget)
            return true;
    }

    return false;
}

QSize DockAreaWidget::minimumSizeHint() const
{
    if (!d->m_minSizeHint.isValid())
        return Super::minimumSizeHint();

    if (d->m_titleBar->isVisible())
        return d->m_minSizeHint + QSize(0, d->m_titleBar->minimumSizeHint().height());
    else
        return d->m_minSizeHint;
}

void DockAreaWidget::onDockWidgetFeaturesChanged()
{
    if (d->m_titleBar)
        d->updateTitleBarButtonStates();
}

bool DockAreaWidget::isTopLevelArea() const
{
    auto container = dockContainer();
    if (!container)
        return false;

    return (container->topLevelDockArea() == this);
}

#ifdef Q_OS_WIN
bool DockAreaWidget::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::PlatformSurface:
        return true;
    default:
        break;
    }

    return Super::event(e);
}
#endif

} // namespace ADS
