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

#include "dockcontainerwidget.h"

#include "ads_globals.h"
#include "dockareawidget.h"
#include "dockingstatereader.h"
#include "dockmanager.h"
#include "dockoverlay.h"
#include "docksplitter.h"
#include "dockwidget.h"
#include "floatingdockcontainer.h"

#include <QAbstractButton>
#include <QDebug>
#include <QEvent>
#include <QGridLayout>
#include <QList>
#include <QLoggingCategory>
#include <QPointer>
#include <QVariant>
#include <QXmlStreamWriter>

#include <functional>
#include <iostream>

static Q_LOGGING_CATEGORY(adsLog, "qtc.qmldesigner.advanceddockingsystem", QtWarningMsg)

namespace ADS
{
    static unsigned int zOrderCounter = 0;

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
        QGridLayout *m_layout = nullptr;
        QSplitter *m_rootSplitter = nullptr;
        bool m_isFloating = false;
        DockAreaWidget *m_lastAddedAreaCache[5];
        int m_visibleDockAreaCount = -1;
        DockAreaWidget *m_topLevelDockArea = nullptr;

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
                                                DockAreaWidget *targetDockArea);

        /**
         * Add dock area to this container
         */
        void addDockArea(DockAreaWidget *newDockWidget, DockWidgetArea area = CenterDockWidgetArea);

        /**
         * Drop floating widget into container
         */
        void dropIntoContainer(FloatingDockContainer *floatingWidget, DockWidgetArea area);

        /**
         * Drop floating widget into dock area
         */
        void dropIntoSection(FloatingDockContainer *floatingWidget,
                             DockAreaWidget *targetArea,
                             DockWidgetArea area);

        /**
         * Moves the dock widget or dock area given in Widget parameter to a
         * new dock widget area
         */
        void moveToNewSection(QWidget *widget, DockAreaWidget *targetArea, DockWidgetArea area);

        /**
         * Moves the dock widget or dock area given in Widget parameter to a
         * a dock area in container
         */
        void moveToContainer(QWidget *widget, DockWidgetArea area);

        /**
         * Creates a new tab for a widget dropped into the center of a section
         */
        void dropIntoCenterOfSection(FloatingDockContainer *floatingWidget,
                                     DockAreaWidget *targetArea);

        /**
         * Creates a new tab for a widget dropped into the center of a section
         */
        void moveIntoCenterOfSection(QWidget *widget, DockAreaWidget *targetArea);

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
         * Restore state of child nodes.
         * \param[in] Stream The data stream that contains the serialized state
         * \param[out] CreatedWidget The widget created from parsed data or 0 if
         * the parsed widget was an empty splitter
         * \param[in] Testing If Testing is true, only the stream data is
         * parsed without modifiying anything.
         */
        bool restoreChildNodes(DockingStateReader &stateReader,
                               QWidget *&createdWidget,
                               bool testing);

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
            for (auto dockArea : m_dockAreas)
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

        void onDockAreaViewToggled(bool visible)
        {
            DockAreaWidget *dockArea = qobject_cast<DockAreaWidget *>(q->sender());
            m_visibleDockAreaCount += visible ? 1 : -1;
            onVisibleDockAreaCountChanged();
            emit q->dockAreaViewToggled(dockArea, visible);
        }
    }; // struct DockContainerWidgetPrivate

    DockContainerWidgetPrivate::DockContainerWidgetPrivate(DockContainerWidget *parent)
        : q(parent)
    {
        std::fill(std::begin(m_lastAddedAreaCache), std::end(m_lastAddedAreaCache), nullptr);
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
            topLevelDockArea->titleBarButton(TitleBarButtonUndock)
                ->setVisible(false || !q->isFloating());
            topLevelDockArea->titleBarButton(TitleBarButtonClose)
                ->setVisible(false || !q->isFloating());
        } else if (this->m_topLevelDockArea) {
            this->m_topLevelDockArea->titleBarButton(TitleBarButtonUndock)->setVisible(true);
            this->m_topLevelDockArea->titleBarButton(TitleBarButtonClose)->setVisible(true);
            this->m_topLevelDockArea = nullptr;
        }
    }

    void DockContainerWidgetPrivate::dropIntoContainer(FloatingDockContainer *floatingWidget,
                                                       DockWidgetArea area)
    {
        auto insertParam = internal::dockAreaInsertParameters(area);
        DockContainerWidget *floatingDockContainer = floatingWidget->dockContainer();
        auto newDockAreas = floatingDockContainer
                                ->findChildren<DockAreaWidget *>(QString(),
                                                                 Qt::FindChildrenRecursively);
        QSplitter *splitter = m_rootSplitter;

        if (m_dockAreas.count() <= 1) {
            splitter->setOrientation(insertParam.orientation());
        } else if (splitter->orientation() != insertParam.orientation()) {
            QSplitter *newSplitter = createSplitter(insertParam.orientation());
            QLayoutItem *layoutItem = m_layout->replaceWidget(splitter, newSplitter);
            newSplitter->addWidget(splitter);
            splitter = newSplitter;
            delete layoutItem;
        }

        // Now we can insert the floating widget content into this container
        auto floatingSplitter = floatingDockContainer->rootSplitter();
        if (floatingSplitter->count() == 1) {
            insertWidgetIntoSplitter(splitter, floatingSplitter->widget(0), insertParam.append());
        } else if (floatingSplitter->orientation() == insertParam.orientation()) {
            int insertIndex = insertParam.append() ? splitter->count() : 0;
            while (floatingSplitter->count())
                splitter->insertWidget(insertIndex++, floatingSplitter->widget(0));
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

    void DockContainerWidgetPrivate::dropIntoCenterOfSection(FloatingDockContainer *floatingWidget,
                                                             DockAreaWidget *targetArea)
    {
        DockContainerWidget *floatingContainer = floatingWidget->dockContainer();
        auto newDockWidgets = floatingContainer->dockWidgets();
        auto topLevelDockArea = floatingContainer->topLevelDockArea();
        int newCurrentIndex = -1;

        // If the floating widget contains only one single dock are, then the
        // current dock widget of the dock area will also be the future current
        // dock widget in the drop area.
        if (topLevelDockArea)
            newCurrentIndex = topLevelDockArea->currentIndex();

        for (int i = 0; i < newDockWidgets.count(); ++i) {
            DockWidget *dockWidget = newDockWidgets[i];
            targetArea->insertDockWidget(i, dockWidget, false);
            // If the floating widget contains multiple visible dock areas, then we
            // simply pick the first visible open dock widget and make it
            // the current one.
            if (newCurrentIndex < 0 && !dockWidget->isClosed())
                newCurrentIndex = i;
        }
        targetArea->setCurrentIndex(newCurrentIndex);
        targetArea->updateTitleBarVisibility();
        return;
    }

    void DockContainerWidgetPrivate::dropIntoSection(FloatingDockContainer *floatingWidget,
                                                     DockAreaWidget *targetArea,
                                                     DockWidgetArea area)
    {
        // Dropping into center means all dock widgets in the dropped floating
        // widget will become tabs of the drop area
        if (CenterDockWidgetArea == area) {
            dropIntoCenterOfSection(floatingWidget, targetArea);
            return;
        }

        auto insertParam = internal::dockAreaInsertParameters(area);
        auto newDockAreas = floatingWidget->dockContainer()
                                ->findChildren<DockAreaWidget *>(QString(),
                                                                 Qt::FindChildrenRecursively);
        QSplitter *targetAreaSplitter = internal::findParent<QSplitter *>(targetArea);

        if (!targetAreaSplitter) {
            QSplitter *splitter = createSplitter(insertParam.orientation());
            m_layout->replaceWidget(targetArea, splitter);
            splitter->addWidget(targetArea);
            targetAreaSplitter = splitter;
        }
        int areaIndex = targetAreaSplitter->indexOf(targetArea);
        auto widget = floatingWidget->dockContainer()
                          ->findChild<QWidget *>(QString(), Qt::FindDirectChildrenOnly);
        auto floatingSplitter = qobject_cast<QSplitter *>(widget);

        if (targetAreaSplitter->orientation() == insertParam.orientation()) {
            auto sizes = targetAreaSplitter->sizes();
            int targetAreaSize = (insertParam.orientation() == Qt::Horizontal)
                                     ? targetArea->width()
                                     : targetArea->height();
            bool adjustSplitterSizes = true;
            if ((floatingSplitter->orientation() != insertParam.orientation())
                && floatingSplitter->count() > 1) {
                targetAreaSplitter->insertWidget(areaIndex + insertParam.insertOffset(), widget);
            } else {
                adjustSplitterSizes = (floatingSplitter->count() == 1);
                int insertIndex = areaIndex + insertParam.insertOffset();
                while (floatingSplitter->count())
                    targetAreaSplitter->insertWidget(insertIndex++, floatingSplitter->widget(0));
            }

            if (adjustSplitterSizes) {
                int size = (targetAreaSize - targetAreaSplitter->handleWidth()) / 2;
                sizes[areaIndex] = size;
                sizes.insert(areaIndex, size);
                targetAreaSplitter->setSizes(sizes);
            }
        } else {
            QList<int> newSplitterSizes;
            QSplitter *newSplitter = createSplitter(insertParam.orientation());
            int targetAreaSize = (insertParam.orientation() == Qt::Horizontal)
                                     ? targetArea->width()
                                     : targetArea->height();
            bool adjustSplitterSizes = true;
            if ((floatingSplitter->orientation() != insertParam.orientation())
                && floatingSplitter->count() > 1) {
                newSplitter->addWidget(widget);
            } else {
                adjustSplitterSizes = (floatingSplitter->count() == 1);
                while (floatingSplitter->count()) {
                    newSplitter->addWidget(floatingSplitter->widget(0));
                }
            }

            // Save the sizes before insertion and restore it later to prevent
            // shrinking of existing area
            auto sizes = targetAreaSplitter->sizes();
            insertWidgetIntoSplitter(newSplitter, targetArea, !insertParam.append());
            if (adjustSplitterSizes) {
                int size = targetAreaSize / 2;
                newSplitter->setSizes({size, size});
            }
            targetAreaSplitter->insertWidget(areaIndex, newSplitter);
            targetAreaSplitter->setSizes(sizes);
        }

        addDockAreasToList(newDockAreas);
        q->dumpLayout();
    }

    void DockContainerWidgetPrivate::moveIntoCenterOfSection(QWidget *widget,
                                                             DockAreaWidget *targetArea)
    {
        auto droppedDockWidget = qobject_cast<DockWidget *>(widget);
        auto droppedArea = qobject_cast<DockAreaWidget *>(widget);

        if (droppedDockWidget) {
            DockAreaWidget *oldDockArea = droppedDockWidget->dockAreaWidget();
            if (oldDockArea == targetArea)
                return;

            if (oldDockArea)
                oldDockArea->removeDockWidget(droppedDockWidget);

            targetArea->insertDockWidget(0, droppedDockWidget, true);
        } else {
            QList<DockWidget *> newDockWidgets = droppedArea->dockWidgets();
            int newCurrentIndex = droppedArea->currentIndex();
            for (int i = 0; i < newDockWidgets.count(); ++i) {
                DockWidget *dockWidget = newDockWidgets[i];
                targetArea->insertDockWidget(i, dockWidget, false);
            }
            targetArea->setCurrentIndex(newCurrentIndex);
            droppedArea->dockContainer()->removeDockArea(droppedArea);
            droppedArea->deleteLater();
        }

        targetArea->updateTitleBarVisibility();
        return;
    }

    void DockContainerWidgetPrivate::moveToNewSection(QWidget *widget,
                                                      DockAreaWidget *targetArea,
                                                      DockWidgetArea area)
    {
        // Dropping into center means all dock widgets in the dropped floating
        // widget will become tabs of the drop area
        if (CenterDockWidgetArea == area) {
            moveIntoCenterOfSection(widget, targetArea);
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
            const int size = (targetAreaSize - targetAreaSplitter->handleWidth()) / 2;
            sizes[areaIndex] = size;
            sizes.insert(areaIndex, size);
        } else {
            auto sizes = targetAreaSplitter->sizes();
            const int targetAreaSize = (insertParam.orientation() == Qt::Horizontal)
                                           ? targetArea->width()
                                           : targetArea->height();
            QSplitter *newSplitter = createSplitter(insertParam.orientation());
            newSplitter->addWidget(targetArea);
            insertWidgetIntoSplitter(newSplitter, newDockArea, insertParam.append());
            const int size = targetAreaSize / 2;
            newSplitter->setSizes({size, size});
            targetAreaSplitter->insertWidget(areaIndex, newSplitter);
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
            auto splitter = internal::findParent<DockSplitter*>(droppedDockArea);
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
            qCInfo(adsLog) << "NodeSplitter orient: " << splitter->orientation()
                           << " WidgetCont: " << splitter->count();
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

        qCInfo(adsLog) << "Restore NodeSplitter Orientation: " << orientation
                       << " WidgetCount: " << widgetCount;
        QSplitter *splitter = nullptr;
        if (!testing)
            splitter = createSplitter(orientation);

        bool visible = false;
        QList<int> sizes;
        while (stateReader.readNextStartElement()) {
            QWidget *childNode = nullptr;
            bool result = true;
            if (stateReader.name() == "splitter") {
                result = restoreSplitter(stateReader, childNode, testing);
            } else if (stateReader.name() == "area") {
                result = restoreDockArea(stateReader, childNode, testing);
            } else if (stateReader.name() == "sizes") {
                QString size = stateReader.readElementText().trimmed();
                qCInfo(adsLog) << "Size: " << size;
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

            qCInfo(adsLog) << "ChildNode isVisible " << childNode->isVisible() << " isVisibleTo "
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
        QString currentDockWidget = stateReader.attributes().value("current").toString();

#ifdef ADS_DEBUG_PRINT
        bool ok;
        int tabs = stateReader.attributes().value("tabs").toInt(&ok);
        if (!ok)
            return false;

        qCInfo(adsLog) << "Restore NodeDockArea Tabs: " << tabs
                       << " Current: " << currentDockWidget;
#endif

        DockAreaWidget *dockArea = nullptr;
        if (!testing)
            dockArea = new DockAreaWidget(m_dockManager, q);

        while (stateReader.readNextStartElement()) {
            if (stateReader.name() != "widget")
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
            DockWidget *dockWidget = m_dockManager->findDockWidget(objectName.toString());
            if (!dockWidget || testing)
                continue;

            qCInfo(adsLog) << "Dock Widget found - parent " << dockWidget->parent();
            // We hide the DockArea here to prevent the short display (the flashing)
            // of the dock areas during application startup
            dockArea->hide();
            dockArea->addDockWidget(dockWidget);
            dockWidget->setToggleViewActionChecked(!closed);
            dockWidget->setClosedState(closed);
            dockWidget->setProperty(internal::closedProperty, closed);
            dockWidget->setProperty(internal::dirtyProperty, false);
        }

        if (testing)
            return true;

        if (!dockArea->dockWidgetsCount()) {
            delete dockArea;
            dockArea = nullptr;
        } else {
            dockArea->setProperty("currentDockWidget", currentDockWidget);
            appendDockAreas({dockArea});
        }

        createdWidget = dockArea;
        return true;
    }

    bool DockContainerWidgetPrivate::restoreChildNodes(DockingStateReader &stateReader,
                                                       QWidget *&createdWidget,
                                                       bool testing)
    {
        bool result = true;
        while (stateReader.readNextStartElement()) {
            if (stateReader.name() == "splitter") {
                result = restoreSplitter(stateReader, createdWidget, testing);
                qCInfo(adsLog) << "Splitter";
            } else if (stateReader.name() == "area") {
                result = restoreDockArea(stateReader, createdWidget, testing);
                qCInfo(adsLog) << "DockAreaWidget";
            } else {
                stateReader.skipCurrentElement();
                qCInfo(adsLog) << "Unknown element" << stateReader.name();
            }
        }

        return result;
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

        QSplitter *splitter = m_rootSplitter;
        if (splitter->orientation() == insertParam.orientation()) {
            insertWidgetIntoSplitter(splitter, newDockArea, insertParam.append());
            if (splitter->isHidden())
                splitter->show();

        } else {
            QSplitter *newSplitter = createSplitter(insertParam.orientation());
            if (insertParam.append()) {
                QLayoutItem *layoutItem = m_layout->replaceWidget(splitter, newSplitter);
                newSplitter->addWidget(splitter);
                newSplitter->addWidget(newDockArea);
                delete layoutItem;
            } else {
                newSplitter->addWidget(newDockArea);
                QLayoutItem *layoutItem = m_layout->replaceWidget(splitter, newSplitter);
                newSplitter->addWidget(splitter);
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
                      << (dockArea->openDockWidgetsCount() > 0 ? " " : "c") << " DockArea"
                      << std::endl;
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
                                                                        DockAreaWidget *targetDockArea)
    {
        if (CenterDockWidgetArea == area) {
            targetDockArea->addDockWidget(dockWidget);
            targetDockArea->updateTitleBarVisibility();
            return targetDockArea;
        }

        DockAreaWidget *newDockArea = new DockAreaWidget(m_dockManager, q);
        newDockArea->addDockWidget(dockWidget);
        auto insertParam = internal::dockAreaInsertParameters(area);

        QSplitter *targetAreaSplitter = internal::findParent<QSplitter *>(targetDockArea);
        int index = targetAreaSplitter->indexOf(targetDockArea);
        if (targetAreaSplitter->orientation() == insertParam.orientation()) {
            qCInfo(adsLog) << "TargetAreaSplitter->orientation() == InsertParam.orientation()";
            targetAreaSplitter->insertWidget(index + insertParam.insertOffset(), newDockArea);
            // do nothing, if flag is not enabled
            if (DockManager::testConfigFlag(DockManager::EqualSplitOnInsertion))
                adjustSplitterSizesOnInsertion(targetAreaSplitter);
        } else {
            qCInfo(adsLog) << "TargetAreaSplitter->orientation() != InsertParam.orientation()";
            auto targetAreaSizes = targetAreaSplitter->sizes();
            QSplitter *newSplitter = createSplitter(insertParam.orientation());
            newSplitter->addWidget(targetDockArea);
            insertWidgetIntoSplitter(newSplitter, newDockArea, insertParam.append());
            targetAreaSplitter->insertWidget(index, newSplitter);
            if (DockManager::testConfigFlag(DockManager::EqualSplitOnInsertion)) {
                targetAreaSplitter->setSizes(targetAreaSizes);
                adjustSplitterSizesOnInsertion(newSplitter);
            }
        }

        appendDockAreas({newDockArea});
        emitDockAreasAdded();
        return newDockArea;
    }

    DockContainerWidget::DockContainerWidget(DockManager *dockManager, QWidget *parent)
        : QFrame(parent)
        , d(new DockContainerWidgetPrivate(this))
    {
        d->m_dockManager = dockManager;
        d->m_isFloating = floatingWidget() != nullptr;

        d->m_layout = new QGridLayout();
        d->m_layout->setContentsMargins(0, 1, 0, 1);
        d->m_layout->setSpacing(0);
        setLayout(d->m_layout);

        // The function d->createSplitter() accesses the config flags from dock
        // manager which in turn requires a properly constructed dock manager.
        // If this dock container is the dock manager, then it is not properly
        // constructed yet because this base class constructor is called before
        // the constructor of the DockManager private class
        if (dockManager != this) {
            d->m_dockManager->registerDockContainer(this);
            createRootSplitter();
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
                                                       DockAreaWidget *dockAreaWidget)
    {
        DockAreaWidget *oldDockArea = dockWidget->dockAreaWidget();
        if (oldDockArea)
            oldDockArea->removeDockWidget(dockWidget);

        dockWidget->setDockManager(d->m_dockManager);
        if (dockAreaWidget)
            return d->addDockWidgetToDockArea(area, dockWidget, dockAreaWidget);
        else
            return d->addDockWidgetToContainer(area, dockWidget);
    }

    void DockContainerWidget::removeDockWidget(DockWidget * dockWidget)
    {
        DockAreaWidget *area = dockWidget->dockAreaWidget();
        if (area)
            area->removeDockWidget(dockWidget);
    }

    unsigned int DockContainerWidget::zOrderIndex() const { return d->m_zOrderIndex; }

    bool DockContainerWidget::isInFrontOf(DockContainerWidget *other) const
    {
        return this->zOrderIndex() > other->zOrderIndex();
    }

    bool DockContainerWidget::event(QEvent *event)
    {
        bool result = QWidget::event(event);
        if (event->type() == QEvent::WindowActivate)
            d->m_zOrderIndex = ++zOrderCounter;
        else if (event->type() == QEvent::Show && !d->m_zOrderIndex)
            d->m_zOrderIndex = ++zOrderCounter;

        return result;
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
        area->disconnect(this);
        d->m_dockAreas.removeAll(area);
        DockSplitter *splitter = internal::findParent<DockSplitter *>(area);

        // Remove area from parent splitter and recursively hide tree of parent
        // splitters if it has no visible content
        area->setParent(nullptr);
        internal::hideEmptyParentSplitters(splitter);

        // Remove this area from cached areas
        const auto &cache = d->m_lastAddedAreaCache;
        if (auto p = std::find(cache, cache + sizeof(cache) / sizeof(cache[0]), area))
            d->m_lastAddedAreaCache[std::distance(cache, p)] = nullptr;

        // If splitter has more than 1 widgets, we are finished and can leave
        if (splitter->count() > 1) {
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
                emitAndExit();
                return;
            }

            QWidget *widget = splitter->widget(0);
            QSplitter *childSplitter = qobject_cast<QSplitter *>(widget);
            // If the one and only content widget of the splitter is not a splitter
            // then we are finished
            if (!childSplitter) {
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
            QSplitter *parentSplitter = internal::findParent<QSplitter *>(splitter);
            auto sizes = parentSplitter->sizes();
            QWidget *widget = splitter->widget(0);
            widget->setParent(this);
            internal::replaceSplitterWidget(parentSplitter, splitter, widget);
            parentSplitter->setSizes(sizes);
        }

        delete splitter;
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
        for (auto dockArea : d->m_dockAreas) {
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

    bool DockContainerWidget::isFloating() const { return d->m_isFloating; }

    int DockContainerWidget::dockAreaCount() const { return d->m_dockAreas.count(); }

    int DockContainerWidget::visibleDockAreaCount() const
    {
        int result = 0;
        for (auto dockArea : d->m_dockAreas)
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
        DockAreaWidget *dockArea = dockAreaAt(targetPosition);
        auto dropArea = InvalidDockWidgetArea;
        auto containerDropArea = d->m_dockManager->containerOverlay()->dropAreaUnderCursor();
        bool dropped = false;

        if (dockArea) {
            auto dropOverlay = d->m_dockManager->dockAreaOverlay();
            dropOverlay->setAllowedAreas(dockArea->allowedAreas());
            dropArea = dropOverlay->showOverlay(dockArea);
            if (containerDropArea != InvalidDockWidgetArea && containerDropArea != dropArea)
                dropArea = InvalidDockWidgetArea;

            if (dropArea != InvalidDockWidgetArea) {
                qCInfo(adsLog) << "Dock Area Drop Content: " << dropArea;
                d->dropIntoSection(floatingWidget, dockArea, dropArea);
                dropped = true;
            }
        }

        // mouse is over container
        if (InvalidDockWidgetArea == dropArea) {
            dropArea = containerDropArea;
            qCInfo(adsLog) << "Container Drop Content: " << dropArea;
            if (dropArea != InvalidDockWidgetArea) {
                d->dropIntoContainer(floatingWidget, dropArea);
                dropped = true;
            }
        }

        if (dropped) {
            floatingWidget->deleteLater();

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

    void DockContainerWidget::dropWidget(QWidget *widget, DockWidgetArea dropArea, DockAreaWidget *targetAreaWidget)
    {
        DockWidget *singleDockWidget = topLevelDockWidget();
        if (targetAreaWidget)
            d->moveToNewSection(widget, targetAreaWidget, dropArea);
        else
            d->moveToContainer(widget, dropArea);

        // If there was a top level widget before the drop, then it is not top
        // level widget anymore
        DockWidget::emitTopLevelEventForWidget(singleDockWidget, false);
        DockWidget *dockWidget = qobject_cast<DockWidget *>(widget);
        if (!dockWidget)
        {
            DockAreaWidget *dockArea = qobject_cast<DockAreaWidget *>(widget);
            auto openDockWidgets = dockArea->openedDockWidgets();
            if (openDockWidgets.count() == 1)
                dockWidget = openDockWidgets[0];
        }

        window()->activateWindow();
        d->m_dockManager->notifyWidgetOrAreaRelocation(widget);
    }

    QList<DockAreaWidget *> DockContainerWidget::openedDockAreas() const
    {
        QList<DockAreaWidget *> result;
        for (auto dockArea : d->m_dockAreas) {
            if (!dockArea->isHidden())
                result.append(dockArea);
        }

        return result;
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
            std::fill(std::begin(d->m_lastAddedAreaCache),
                      std::end(d->m_lastAddedAreaCache),
                      nullptr);
        }

        if (isFloating) {
            qCInfo(adsLog) << "Restore floating widget";
            if (!stateReader.readNextStartElement() || stateReader.name() != "geometry")
                return false;

            QByteArray geometryString = stateReader
                                            .readElementText(
                                                DockingStateReader::ErrorOnUnexpectedElement)
                                            .toLocal8Bit();
            QByteArray geometry = QByteArray::fromBase64(geometryString);
            if (geometry.isEmpty())
                return false;

            if (!testing) {
                FloatingDockContainer *floatingDockContainer = floatingWidget();
                floatingDockContainer->restoreGeometry(geometry);
            }
        }

        if (!d->restoreChildNodes(stateReader, newRootSplitter, testing))
            return false;

        if (testing)
            return true;

        // If the root splitter is empty, rostoreChildNodes returns a 0 pointer
        // and we need to create a new empty root splitter
        if (!newRootSplitter)
            newRootSplitter = d->createSplitter(Qt::Horizontal);

        d->m_layout->replaceWidget(d->m_rootSplitter, newRootSplitter);
        QSplitter *oldRoot = d->m_rootSplitter;
        d->m_rootSplitter = qobject_cast<QSplitter *>(newRootSplitter);
        oldRoot->deleteLater();

        return true;
    }

    QSplitter *DockContainerWidget::rootSplitter() const { return d->m_rootSplitter; }

    void DockContainerWidget::createRootSplitter()
    {
        if (d->m_rootSplitter)
            return;

        d->m_rootSplitter = d->createSplitter(Qt::Horizontal);
        d->m_layout->addWidget(d->m_rootSplitter);
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
        for (const auto dockArea : d->m_dockAreas)
            result.append(dockArea->dockWidgets());

        return result;
    }

    DockWidget::DockWidgetFeatures DockContainerWidget::features() const
    {
        DockWidget::DockWidgetFeatures features(DockWidget::AllDockWidgetFeatures);
        for (const auto dockArea : d->m_dockAreas)
            features &= dockArea->features();

        return features;
    }

    FloatingDockContainer *DockContainerWidget::floatingWidget() const
    {
        return internal::findParent<FloatingDockContainer *>(this);
    }

    void DockContainerWidget::closeOtherAreas(DockAreaWidget *keepOpenArea)
    {
        for (const auto dockArea : d->m_dockAreas) {
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

} // namespace ADS
