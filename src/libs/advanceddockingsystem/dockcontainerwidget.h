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

#pragma once

#include "ads_globals.h"
#include "dockwidget.h"

#include <QFrame>

QT_BEGIN_NAMESPACE
class QXmlStreamWriter;
QT_END_NAMESPACE

namespace ADS {

class DockContainerWidgetPrivate;
class DockAreaWidget;
class DockWidget;
class DockManager;
class DockManagerPrivate;
class FloatingDockContainer;
class FloatingDockContainerPrivate;
class FloatingDragPreview;
class DockingStateReader;
class FloatingDragPreviewPrivate;

/**
 * Container that manages a number of dock areas with single dock widgets
 * or tabified dock widgets in each area.
 * Each window that support docking has a DockContainerWidget. That means
 * the main application window and all floating windows contain a
 * DockContainerWidget instance.
 */
class ADS_EXPORT DockContainerWidget : public QFrame
{
    Q_OBJECT
private:
    DockContainerWidgetPrivate *d; ///< private data (pimpl)
    friend class DockContainerWidgetPrivate;
    friend class DockManager;
    friend class DockManagerPrivate;
    friend class DockAreaWidget;
    friend struct DockAreaWidgetPrivate;
    friend class FloatingDockContainer;
    friend class FloatingDockContainerPrivate;
    friend class DockWidget;
    friend class FloatingDragPreview;
    friend class FloatingDragPreviewPrivate;

protected:
    /**
     * Handles activation events to update zOrderIndex
     */
    bool event(QEvent *event) override;

    /**
     * Access function for the internal root splitter
     */
    QSplitter *rootSplitter() const;

    /**
     * Helper function for creation of the root splitter
     */
    void createRootSplitter();

    /**
     * Drop floating widget into the container
     */
    void dropFloatingWidget(FloatingDockContainer *floatingWidget, const QPoint &targetPos);

    /**
     * Drop a dock area or a dock widget given in widget parameter.
     * If the TargetAreaWidget is a nullptr, then the DropArea indicates
     * the drop area for the container. If the given TargetAreaWidget is not
     * a nullptr, then the DropArea indicates the drop area in the given
     * TargetAreaWidget
     */
    void dropWidget(QWidget *widget, DockWidgetArea dropArea, DockAreaWidget *targetAreaWidget);

    /**
     * Adds the given dock area to this container widget
     */
    void addDockArea(DockAreaWidget *dockAreaWidget, DockWidgetArea area = CenterDockWidgetArea);

    /**
     * Removes the given dock area from this container
     */
    void removeDockArea(DockAreaWidget *area);

    /**
     * This function replaces the goto construct. Still need to write a good description.
     */
    void emitAndExit() const; // TODO rename

    /**
     * Saves the state into the given stream
     */
    void saveState(QXmlStreamWriter &stream) const;

    /**
     * Restores the state from given stream.
     * If Testing is true, the function only parses the data from the given
     * stream but does not restore anything. You can use this check for
     * faulty files before you start restoring the state
     */
    bool restoreState(DockingStateReader &stream, bool testing);

    /**
     * This function returns the last added dock area widget for the given
     * area identifier or 0 if no dock area widget has been added for the given
     * area
     */
    DockAreaWidget *lastAddedDockAreaWidget(DockWidgetArea area) const;

    /**
     * If hasSingleVisibleDockWidget() returns true, this function returns the
     * one and only visible dock widget. Otherwise it returns a nullptr.
     */
    DockWidget *topLevelDockWidget() const;

    /**
     * Returns the top level dock area.
     */
    DockAreaWidget *topLevelDockArea() const;

    /**
    * This function returns a list of all dock widgets in this floating widget.
    * It may be possible, depending on the implementation, that dock widgets,
    * that are not visible to the user have no parent widget. Therefore simply
    * calling findChildren() would not work here. Therefore this function
    * iterates over all dock areas and creates a list that contains all
    * dock widgets returned from all dock areas.
    */
    QList<DockWidget *> dockWidgets() const;

public:
    /**
     * Default Constructor
     */
    DockContainerWidget(DockManager *dockManager, QWidget *parent = nullptr);

    /**
     * Virtual Destructor
     */
    ~DockContainerWidget() override;

    /**
     * Adds dockwidget into the given area.
     * If DockAreaWidget is not null, then the area parameter indicates the area
     * into the DockAreaWidget. If DockAreaWidget is null, the Dockwidget will
     * be dropped into the container.
     * \return Returns the dock area widget that contains the new DockWidget
     */
    DockAreaWidget *addDockWidget(DockWidgetArea area,
                                  DockWidget *dockWidget,
                                  DockAreaWidget *dockAreaWidget = nullptr);

    /**
     * Removes dockwidget
     */
    void removeDockWidget(DockWidget *dockWidget);

    /**
     * Returns the current zOrderIndex
     */
    virtual unsigned int zOrderIndex() const;

    /**
     * This function returns true if this container widgets z order index is
     * higher than the index of the container widget given in Other parameter
     */
    bool isInFrontOf(DockContainerWidget *other) const;

    /**
     * Returns the dock area at the given global position or 0 if there is no
     * dock area at this position
     */
    DockAreaWidget *dockAreaAt(const QPoint &globalPos) const;

    /**
     * Returns the dock area at the given Index or 0 if the index is out of
     * range
     */
    DockAreaWidget *dockArea(int index) const;

    /**
     * Returns the list of dock areas that are not closed
     * If all dock widgets in a dock area are closed, the dock area will be closed
     */
    QList<DockAreaWidget *> openedDockAreas() const;

    /**
     * This function returns true if this dock area has only one single
     * visible dock widget.
     * A top level widget is a real floating widget. Only the isFloating()
     * function of top level widgets may returns true.
     */
    bool hasTopLevelDockWidget() const;

    /**
     * Returns the number of dock areas in this container
     */
    int dockAreaCount() const;

    /**
     * Returns the number of visible dock areas
     */
    int visibleDockAreaCount() const;

    /**
     * This function returns true, if this container is in a floating widget
     */
    bool isFloating() const;

    /**
     * Dumps the layout for debugging purposes
     */
    void dumpLayout() const;

    /**
     * This functions returns the dock widget features of all dock widget in
     * this container.
     * A bitwise and is used to combine the flags of all dock widgets. That
     * means, if only dock widget does not support a certain flag, the whole
     * dock are does not support the flag.
     */
    DockWidget::DockWidgetFeatures features() const;

    /**
     * If this dock container is in a floating widget, this function returns
     * the floating widget.
     * Else, it returns a nullptr.
     */
    FloatingDockContainer *floatingWidget() const;

    /**
     * Call this function to close all dock areas except the KeepOpenArea
     */
    void closeOtherAreas(DockAreaWidget *keepOpenArea);

signals:
    /**
     * This signal is emitted if one or multiple dock areas has been added to
     * the internal list of dock areas.
     * If multiple dock areas are inserted, this signal is emitted only once
     */
    void dockAreasAdded();

    /**
     * This signal is emitted if one or multiple dock areas has been removed
     */
    void dockAreasRemoved();

    /**
     * This signal is emitted if a dock area is opened or closed via
     * toggleView() function
     */
    void dockAreaViewToggled(DockAreaWidget *dockArea, bool open);
}; // class DockContainerWidget

} // namespace ADS
