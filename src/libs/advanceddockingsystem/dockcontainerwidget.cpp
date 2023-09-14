// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#include "dockcontainerwidget.h"

#include "ads_globals_p.h"
#include "autohidedockcontainer.h"
#include "autohidesidebar.h"
#include "autohidetab.h"
#include "dockareawidget.h"
#include "dockingstatereader.h"
#include "dockmanager.h"
#include "dockoverlay.h"
#include "docksplitter.h"
#include "dockwidget.h"
#include "floatingdockcontainer.h"

#include <QAbstractButton>
#include <QDebug>
#include <QGridLayout>
#include <QList>
#include <QLoggingCategory>
#include <QMouseEvent>
#include <QPointer>
#include <QTimer>
#include <QVariant>
#include <QXmlStreamWriter>

#include <functional>
#include <iostream>

namespace ADS {

static unsigned int g_zOrderCounter = 0;

enum eDropMode {
    DropModeIntoArea,      ///< drop widget into a dock area
    DropModeIntoContainer, ///< drop into container
    DropModeInvalid        ///< invalid mode - do not drop
};

/**
 * Converts dock area ID to an index for array access
 */
static int areaIdToIndex(DockWidgetArea area)
{
    switch (area) {
    case LeftDockWidgetArea:
        return 0;
    case RightDockWidgetArea:
        return 1;
    case TopDockWidgetArea:
        return 2;
    case BottomDockWidgetArea:
        return 3;
    case CenterDockWidgetArea:
        return 4;
    default:
        return 4;
    }
}

/**
 * Helper function to ease insertion of dock area into splitter
 */
static void insertWidgetIntoSplitter(QSplitter *splitter, QWidget *widget, bool append)
{
    if (append)
        splitter->addWidget(widget);
    else
        splitter->insertWidget(0, widget);
}

/**
 * Private data class of DockContainerWidget class (pimpl)
 */
class DockContainerWidgetPrivate
{
public:
    DockContainerWidget *q;
    QPointer<DockManager> m_dockManager;
    unsigned int m_zOrderIndex = 0;
    QList<DockAreaWidget *> m_dockAreas;
    QList<AutoHideDockContainer *> m_autoHideWidgets;
    QMap<SideBarLocation, AutoHideSideBar *> m_sideTabBarWidgets;
    QGridLayout *m_layout = nullptr;
    DockSplitter *m_rootSplitter = nullptr;
    bool m_isFloating = false;
    DockAreaWidget *m_lastAddedAreaCache[5];
    int m_visibleDockAreaCount = -1;
    DockAreaWidget *m_topLevelDockArea = nullptr;
    QTimer m_delayedAutoHideTimer;
    AutoHideTab *m_delayedAutoHideTab;
    bool m_delayedAutoHideShow = false;

    /**
     * Private data constructor
     */
    DockContainerWidgetPrivate(DockContainerWidget *parent);

    /**
     * Adds dock widget to container and returns the dock area that contains
     * the inserted dock widget
     */
    DockAreaWidget *addDockWidgetToContainer(DockWidgetArea area, DockWidget *dockWidget);

    /**
     * Adds dock widget to a existing DockWidgetArea
     */
    DockAreaWidget *addDockWidgetToDockArea(DockWidgetArea area,
                                            DockWidget *dockWidget,
                                            DockAreaWidget *targetDockArea,
                                            int index = -1);

    /**
     * Add dock area to this container
     */
    void addDockArea(DockAreaWidget *newDockWidget, DockWidgetArea area = CenterDockWidgetArea);

    /**
     * Drop floating widget into container
     */
    void dropIntoContainer(FloatingDockContainer *floatingWidget, DockWidgetArea area);

    /**
     * Drop floating widget into auto hide side bar
     */
    void dropIntoAutoHideSideBar(FloatingDockContainer *floatingWidget, DockWidgetArea area);

    /**
     * Creates a new tab for a widget dropped into the center of a section
     */
    void dropIntoCenterOfSection(FloatingDockContainer *floatingWidget,
                                 DockAreaWidget *targetArea,
                                 int tabIndex);

    /**
     * Drop floating widget into dock area
     */
    void dropIntoSection(FloatingDockContainer *floatingWidget,
                         DockAreaWidget *targetArea,
                         DockWidgetArea area,
                         int tabIndex = 0);

    /**
     * Moves the dock widget or dock area given in Widget parameter to a new dock widget area.
     */
    void moveToNewSection(QWidget *widget,
                          DockAreaWidget *targetArea,
                          DockWidgetArea area,
                          int tabIndex = 0);

    /**
     * Moves the dock widget or dock area given in Widget parameter to a dock area in container.
     */
    void moveToContainer(QWidget *widget, DockWidgetArea area);

    /**
     * Creates a new tab for a widget dropped into the center of a section
     */
    void moveIntoCenterOfSection(QWidget *widget, DockAreaWidget *targetArea, int tabIndex = 0);

    /**
     * Moves the dock widget or dock area given in Widget parameter to
     * a auto hide sidebar area
     */
    void moveToAutoHideSideBar(QWidget *widget,
                               DockWidgetArea area,
                               int tabIndex = TabDefaultInsertIndex);

    /**
     * Adds new dock areas to the internal dock area list
     */
    void addDockAreasToList(const QList<DockAreaWidget *> newDockAreas);

    /**
     * Wrapper function for DockAreas append, that ensures that dock area signals
     * are properly connected to dock container slots
     */
    void appendDockAreas(const QList<DockAreaWidget *> newDockAreas);

    /**
     * Save state of child nodes
     */
    void saveChildNodesState(QXmlStreamWriter &stream, QWidget *widget);

    /**
     * Save state of auto hide widgets
     */
    void saveAutoHideWidgetsState(QXmlStreamWriter &stream);

    /**
     * Restore state of child nodes.
     * \param[in] Stream The data stream that contains the serialized state
     * \param[out] CreatedWidget The widget created from parsed data or 0 if
     * the parsed widget was an empty splitter
     * \param[in] Testing If Testing is true, only the stream data is
     * parsed without modifiying anything.
     */
    bool restoreChildNodes(DockingStateReader &stateReader, QWidget *&createdWidget, bool testing);

    /**
     * Restores a splitter.
     * \see restoreChildNodes() for details
     */
    bool restoreSplitter(DockingStateReader &stateReader, QWidget *&createdWidget, bool testing);

    /**
     * Restores a dock area.
     * \see restoreChildNodes() for details
     */
    bool restoreDockArea(DockingStateReader &stateReader, QWidget *&createdWidget, bool testing);

    /**
     * Restores a auto hide side bar
     */
    bool restoreSideBar(DockingStateReader &stream, QWidget *&createdWidget, bool testing);

    /**
     * Helper function for recursive dumping of layout
     */
    void dumpRecursive(int level, QWidget *widget) const;

    /**
     * Calculate the drop mode from the given target position
     */
    eDropMode getDropMode(const QPoint &targetPosition);

    /**
     * Initializes the visible dock area count variable if it is not initialized
     * yet
     */
    void initVisibleDockAreaCount()
    {
        if (m_visibleDockAreaCount > -1)
            return;

        m_visibleDockAreaCount = 0;
        for (auto dockArea : std::as_const(m_dockAreas))
            m_visibleDockAreaCount += dockArea->isHidden() ? 0 : 1;
    }

    /**
     * Access function for the visible dock area counter
     */
    int visibleDockAreaCount()
    {
        // Lazy initialization - we initialize the m_visibleDockAreaCount variable
        // on first use
        initVisibleDockAreaCount();
        return m_visibleDockAreaCount;
    }

    /**
     * The visible dock area count changes, if dock areas are remove, added or
     * when its view is toggled
     */
    void onVisibleDockAreaCountChanged();

    void emitDockAreasRemoved()
    {
        onVisibleDockAreaCountChanged();
        emit q->dockAreasRemoved();
    }

    void emitDockAreasAdded()
    {
        onVisibleDockAreaCountChanged();
        emit q->dockAreasAdded();
    }

    /**
     * Helper function for creation of new splitter
     */
    DockSplitter *createSplitter(Qt::Orientation orientation, QWidget *parent = nullptr)
    {
        auto *splitter = new DockSplitter(orientation, parent);
        splitter->setOpaqueResize(DockManager::testConfigFlag(DockManager::OpaqueSplitterResize));
        splitter->setChildrenCollapsible(false);
        return splitter;
    }

    /**
     * Ensures equal distribution of the sizes of a splitter if an dock widget
     * is inserted from code
     */
    void adjustSplitterSizesOnInsertion(QSplitter *splitter, qreal lastRatio = 1.0)
    {
        const int areaSize = (splitter->orientation() == Qt::Horizontal) ? splitter->width()
                                                                         : splitter->height();
        auto splitterSizes = splitter->sizes();

        const qreal totalRatio = splitterSizes.size() - 1.0 + lastRatio;
        for (int i = 0; i < splitterSizes.size() - 1; ++i)
            splitterSizes[i] = areaSize / totalRatio;

        splitterSizes.back() = areaSize * lastRatio / totalRatio;
        splitter->setSizes(splitterSizes);
    }

    /**
     * This function forces the dock container widget to update handles of splitters
     * based if a central widget exists.
     */
    void updateSplitterHandles(QSplitter *splitter);

    /**
     * If no central widget exists, the widgets resize with the container.
     * If a central widget exists, the widgets surrounding the central widget
     * do not resize its height or width.
     */
    bool widgetResizesWithContainer(QWidget *widget);

    void onDockAreaViewToggled(DockAreaWidget *dockArea, bool visible)
    {
        m_visibleDockAreaCount += visible ? 1 : -1;
        onVisibleDockAreaCountChanged();
        emit q->dockAreaViewToggled(dockArea, visible);
    }
}; // struct DockContainerWidgetPrivate

DockContainerWidgetPrivate::DockContainerWidgetPrivate(DockContainerWidget *parent)
    : q(parent)
{
    std::fill(std::begin(m_lastAddedAreaCache), std::end(m_lastAddedAreaCache), nullptr);
    m_delayedAutoHideTimer.setSingleShot(true);
    m_delayedAutoHideTimer.setInterval(500);
    QObject::connect(&m_delayedAutoHideTimer, &QTimer::timeout, q, [this]() {
        auto globalPos = m_delayedAutoHideTab->mapToGlobal(QPoint(0, 0));
        qApp->sendEvent(m_delayedAutoHideTab,
                        new QMouseEvent(QEvent::MouseButtonPress,
                                        QPoint(0, 0),
                                        globalPos,
                                        Qt::LeftButton,
                                        {Qt::LeftButton},
                                        Qt::NoModifier));
    });
}

eDropMode DockContainerWidgetPrivate::getDropMode(const QPoint &targetPosition)
{
    DockAreaWidget *dockArea = q->dockAreaAt(targetPosition);
    auto dropArea = InvalidDockWidgetArea;
    auto containerDropArea = m_dockManager->containerOverlay()->dropAreaUnderCursor();

    if (dockArea) {
        auto dropOverlay = m_dockManager->dockAreaOverlay();
        dropOverlay->setAllowedAreas(dockArea->allowedAreas());
        dropArea = dropOverlay->showOverlay(dockArea);
        if (containerDropArea != InvalidDockWidgetArea && containerDropArea != dropArea)
            dropArea = InvalidDockWidgetArea;

        if (dropArea != InvalidDockWidgetArea) {
            qCInfo(adsLog) << "Dock Area Drop Content: " << dropArea;
            return DropModeIntoArea;
        }
    }

    // mouse is over container
    if (InvalidDockWidgetArea == dropArea) {
        dropArea = containerDropArea;
        qCInfo(adsLog) << "Container Drop Content: " << dropArea;
        if (dropArea != InvalidDockWidgetArea)
            return DropModeIntoContainer;
    }

    return DropModeInvalid;
}

void DockContainerWidgetPrivate::onVisibleDockAreaCountChanged()
{
    auto topLevelDockArea = q->topLevelDockArea();

    if (topLevelDockArea) {
        this->m_topLevelDockArea = topLevelDockArea;
        topLevelDockArea->updateTitleBarButtonVisibility(true);
    } else if (this->m_topLevelDockArea) {
        this->m_topLevelDockArea->updateTitleBarButtonVisibility(false);
        this->m_topLevelDockArea = nullptr;
    }
}

void DockContainerWidgetPrivate::dropIntoContainer(FloatingDockContainer *floatingWidget,
                                                   DockWidgetArea area)
{
    auto insertParam = internal::dockAreaInsertParameters(area);
    DockContainerWidget *floatingDockContainer = floatingWidget->dockContainer();
    auto newDockAreas
        = floatingDockContainer->findChildren<DockAreaWidget *>(QString(),
                                                                Qt::FindChildrenRecursively);
    auto *splitter = m_rootSplitter;

    if (m_dockAreas.count() <= 1) {
        splitter->setOrientation(insertParam.orientation());
    } else if (splitter->orientation() != insertParam.orientation()) {
        auto *newSplitter = createSplitter(insertParam.orientation());
        QLayoutItem *layoutItem = m_layout->replaceWidget(splitter, newSplitter);
        newSplitter->addWidget(splitter);
        updateSplitterHandles(newSplitter);
        splitter = newSplitter;
        delete layoutItem;
    }

    // Now we can insert the floating widget content into this container
    auto floatingSplitter = floatingDockContainer->rootSplitter();
    if (floatingSplitter->count() == 1) {
        insertWidgetIntoSplitter(splitter, floatingSplitter->widget(0), insertParam.append());
        updateSplitterHandles(splitter);
    } else if (floatingSplitter->orientation() == insertParam.orientation()) {
        int insertIndex = insertParam.append() ? splitter->count() : 0;
        while (floatingSplitter->count()) {
            splitter->insertWidget(insertIndex++, floatingSplitter->widget(0));
            updateSplitterHandles(splitter);
        }
    } else {
        insertWidgetIntoSplitter(splitter, floatingSplitter, insertParam.append());
    }

    m_rootSplitter = splitter;
    addDockAreasToList(newDockAreas);

    // If we dropped the floating widget into the main dock container that does
    // not contain any dock widgets, then splitter is invisible and we need to
    // show it to display the docked widgets
    if (!splitter->isVisible())
        splitter->show();

    q->dumpLayout();
}

void DockContainerWidgetPrivate::dropIntoAutoHideSideBar(FloatingDockContainer *floatingWidget,
                                                         DockWidgetArea area)
{
    auto sideBarLocation = internal::toSideBarLocation(area);
    auto newDockAreas = floatingWidget->findChildren<DockAreaWidget *>(QString(),
                                                                       Qt::FindChildrenRecursively);
    int tabIndex = m_dockManager->containerOverlay()->tabIndexUnderCursor();
    for (auto dockArea : newDockAreas) {
        auto dockWidgets = dockArea->dockWidgets();
        for (auto dockWidget : dockWidgets)
            q->createAndSetupAutoHideContainer(sideBarLocation, dockWidget, tabIndex++);
    }
}

void DockContainerWidgetPrivate::dropIntoCenterOfSection(FloatingDockContainer *floatingWidget,
                                                         DockAreaWidget *targetArea,
                                                         int tabIndex)
{
    DockContainerWidget *floatingContainer = floatingWidget->dockContainer();
    auto newDockWidgets = floatingContainer->dockWidgets();
    auto topLevelDockArea = floatingContainer->topLevelDockArea();
    int newCurrentIndex = -1;
    tabIndex = qMax(0, tabIndex);

    // If the floating widget contains only one single dock are, then the current dock widget of
    // the dock area will also be the future current dock widget in the drop area.
    if (topLevelDockArea)
        newCurrentIndex = topLevelDockArea->currentIndex();

    for (int i = 0; i < newDockWidgets.count(); ++i) {
        DockWidget *dockWidget = newDockWidgets[i];
        targetArea->insertDockWidget(tabIndex + i, dockWidget, false);
        // If the floating widget contains multiple visible dock areas, then we simply pick the
        // first visible open dock widget and make it the current one.
        if (newCurrentIndex < 0 && !dockWidget->isClosed())
            newCurrentIndex = i;
    }
    targetArea->setCurrentIndex(newCurrentIndex + tabIndex);
    targetArea->updateTitleBarVisibility();
    return;
}

void DockContainerWidgetPrivate::dropIntoSection(FloatingDockContainer *floatingWidget,
                                                 DockAreaWidget *targetArea,
                                                 DockWidgetArea area,
                                                 int tabIndex)
{
    // Dropping into center means all dock widgets in the dropped floating widget will become
    // tabs of the drop area.
    if (CenterDockWidgetArea == area) {
        dropIntoCenterOfSection(floatingWidget, targetArea, tabIndex);
        return;
    }

    DockContainerWidget *floatingContainer = floatingWidget->dockContainer();
    auto insertParam = internal::dockAreaInsertParameters(area);
    auto newDockAreas
        = floatingContainer->findChildren<DockAreaWidget *>(QString(), Qt::FindChildrenRecursively);
    QSplitter *targetAreaSplitter = internal::findParent<QSplitter *>(targetArea);

    if (!targetAreaSplitter) {
        QSplitter *splitter = createSplitter(insertParam.orientation());
        m_layout->replaceWidget(targetArea, splitter);
        splitter->addWidget(targetArea);
        targetAreaSplitter = splitter;
    }
    int areaIndex = targetAreaSplitter->indexOf(targetArea);
    auto floatingSplitter = floatingContainer->rootSplitter();

    if (targetAreaSplitter->orientation() == insertParam.orientation()) {
        auto sizes = targetAreaSplitter->sizes();
        int targetAreaSize = (insertParam.orientation() == Qt::Horizontal) ? targetArea->width()
                                                                           : targetArea->height();
        bool adjustSplitterSizes = true;
        if ((floatingSplitter->orientation() != insertParam.orientation())
            && floatingSplitter->count() > 1) {
            targetAreaSplitter->insertWidget(areaIndex + insertParam.insertOffset(),
                                             floatingSplitter);
            updateSplitterHandles(targetAreaSplitter);
        } else {
            adjustSplitterSizes = (floatingSplitter->count() == 1);
            int insertIndex = areaIndex + insertParam.insertOffset();
            while (floatingSplitter->count()) {
                targetAreaSplitter->insertWidget(insertIndex++, floatingSplitter->widget(0));
                updateSplitterHandles(targetAreaSplitter);
            }
        }

        if (adjustSplitterSizes) {
            int size = (targetAreaSize - targetAreaSplitter->handleWidth()) / 2;
            sizes[areaIndex] = size;
            sizes.insert(areaIndex, size);
            targetAreaSplitter->setSizes(sizes);
        }
    } else {
        QSplitter *newSplitter = createSplitter(insertParam.orientation());
        int targetAreaSize = (insertParam.orientation() == Qt::Horizontal) ? targetArea->width()
                                                                           : targetArea->height();
        bool adjustSplitterSizes = true;
        if ((floatingSplitter->orientation() != insertParam.orientation())
            && floatingSplitter->count() > 1) {
            newSplitter->addWidget(floatingSplitter);
            updateSplitterHandles(newSplitter);
        } else {
            adjustSplitterSizes = (floatingSplitter->count() == 1);
            while (floatingSplitter->count()) {
                newSplitter->addWidget(floatingSplitter->widget(0));
                updateSplitterHandles(newSplitter);
            }
        }

        // Save the sizes before insertion and restore it later to prevent
        // shrinking of existing area
        auto sizes = targetAreaSplitter->sizes();
        insertWidgetIntoSplitter(newSplitter, targetArea, !insertParam.append());
        updateSplitterHandles(newSplitter);
        if (adjustSplitterSizes) {
            int size = targetAreaSize / 2;
            newSplitter->setSizes({size, size});
        }
        targetAreaSplitter->insertWidget(areaIndex, newSplitter);
        targetAreaSplitter->setSizes(sizes);
        updateSplitterHandles(targetAreaSplitter);
    }

    addDockAreasToList(newDockAreas);
    q->dumpLayout();
}

void DockContainerWidgetPrivate::moveToNewSection(QWidget *widget,
                                                  DockAreaWidget *targetArea,
                                                  DockWidgetArea area,
                                                  int tabIndex)
{
    // Dropping into center means all dock widgets in the dropped floating widget will become
    // tabs of the drop area.
    if (CenterDockWidgetArea == area) {
        moveIntoCenterOfSection(widget, targetArea, tabIndex);
        return;
    }

    DockWidget *droppedDockWidget = qobject_cast<DockWidget *>(widget);
    DockAreaWidget *droppedDockArea = qobject_cast<DockAreaWidget *>(widget);
    DockAreaWidget *newDockArea;
    if (droppedDockWidget) {
        newDockArea = new DockAreaWidget(m_dockManager, q);
        DockAreaWidget *oldDockArea = droppedDockWidget->dockAreaWidget();
        if (oldDockArea)
            oldDockArea->removeDockWidget(droppedDockWidget);

        newDockArea->addDockWidget(droppedDockWidget);
    } else {
        droppedDockArea->dockContainer()->removeDockArea(droppedDockArea);
        newDockArea = droppedDockArea;
    }

    auto insertParam = internal::dockAreaInsertParameters(area);
    QSplitter *targetAreaSplitter = internal::findParent<QSplitter *>(targetArea);
    const int areaIndex = targetAreaSplitter->indexOf(targetArea);
    auto sizes = targetAreaSplitter->sizes();
    if (targetAreaSplitter->orientation() == insertParam.orientation()) {
        const int targetAreaSize = (insertParam.orientation() == Qt::Horizontal)
                                       ? targetArea->width()
                                       : targetArea->height();
        targetAreaSplitter->insertWidget(areaIndex + insertParam.insertOffset(), newDockArea);
        updateSplitterHandles(targetAreaSplitter);
        const int size = (targetAreaSize - targetAreaSplitter->handleWidth()) / 2;
        sizes[areaIndex] = size;
        sizes.insert(areaIndex, size);
    } else {
        const int targetAreaSize = (insertParam.orientation() == Qt::Horizontal)
                                       ? targetArea->width()
                                       : targetArea->height();
        QSplitter *newSplitter = createSplitter(insertParam.orientation());
        newSplitter->addWidget(targetArea);
        insertWidgetIntoSplitter(newSplitter, newDockArea, insertParam.append());
        updateSplitterHandles(newSplitter);
        const int size = targetAreaSize / 2;
        newSplitter->setSizes({size, size});
        targetAreaSplitter->insertWidget(areaIndex, newSplitter);
        updateSplitterHandles(targetAreaSplitter);
    }
    targetAreaSplitter->setSizes(sizes);

    addDockAreasToList({newDockArea});
}

void DockContainerWidgetPrivate::moveToContainer(QWidget *widget, DockWidgetArea area)
{
    DockWidget *droppedDockWidget = qobject_cast<DockWidget *>(widget);
    DockAreaWidget *droppedDockArea = qobject_cast<DockAreaWidget *>(widget);
    DockAreaWidget *newDockArea = nullptr;

    if (droppedDockWidget) {
        newDockArea = new DockAreaWidget(m_dockManager, q);
        DockAreaWidget *oldDockArea = droppedDockWidget->dockAreaWidget();
        if (oldDockArea)
            oldDockArea->removeDockWidget(droppedDockWidget);

        newDockArea->addDockWidget(droppedDockWidget);
    } else {
        // We check, if we insert the dropped widget into the same place that
        // it already has and do nothing, if it is the same place. It would
        // also work without this check, but it looks nicer with the check
        // because there will be no layout updates
        auto splitter = internal::findParent<DockSplitter *>(droppedDockArea);
        auto insertParam = internal::dockAreaInsertParameters(area);
        if (splitter == m_rootSplitter && insertParam.orientation() == splitter->orientation()) {
            if (insertParam.append() && splitter->lastWidget() == droppedDockArea)
                return;
            else if (!insertParam.append() && splitter->firstWidget() == droppedDockArea)
                return;
        }
        droppedDockArea->dockContainer()->removeDockArea(droppedDockArea);
        newDockArea = droppedDockArea;
    }

    addDockArea(newDockArea, area);
    m_lastAddedAreaCache[areaIdToIndex(area)] = newDockArea;
}

void DockContainerWidgetPrivate::moveIntoCenterOfSection(QWidget *widget,
                                                         DockAreaWidget *targetArea,
                                                         int tabIndex)
{
    auto droppedDockWidget = qobject_cast<DockWidget *>(widget);
    auto droppedArea = qobject_cast<DockAreaWidget *>(widget);
    tabIndex = qMax(0, tabIndex);

    if (droppedDockWidget) {
        DockAreaWidget *oldDockArea = droppedDockWidget->dockAreaWidget();
        if (oldDockArea == targetArea)
            return;

        if (oldDockArea)
            oldDockArea->removeDockWidget(droppedDockWidget);

        targetArea->insertDockWidget(tabIndex, droppedDockWidget, true);
    } else {
        QList<DockWidget *> newDockWidgets = droppedArea->dockWidgets();
        int newCurrentIndex = droppedArea->currentIndex();
        for (int i = 0; i < newDockWidgets.count(); ++i) {
            DockWidget *dockWidget = newDockWidgets[i];
            targetArea->insertDockWidget(tabIndex + i, dockWidget, false);
        }
        targetArea->setCurrentIndex(tabIndex + newCurrentIndex);
        droppedArea->dockContainer()->removeDockArea(droppedArea);
        droppedArea->deleteLater();
    }

    targetArea->updateTitleBarVisibility();
    return;
}

void DockContainerWidgetPrivate::moveToAutoHideSideBar(QWidget *widget,
                                                       DockWidgetArea area,
                                                       int tabIndex)
{
    DockWidget *droppedDockWidget = qobject_cast<DockWidget *>(widget);
    DockAreaWidget *droppedDockArea = qobject_cast<DockAreaWidget *>(widget);
    auto sideBarLocation = internal::toSideBarLocation(area);

    if (droppedDockWidget) {
        if (q == droppedDockWidget->dockContainer())
            droppedDockWidget->setAutoHide(true, sideBarLocation, tabIndex);
        else
            q->createAndSetupAutoHideContainer(sideBarLocation, droppedDockWidget, tabIndex);

    } else {
        if (q == droppedDockArea->dockContainer()) {
            droppedDockArea->setAutoHide(true, sideBarLocation, tabIndex);
        } else {
            for (const auto dockWidget : droppedDockArea->openedDockWidgets()) {
                if (!dockWidget->features().testFlag(DockWidget::DockWidgetPinnable))
                    continue;

                q->createAndSetupAutoHideContainer(sideBarLocation, dockWidget, tabIndex++);
            }
        }
    }
}

void DockContainerWidgetPrivate::updateSplitterHandles(QSplitter *splitter)
{
    if (!m_dockManager->centralWidget() || !splitter)
        return;

    for (int i = 0; i < splitter->count(); ++i)
        splitter->setStretchFactor(i, widgetResizesWithContainer(splitter->widget(i)) ? 1 : 0);
}

bool DockContainerWidgetPrivate::widgetResizesWithContainer(QWidget *widget)
{
    if (!m_dockManager->centralWidget())
        return true;

    auto area = qobject_cast<DockAreaWidget *>(widget);
    if (area)
        return area->isCentralWidgetArea();

    auto innerSplitter = qobject_cast<DockSplitter *>(widget);
    if (innerSplitter)
        return innerSplitter->isResizingWithContainer();

    return false;
}

void DockContainerWidgetPrivate::addDockAreasToList(const QList<DockAreaWidget *> newDockAreas)
{
    const int countBefore = m_dockAreas.count();
    const int newAreaCount = newDockAreas.count();
    appendDockAreas(newDockAreas);
    // If the user dropped a floating widget that contains only one single
    // visible dock area, then its title bar button TitleBarButtonUndock is
    // likely hidden. We need to ensure, that it is visible
    for (auto dockArea : newDockAreas) {
        dockArea->titleBarButton(TitleBarButtonUndock)->setVisible(true);
        dockArea->titleBarButton(TitleBarButtonClose)->setVisible(true);
    }

    // We need to ensure, that the dock area title bar is visible. The title bar
    // is invisible, if the dock are is a single dock area in a floating widget.
    if (1 == countBefore)
        m_dockAreas.at(0)->updateTitleBarVisibility();

    if (1 == newAreaCount)
        m_dockAreas.last()->updateTitleBarVisibility();

    emitDockAreasAdded();
}

void DockContainerWidgetPrivate::appendDockAreas(const QList<DockAreaWidget *> newDockAreas)
{
    m_dockAreas.append(newDockAreas);
    for (auto dockArea : newDockAreas) {
        QObject::connect(dockArea,
                         &DockAreaWidget::viewToggled,
                         q,
                         std::bind(&DockContainerWidgetPrivate::onDockAreaViewToggled,
                                   this,
                                   dockArea,
                                   std::placeholders::_1));
    }
}

void DockContainerWidgetPrivate::saveChildNodesState(QXmlStreamWriter &stream, QWidget *widget)
{
    QSplitter *splitter = qobject_cast<QSplitter *>(widget);
    if (splitter) {
        stream.writeStartElement("splitter");
        stream.writeAttribute("orientation",
                              QVariant::fromValue(splitter->orientation()).toString());
        stream.writeAttribute("count", QString::number(splitter->count()));
        qCInfo(adsLog) << "NodeSplitter orientation:" << splitter->orientation()
                       << "WidgetCount:" << splitter->count();
        for (int i = 0; i < splitter->count(); ++i)
            saveChildNodesState(stream, splitter->widget(i));

        stream.writeStartElement("sizes");
        QStringList sizes;
        for (auto size : splitter->sizes())
            sizes.append(QString::number(size));

        stream.writeCharacters(sizes.join(" "));
        stream.writeEndElement(); // sizes
        stream.writeEndElement(); // splitter
    } else {
        DockAreaWidget *dockArea = qobject_cast<DockAreaWidget *>(widget);
        if (dockArea)
            dockArea->saveState(stream);
    }
}

void DockContainerWidgetPrivate::saveAutoHideWidgetsState(QXmlStreamWriter &s)
{
    for (const auto sideTabBar : m_sideTabBarWidgets.values()) {
        if (!sideTabBar->count())
            continue;

        sideTabBar->saveState(s);
    }
}

bool DockContainerWidgetPrivate::restoreChildNodes(DockingStateReader &stateReader,
                                                   QWidget *&createdWidget,
                                                   bool testing)
{
    bool result = true;
    while (stateReader.readNextStartElement()) {
        if (stateReader.name() == QLatin1String("splitter")) {
            result = restoreSplitter(stateReader, createdWidget, testing);
            qCInfo(adsLog) << "Splitter";
        } else if (stateReader.name() == QLatin1String("area")) {
            result = restoreDockArea(stateReader, createdWidget, testing);
            qCInfo(adsLog) << "DockAreaWidget";
        } else if (stateReader.name() == QLatin1String("sideBar")) {
            result = restoreSideBar(stateReader, createdWidget, testing);
            qCInfo(adsLog) << "SideBar";
        } else {
            stateReader.skipCurrentElement();
            qCInfo(adsLog) << "Unknown element";
        }
    }

    return result;
}

bool DockContainerWidgetPrivate::restoreSplitter(DockingStateReader &stateReader,
                                                 QWidget *&createdWidget,
                                                 bool testing)
{
    QVariant orientationVar = QVariant(stateReader.attributes().value("orientation").toString());

    // Check if the orientation string is convertable
    if (!orientationVar.canConvert<Qt::Orientation>())
        return false;

    Qt::Orientation orientation = orientationVar.value<Qt::Orientation>();

    bool ok;
    int widgetCount = stateReader.attributes().value("count").toInt(&ok);
    if (!ok)
        return false;

    qCInfo(adsLog) << "Restore NodeSplitter Orientation:" << orientation
                   << "WidgetCount:" << widgetCount;
    QSplitter *splitter = nullptr;
    if (!testing)
        splitter = createSplitter(orientation);

    bool visible = false;
    QList<int> sizes;
    while (stateReader.readNextStartElement()) {
        QWidget *childNode = nullptr;
        bool result = true;
        if (stateReader.name() == QLatin1String("splitter")) {
            result = restoreSplitter(stateReader, childNode, testing);
        } else if (stateReader.name() == QLatin1String("area")) {
            result = restoreDockArea(stateReader, childNode, testing);
        } else if (stateReader.name() == QLatin1String("sizes")) {
            QString size = stateReader.readElementText().trimmed();
            qCInfo(adsLog) << "Size:" << size;
            QTextStream textStream(&size);
            while (!textStream.atEnd()) {
                int value;
                textStream >> value;
                sizes.append(value);
            }
        } else {
            stateReader.skipCurrentElement();
        }

        if (!result)
            return false;

        if (testing || !childNode)
            continue;

        qCInfo(adsLog) << "ChildNode isVisible" << childNode->isVisible() << "isVisibleTo"
                       << childNode->isVisibleTo(splitter);
        splitter->addWidget(childNode);
        visible |= childNode->isVisibleTo(splitter);
    }

    if (sizes.count() != widgetCount)
        return false;

    if (!testing) {
        if (!splitter->count()) {
            delete splitter;
            splitter = nullptr;
        } else {
            splitter->setSizes(sizes);
            splitter->setVisible(visible);
        }
        createdWidget = splitter;
    } else {
        createdWidget = nullptr;
    }

    return true;
}

bool DockContainerWidgetPrivate::restoreDockArea(DockingStateReader &stateReader,
                                                 QWidget *&createdWidget,
                                                 bool testing)
{
    DockAreaWidget *dockArea = nullptr;
    auto result = DockAreaWidget::restoreState(stateReader, dockArea, testing, q);
    if (result && dockArea)
        appendDockAreas({dockArea});

    createdWidget = dockArea;
    return result;
}

bool DockContainerWidgetPrivate::restoreSideBar(DockingStateReader &stateReader,
                                                QWidget *&createdWidget,
                                                bool testing)
{
    Q_UNUSED(createdWidget)
    // Simply ignore side bar auto hide widgets from saved state if  auto hide support is disabled
    if (!DockManager::testAutoHideConfigFlag(DockManager::AutoHideFeatureEnabled))
        return true;

    bool ok;
    auto area = (SideBarLocation) stateReader.attributes().value("area").toInt(&ok);
    if (!ok)
        return false;

    while (stateReader.readNextStartElement()) {
        if (stateReader.name() != QLatin1String("widget"))
            continue;

        auto name = stateReader.attributes().value("name");
        if (name.isEmpty())
            return false;

        bool ok;
        bool closed = stateReader.attributes().value("closed").toInt(&ok);
        if (!ok)
            return false;

        int size = stateReader.attributes().value("size").toInt(&ok);
        if (!ok)
            return false;

        stateReader.skipCurrentElement();
        DockWidget *dockWidget = m_dockManager->findDockWidget(name.toString());
        if (!dockWidget || testing)
            continue;

        auto sideBar = q->autoHideSideBar(area);
        AutoHideDockContainer *autoHideContainer;
        if (dockWidget->isAutoHide()) {
            autoHideContainer = dockWidget->autoHideDockContainer();
            if (autoHideContainer->autoHideSideBar() != sideBar)
                sideBar->addAutoHideWidget(autoHideContainer);
        } else {
            autoHideContainer = sideBar->insertDockWidget(-1, dockWidget);
        }
        autoHideContainer->setSize(size);
        dockWidget->setProperty(internal::g_closedProperty, closed);
        dockWidget->setProperty(internal::g_dirtyProperty, false);
    }

    return true;
}

DockAreaWidget *DockContainerWidgetPrivate::addDockWidgetToContainer(DockWidgetArea area,
                                                                     DockWidget *dockWidget)
{
    DockAreaWidget *newDockArea = new DockAreaWidget(m_dockManager, q);
    newDockArea->addDockWidget(dockWidget);
    addDockArea(newDockArea, area);
    newDockArea->updateTitleBarVisibility();
    m_lastAddedAreaCache[areaIdToIndex(area)] = newDockArea;
    return newDockArea;
}

void DockContainerWidgetPrivate::addDockArea(DockAreaWidget *newDockArea, DockWidgetArea area)
{
    auto insertParam = internal::dockAreaInsertParameters(area);
    // As long as we have only one dock area in the splitter we can adjust its orientation
    if (m_dockAreas.count() <= 1)
        m_rootSplitter->setOrientation(insertParam.orientation());

    auto *splitter = m_rootSplitter;
    if (splitter->orientation() == insertParam.orientation()) {
        insertWidgetIntoSplitter(splitter, newDockArea, insertParam.append());
        updateSplitterHandles(splitter);
        if (splitter->isHidden())
            splitter->show();

    } else {
        auto *newSplitter = createSplitter(insertParam.orientation());
        if (insertParam.append()) {
            QLayoutItem *layoutItem = m_layout->replaceWidget(splitter, newSplitter);
            newSplitter->addWidget(splitter);
            newSplitter->addWidget(newDockArea);
            updateSplitterHandles(newSplitter);
            delete layoutItem;
        } else {
            newSplitter->addWidget(newDockArea);
            QLayoutItem *layoutItem = m_layout->replaceWidget(splitter, newSplitter);
            newSplitter->addWidget(splitter);
            updateSplitterHandles(newSplitter);
            delete layoutItem;
        }
        m_rootSplitter = newSplitter;
    }

    addDockAreasToList({newDockArea});
}

void DockContainerWidgetPrivate::dumpRecursive(int level, QWidget *widget) const
{
#if defined(QT_DEBUG)
    QSplitter *splitter = qobject_cast<QSplitter *>(widget);
    QByteArray buf;
    buf.fill(' ', level * 4);
    if (splitter) {
#ifdef ADS_DEBUG_PRINT
        qDebug("%sSplitter %s v: %s c: %s",
               buf.data(),
               (splitter->orientation() == Qt::Vertical) ? "--" : "|",
               splitter->isHidden() ? " " : "v",
               QString::number(splitter->count()).toStdString().c_str());
        std::cout << buf.data() << "Splitter "
                  << ((splitter->orientation() == Qt::Vertical) ? "--" : "|") << " "
                  << (splitter->isHidden() ? " " : "v") << " "
                  << QString::number(splitter->count()).toStdString() << std::endl;
#endif
        for (int i = 0; i < splitter->count(); ++i)
            dumpRecursive(level + 1, splitter->widget(i));
    } else {
        DockAreaWidget *dockArea = qobject_cast<DockAreaWidget *>(widget);
        if (!dockArea)
            return;

#ifdef ADS_DEBUG_PRINT
        qDebug("%sDockArea", buf.data());
        std::cout << buf.data() << (dockArea->isHidden() ? " " : "v")
                  << (dockArea->openDockWidgetsCount() > 0 ? " " : "c") << " DockArea" << std::endl;
        buf.fill(' ', (level + 1) * 4);
        for (int i = 0; i < dockArea->dockWidgetsCount(); ++i) {
            std::cout << buf.data() << (i == dockArea->currentIndex() ? "*" : " ");
            DockWidget *dockWidget = dockArea->dockWidget(i);
            std::cout << (dockWidget->isHidden() ? " " : "v");
            std::cout << (dockWidget->isClosed() ? "c" : " ") << " ";
            std::cout << dockWidget->windowTitle().toStdString() << std::endl;
        }
#endif
    }
#else
    Q_UNUSED(level)
    Q_UNUSED(widget)
#endif
}

DockAreaWidget *DockContainerWidgetPrivate::addDockWidgetToDockArea(DockWidgetArea area,
                                                                    DockWidget *dockWidget,
                                                                    DockAreaWidget *targetDockArea,
                                                                    int index)
{
    if (CenterDockWidgetArea == area) {
        targetDockArea->insertDockWidget(index, dockWidget);
        targetDockArea->updateTitleBarVisibility();
        return targetDockArea;
    }

    DockAreaWidget *newDockArea = new DockAreaWidget(m_dockManager, q);
    newDockArea->addDockWidget(dockWidget);
    auto insertParam = internal::dockAreaInsertParameters(area);

    QSplitter *targetAreaSplitter = internal::findParent<QSplitter *>(targetDockArea);
    int targetIndex = targetAreaSplitter->indexOf(targetDockArea);
    if (targetAreaSplitter->orientation() == insertParam.orientation()) {
        qCInfo(adsLog) << "TargetAreaSplitter->orientation() == InsertParam.orientation()";
        targetAreaSplitter->insertWidget(targetIndex + insertParam.insertOffset(), newDockArea);
        // do nothing, if flag is not enabled
        if (DockManager::testConfigFlag(DockManager::EqualSplitOnInsertion))
            adjustSplitterSizesOnInsertion(targetAreaSplitter);
    } else {
        qCInfo(adsLog) << "TargetAreaSplitter->orientation() != InsertParam.orientation()";
        auto targetAreaSizes = targetAreaSplitter->sizes();
        QSplitter *newSplitter = createSplitter(insertParam.orientation());
        newSplitter->addWidget(targetDockArea);
        insertWidgetIntoSplitter(newSplitter, newDockArea, insertParam.append());
        updateSplitterHandles(newSplitter);
        targetAreaSplitter->insertWidget(targetIndex, newSplitter);
        updateSplitterHandles(targetAreaSplitter);
        if (DockManager::testConfigFlag(DockManager::EqualSplitOnInsertion)) {
            targetAreaSplitter->setSizes(targetAreaSizes);
            adjustSplitterSizesOnInsertion(newSplitter);
        }
    }

    appendDockAreas({newDockArea});
    return newDockArea;
}

DockContainerWidget::DockContainerWidget(DockManager *dockManager, QWidget *parent)
    : QFrame(parent)
    , d(new DockContainerWidgetPrivate(this))
{
    d->m_dockManager = dockManager;
    d->m_isFloating = floatingWidget() != nullptr;

    d->m_layout = new QGridLayout();
    d->m_layout->setContentsMargins(0, 0, 0, 0);
    d->m_layout->setSpacing(0);
    d->m_layout->setColumnStretch(1, 1);
    d->m_layout->setRowStretch(1, 1);
    setLayout(d->m_layout);

    // The function d->createSplitter() accesses the config flags from dock manager which in turn
    // requires a properly constructed dock manager. If this dock container is the dock manager,
    // then it is not properly constructed yet because this base class constructor is called before
    // the constructor of the DockManager private class
    if (dockManager != this) {
        d->m_dockManager->registerDockContainer(this);
        createRootSplitter();
        createSideTabBarWidgets();
    }
}

DockContainerWidget::~DockContainerWidget()
{
    if (d->m_dockManager)
        d->m_dockManager->removeDockContainer(this);

    delete d;
}

DockAreaWidget *DockContainerWidget::addDockWidget(DockWidgetArea area,
                                                   DockWidget *dockWidget,
                                                   DockAreaWidget *dockAreaWidget,
                                                   int index)
{
    auto currentTopLevelDockWIdget = topLevelDockWidget();
    DockAreaWidget *oldDockArea = dockWidget->dockAreaWidget();
    if (oldDockArea)
        oldDockArea->removeDockWidget(dockWidget);

    dockWidget->setDockManager(d->m_dockManager);
    DockAreaWidget *dockArea;
    if (dockAreaWidget)
        dockArea = d->addDockWidgetToDockArea(area, dockWidget, dockAreaWidget, index);
    else
        dockArea = d->addDockWidgetToContainer(area, dockWidget);

    if (currentTopLevelDockWIdget) {
        auto newTopLevelDockWidget = topLevelDockWidget();
        // If the container contained only one visible dock widget, we need to emit a top level
        // event for this widget because it is not the one and only visible docked widget anymore.
        if (!newTopLevelDockWidget)
            DockWidget::emitTopLevelEventForWidget(currentTopLevelDockWIdget, false);
    }

    return dockArea;
}

AutoHideDockContainer *DockContainerWidget::createAndSetupAutoHideContainer(SideBarLocation area,
                                                                            DockWidget *dockWidget,
                                                                            int tabIndex)
{
    if (!DockManager::testAutoHideConfigFlag(DockManager::AutoHideFeatureEnabled)) {
        Q_ASSERT_X(false,
                   "CDockContainerWidget::createAndInitializeDockWidgetOverlayContainer",
                   "Requested area does not exist in config");
        return nullptr;
    }
    if (d->m_dockManager != dockWidget->dockManager()) {
        dockWidget->setDockManager(
            d->m_dockManager); // Auto hide Dock Container needs a valid dock manager
    }

    return autoHideSideBar(area)->insertDockWidget(tabIndex, dockWidget);
}

void DockContainerWidget::removeDockWidget(DockWidget *dockWidget)
{
    DockAreaWidget *area = dockWidget->dockAreaWidget();
    if (area)
        area->removeDockWidget(dockWidget);
}

unsigned int DockContainerWidget::zOrderIndex() const
{
    return d->m_zOrderIndex;
}

bool DockContainerWidget::isInFrontOf(DockContainerWidget *other) const
{
    return this->zOrderIndex() > other->zOrderIndex();
}

bool DockContainerWidget::event(QEvent *event)
{
    bool result = QWidget::event(event);
    if (event->type() == QEvent::WindowActivate)
        d->m_zOrderIndex = ++g_zOrderCounter;
    else if (event->type() == QEvent::Show && !d->m_zOrderIndex)
        d->m_zOrderIndex = ++g_zOrderCounter;

    return result;
}

QList<AutoHideDockContainer *> DockContainerWidget::autoHideWidgets() const
{
    return d->m_autoHideWidgets;
}

void DockContainerWidget::addDockArea(DockAreaWidget *dockAreaWidget, DockWidgetArea area)
{
    DockContainerWidget *container = dockAreaWidget->dockContainer();
    if (container && container != this)
        container->removeDockArea(dockAreaWidget);

    d->addDockArea(dockAreaWidget, area);
}

void DockContainerWidget::removeDockArea(DockAreaWidget *area)
{
    qCInfo(adsLog) << Q_FUNC_INFO;

    // If it is an auto hide area, then there is nothing much to do
    if (area->isAutoHide()) {
        area->setAutoHideDockContainer(nullptr);
        return;
    }

    area->disconnect(this);
    d->m_dockAreas.removeAll(area);
    DockSplitter *splitter = internal::findParent<DockSplitter *>(area);

    // Remove area from parent splitter and recursively hide tree of parent
    // splitters if it has no visible content
    area->setParent(nullptr);
    internal::hideEmptyParentSplitters(splitter);

    // Remove this area from cached areas
    auto p = std::find(std::begin(d->m_lastAddedAreaCache), std::end(d->m_lastAddedAreaCache), area);
    if (p != std::end(d->m_lastAddedAreaCache))
        *p = nullptr;

    // If splitter has more than 1 widget, we are finished and can leave
    if (splitter->count() > 1) {
        updateSplitterHandles(splitter);
        emitAndExit();
        return;
    }

    // If this is the RootSplitter we need to remove empty splitters to
    // avoid too many empty splitters
    if (splitter == d->m_rootSplitter) {
        qCInfo(adsLog) << "Removed from RootSplitter";
        // If splitter is empty, we are finished
        if (!splitter->count()) {
            splitter->hide();
            updateSplitterHandles(splitter);
            emitAndExit();
            return;
        }

        QWidget *widget = splitter->widget(0);
        auto *childSplitter = qobject_cast<DockSplitter *>(widget);
        // If the one and only content widget of the splitter is not a splitter
        // then we are finished
        if (!childSplitter) {
            updateSplitterHandles(splitter);
            emitAndExit();
            return;
        }

        // We replace the superfluous RootSplitter with the ChildSplitter
        childSplitter->setParent(nullptr);
        QLayoutItem *layoutItem = d->m_layout->replaceWidget(splitter, childSplitter);
        d->m_rootSplitter = childSplitter;
        delete layoutItem;
        qCInfo(adsLog) << "RootSplitter replaced by child splitter";
    } else if (splitter->count() == 1) {
        qCInfo(adsLog) << "Replacing splitter with content";
        auto *parentSplitter = internal::findParent<QSplitter *>(splitter);
        auto sizes = parentSplitter->sizes();
        QWidget *widget = splitter->widget(0);
        widget->setParent(this);
        internal::replaceSplitterWidget(parentSplitter, splitter, widget);
        parentSplitter->setSizes(sizes);
    }

    delete splitter;
    splitter = nullptr;

    // TODO Check goto...
    updateSplitterHandles(splitter);
    emitAndExit();
}

void DockContainerWidget::emitAndExit() const
{
    DockWidget *topLevelWidget = topLevelDockWidget();

    // Updated the title bar visibility of the dock widget if there is only
    // one single visible dock widget
    DockWidget::emitTopLevelEventForWidget(topLevelWidget, true);
    dumpLayout();
    d->emitDockAreasRemoved();
}

DockAreaWidget *DockContainerWidget::dockAreaAt(const QPoint &globalPosition) const
{
    for (auto dockArea : std::as_const(d->m_dockAreas)) {
        if (dockArea->isVisible()
            && dockArea->rect().contains(dockArea->mapFromGlobal(globalPosition)))
            return dockArea;
    }

    return nullptr;
}

DockAreaWidget *DockContainerWidget::dockArea(int index) const
{
    return (index < dockAreaCount()) ? d->m_dockAreas[index] : nullptr;
}

bool DockContainerWidget::isFloating() const
{
    return d->m_isFloating;
}

int DockContainerWidget::dockAreaCount() const
{
    return d->m_dockAreas.count();
}

int DockContainerWidget::visibleDockAreaCount() const
{
    int result = 0;
    for (auto dockArea : std::as_const(d->m_dockAreas))
        result += dockArea->isHidden() ? 0 : 1;

    return result;

    // TODO Cache or precalculate this to speed it up because it is used during
    // movement of floating widget
    //return d->visibleDockAreaCount();
}

void DockContainerWidget::dropFloatingWidget(FloatingDockContainer *floatingWidget,
                                             const QPoint &targetPosition)
{
    qCInfo(adsLog) << Q_FUNC_INFO;
    DockWidget *singleDroppedDockWidget = floatingWidget->topLevelDockWidget();
    DockWidget *singleDockWidget = topLevelDockWidget();
    auto dropArea = InvalidDockWidgetArea;
    auto containerDropArea = d->m_dockManager->containerOverlay()->dropAreaUnderCursor();
    bool dropped = false;

    DockAreaWidget *dockArea = dockAreaAt(targetPosition);
    // Mouse is over dock area
    if (dockArea) {
        auto dropOverlay = d->m_dockManager->dockAreaOverlay();
        dropOverlay->setAllowedAreas(dockArea->allowedAreas());
        dropArea = dropOverlay->showOverlay(dockArea);
        if (containerDropArea != InvalidDockWidgetArea && containerDropArea != dropArea)
            dropArea = InvalidDockWidgetArea;

        if (dropArea != InvalidDockWidgetArea) {
            qCInfo(adsLog) << "Dock Area Drop Content: " << dropArea;
            int tabIndex = d->m_dockManager->dockAreaOverlay()->tabIndexUnderCursor();
            d->dropIntoSection(floatingWidget, dockArea, dropArea, tabIndex);
            dropped = true;
        }
    }

    // Mouse is over container or auto hide side bar
    if (InvalidDockWidgetArea == dropArea && InvalidDockWidgetArea != containerDropArea) {
        qCInfo(adsLog) << "Container Drop Content: " << containerDropArea;

        if (internal::isSideBarArea(containerDropArea))
            d->dropIntoAutoHideSideBar(floatingWidget, containerDropArea);
        else
            d->dropIntoContainer(floatingWidget, containerDropArea);

        dropped = true;
    }

    // Remove the auto hide widgets from the FloatingWidget and insert them into this widget
    for (auto autohideWidget : floatingWidget->dockContainer()->autoHideWidgets()) {
        auto sideBar = autoHideSideBar(autohideWidget->sideBarLocation());
        sideBar->addAutoHideWidget(autohideWidget);
    }

    if (dropped) {
        // Fix https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System/issues/351
        floatingWidget->hideAndDeleteLater();

        // If we dropped a floating widget with only one single dock widget, then we
        // drop a top level widget that changes from floating to docked now
        DockWidget::emitTopLevelEventForWidget(singleDroppedDockWidget, false);

        // If there was a top level widget before the drop, then it is not top
        // level widget anymore
        DockWidget::emitTopLevelEventForWidget(singleDockWidget, false);
    }

    window()->activateWindow();
    if (singleDroppedDockWidget)
        d->m_dockManager->notifyWidgetOrAreaRelocation(singleDroppedDockWidget);

    d->m_dockManager->notifyFloatingWidgetDrop(floatingWidget);
}

void DockContainerWidget::dropWidget(QWidget *widget,
                                     DockWidgetArea dropArea,
                                     DockAreaWidget *targetAreaWidget,
                                     int tabIndex)
{
    DockWidget *singleDockWidget = topLevelDockWidget();
    if (targetAreaWidget)
        d->moveToNewSection(widget, targetAreaWidget, dropArea, tabIndex);
    else if (internal::isSideBarArea(dropArea))
        d->moveToAutoHideSideBar(widget, dropArea, tabIndex);
    else
        d->moveToContainer(widget, dropArea);

    // If there was a top level widget before the drop, then it is not top
    // level widget anymore
    DockWidget::emitTopLevelEventForWidget(singleDockWidget, false);

    window()->activateWindow();
    d->m_dockManager->notifyWidgetOrAreaRelocation(widget);
}

QList<DockAreaWidget *> DockContainerWidget::openedDockAreas() const
{
    QList<DockAreaWidget *> result;
    for (auto dockArea : std::as_const(d->m_dockAreas)) {
        if (!dockArea->isHidden())
            result.append(dockArea);
    }

    return result;
}

QList<DockWidget *> DockContainerWidget::openedDockWidgets() const
{
    QList<DockWidget *> dockWidgetList;
    for (auto dockArea : d->m_dockAreas) {
        if (!dockArea->isHidden())
            dockWidgetList.append(dockArea->openedDockWidgets());
    }
    return dockWidgetList;
}

bool DockContainerWidget::hasOpenDockAreas() const
{
    for (auto dockArea : d->m_dockAreas) {
        if (!dockArea->isHidden())
            return true;
    }

    return false;
}

void DockContainerWidget::saveState(QXmlStreamWriter &stream) const
{
    qCInfo(adsLog) << Q_FUNC_INFO << "isFloating " << isFloating();

    stream.writeStartElement("container");
    stream.writeAttribute("floating", QVariant::fromValue(isFloating()).toString());
    if (isFloating()) {
        FloatingDockContainer *floatingDockContainer = floatingWidget();
        QByteArray geometry = floatingDockContainer->saveGeometry();
        stream.writeTextElement("geometry", QString::fromLatin1(geometry.toBase64()));
    }
    d->saveChildNodesState(stream, d->m_rootSplitter);
    d->saveAutoHideWidgetsState(stream);
    stream.writeEndElement();
}

bool DockContainerWidget::restoreState(DockingStateReader &stateReader, bool testing)
{
    QVariant floatingVar = QVariant(stateReader.attributes().value("floating").toString());
    if (!floatingVar.canConvert<bool>())
        return false;

    bool isFloating = floatingVar.value<bool>();
    qCInfo(adsLog) << "Restore DockContainerWidget Floating" << isFloating;

    QWidget *newRootSplitter{};
    if (!testing) {
        d->m_visibleDockAreaCount = -1; // invalidate the dock area count
        d->m_dockAreas.clear();
        std::fill(std::begin(d->m_lastAddedAreaCache), std::end(d->m_lastAddedAreaCache), nullptr);
    }

    if (isFloating) {
        qCInfo(adsLog) << "Restore floating widget";
        if (!stateReader.readNextStartElement() || stateReader.name() != QLatin1String("geometry"))
            return false;

        QByteArray geometryString
            = stateReader.readElementText(DockingStateReader::ErrorOnUnexpectedElement).toLocal8Bit();
        QByteArray geometry = QByteArray::fromBase64(geometryString);
        if (geometry.isEmpty())
            return false;

        if (!testing) {
            FloatingDockContainer *floatingDockContainer = floatingWidget();
            if (floatingDockContainer)
                floatingDockContainer->restoreGeometry(geometry);
        }
    }

    if (!d->restoreChildNodes(stateReader, newRootSplitter, testing))
        return false;

    if (testing)
        return true;

    // If the root splitter is empty, restoreChildNodes returns a nullptr
    // and we need to create a new empty root splitter
    if (!newRootSplitter)
        newRootSplitter = d->createSplitter(Qt::Horizontal);

    d->m_layout->replaceWidget(d->m_rootSplitter, newRootSplitter);
    QSplitter *oldRoot = d->m_rootSplitter;
    d->m_rootSplitter = qobject_cast<DockSplitter *>(newRootSplitter);
    oldRoot->deleteLater();

    return true;
}

QSplitter *DockContainerWidget::rootSplitter() const
{
    return d->m_rootSplitter;
}

void DockContainerWidget::createRootSplitter()
{
    if (d->m_rootSplitter)
        return;

    d->m_rootSplitter = d->createSplitter(Qt::Horizontal);
    // Add it to the center - the 0 and 2 indices are used for the SideTabBar widgets
    d->m_layout->addWidget(d->m_rootSplitter, 1, 1);
}

void DockContainerWidget::createSideTabBarWidgets()
{
    if (!DockManager::testAutoHideConfigFlag(DockManager::AutoHideFeatureEnabled))
        return;

    {
        auto area = SideBarLocation::SideBarLeft;
        d->m_sideTabBarWidgets[area] = new AutoHideSideBar(this, area);
        d->m_layout->addWidget(d->m_sideTabBarWidgets[area], 1, 0);
    }

    {
        auto area = SideBarLocation::SideBarRight;
        d->m_sideTabBarWidgets[area] = new AutoHideSideBar(this, area);
        d->m_layout->addWidget(d->m_sideTabBarWidgets[area], 1, 2);
    }

    {
        auto area = SideBarLocation::SideBarBottom;
        d->m_sideTabBarWidgets[area] = new AutoHideSideBar(this, area);
        d->m_layout->addWidget(d->m_sideTabBarWidgets[area], 2, 1);
    }

    {
        auto area = SideBarLocation::SideBarTop;
        d->m_sideTabBarWidgets[area] = new AutoHideSideBar(this, area);
        d->m_layout->addWidget(d->m_sideTabBarWidgets[area], 0, 1);
    }
}

void DockContainerWidget::dumpLayout() const
{
#if (ADS_DEBUG_LEVEL > 0)
    qDebug("\n\nDumping layout --------------------------");
    std::cout << "\n\nDumping layout --------------------------" << std::endl;
    d->dumpRecursive(0, d->m_rootSplitter);
    qDebug("--------------------------\n\n");
    std::cout << "--------------------------\n\n" << std::endl;
#endif
}

DockAreaWidget *DockContainerWidget::lastAddedDockAreaWidget(DockWidgetArea area) const
{
    return d->m_lastAddedAreaCache[areaIdToIndex(area)];
}

bool DockContainerWidget::hasTopLevelDockWidget() const
{
    auto dockAreas = openedDockAreas();
    if (dockAreas.count() != 1)
        return false;

    return dockAreas[0]->openDockWidgetsCount() == 1;
}

DockWidget *DockContainerWidget::topLevelDockWidget() const
{
    auto dockArea = topLevelDockArea();
    if (!dockArea)
        return nullptr;

    auto dockWidgets = dockArea->openedDockWidgets();
    if (dockWidgets.count() != 1)
        return nullptr;

    return dockWidgets[0];
}

DockAreaWidget *DockContainerWidget::topLevelDockArea() const
{
    auto dockAreas = openedDockAreas();
    if (dockAreas.count() != 1)
        return nullptr;

    return dockAreas[0];
}

QList<DockWidget *> DockContainerWidget::dockWidgets() const
{
    QList<DockWidget *> result;
    for (const auto dockArea : std::as_const(d->m_dockAreas))
        result.append(dockArea->dockWidgets());

    return result;
}

void DockContainerWidget::updateSplitterHandles(QSplitter *splitter)
{
    d->updateSplitterHandles(splitter);
}

void DockContainerWidget::registerAutoHideWidget(AutoHideDockContainer *autohideWidget)
{
    d->m_autoHideWidgets.append(autohideWidget);
    Q_EMIT autoHideWidgetCreated(autohideWidget);
    qCInfo(adsLog) << "d->AutoHideWidgets.count() " << d->m_autoHideWidgets.count();
}

void DockContainerWidget::removeAutoHideWidget(AutoHideDockContainer *autohideWidget)
{
    d->m_autoHideWidgets.removeAll(autohideWidget);
}

DockWidget::DockWidgetFeatures DockContainerWidget::features() const
{
    DockWidget::DockWidgetFeatures features(DockWidget::AllDockWidgetFeatures);
    for (const auto dockArea : std::as_const(d->m_dockAreas))
        features &= dockArea->features();

    return features;
}

FloatingDockContainer *DockContainerWidget::floatingWidget() const
{
    return internal::findParent<FloatingDockContainer *>(this);
}

void DockContainerWidget::closeOtherAreas(DockAreaWidget *keepOpenArea)
{
    for (const auto dockArea : std::as_const(d->m_dockAreas)) {
        if (dockArea == keepOpenArea)
            continue;

        if (!dockArea->features(BitwiseAnd).testFlag(DockWidget::DockWidgetClosable))
            continue;

        // We do not close areas with widgets with custom close handling
        if (dockArea->features(BitwiseOr).testFlag(DockWidget::CustomCloseHandling))
            continue;

        dockArea->closeArea();
    }
}

AutoHideSideBar *DockContainerWidget::autoHideSideBar(SideBarLocation area) const
{
    return d->m_sideTabBarWidgets[area];
}

QRect DockContainerWidget::contentRect() const
{
    if (!d->m_rootSplitter)
        return QRect();

    return d->m_rootSplitter->geometry();
}

QRect DockContainerWidget::contentRectGlobal() const
{
    if (!d->m_rootSplitter)
        return QRect();

    if (d->m_rootSplitter->hasVisibleContent()) {
        return d->m_rootSplitter->geometry();
    } else {
        auto contentRect = rect();
        contentRect.adjust(autoHideSideBar(SideBarLeft)->sizeHint().width(),
                           autoHideSideBar(SideBarTop)->sizeHint().height(),
                           -autoHideSideBar(SideBarRight)->sizeHint().width(),
                           -autoHideSideBar(SideBarBottom)->sizeHint().height());

        return contentRect;
    }
}

DockManager *DockContainerWidget::dockManager() const
{
    return d->m_dockManager;
}

void DockContainerWidget::handleAutoHideWidgetEvent(QEvent *e, QWidget *w)
{
    if (!DockManager::testAutoHideConfigFlag(DockManager::AutoHideShowOnMouseOver))
        return;

    if (dockManager()->isRestoringState())
        return;

    auto autoHideTab = qobject_cast<AutoHideTab *>(w);
    if (autoHideTab) {
        switch (e->type()) {
        case QEvent::Enter:
            if (!autoHideTab->dockWidget()->isVisible()) {
                d->m_delayedAutoHideTab = autoHideTab;
                d->m_delayedAutoHideShow = true;
                d->m_delayedAutoHideTimer.start();
            } else {
                d->m_delayedAutoHideTimer.stop();
            }
            break;

        case QEvent::MouseButtonPress:
            d->m_delayedAutoHideTimer.stop();
            break;

        case QEvent::Leave:
            if (autoHideTab->dockWidget()->isVisible()) {
                d->m_delayedAutoHideTab = autoHideTab;
                d->m_delayedAutoHideShow = false;
                d->m_delayedAutoHideTimer.start();
            } else {
                d->m_delayedAutoHideTimer.stop();
            }
            break;

        default:
            break;
        }
        return;
    }

    auto autoHideContainer = qobject_cast<AutoHideDockContainer *>(w);
    if (autoHideContainer) {
        switch (e->type()) {
        case QEvent::Enter:
        case QEvent::Hide:
            d->m_delayedAutoHideTimer.stop();
            break;

        case QEvent::Leave:
            if (autoHideContainer->isVisible()) {
                d->m_delayedAutoHideTab = autoHideContainer->autoHideTab();
                d->m_delayedAutoHideShow = false;
                d->m_delayedAutoHideTimer.start();
            }
            break;

        default:
            break;
        }
        return;
    }
}

} // namespace ADS
