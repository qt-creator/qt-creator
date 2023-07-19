// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#pragma once

#include "ads_globals.h"
#include "autohidetab.h"
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
class AutoHideSideBar;
class AutoHideTab;
struct AutoHideTabPrivate;
struct AutoHideDockContainerPrivate;
class AutoHideDockContainer;

/**
 * Container that manages a number of dock areas with single dock widgets or tabified dock widgets
 * in each area. Each window that support docking has a DockContainerWidget. That means the main
 * application window and all floating windows contain a DockContainerWidget instance.
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
    friend AutoHideDockContainer;
    friend AutoHideTab;
    friend AutoHideTabPrivate;
    friend AutoHideDockContainerPrivate;
    friend AutoHideSideBar;

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
     * Creates and initializes a dockwidget auto hide container into the given area. Initializing
     * inserts the tabs into the side tab widget and hides it
     * Returns nullptr if you try and insert into an area where the configuration is not enabled
     */
    AutoHideDockContainer *createAndSetupAutoHideContainer(SideBarLocation area,
                                                           DockWidget *dockWidget,
                                                           int tabIndex = -1);

    /**
     * Helper function for creation of the root splitter
     */
    void createRootSplitter();

    /**
     * Helper function for creation of the side tab bar widgets
     */
    void createSideTabBarWidgets();

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
    void dropWidget(QWidget *widget,
                    DockWidgetArea dropArea,
                    DockAreaWidget *targetAreaWidget,
                    int tabIndex = -1);

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
    void emitAndExit() const;

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

    /**
     * This function forces the dock container widget to update handles of splitters
     * based on resize modes of dock widgets contained in the container.
     */
    void updateSplitterHandles(QSplitter *splitter);

    /**
     * Registers the given floating widget in the internal list of auto hide widgets
     */
    void registerAutoHideWidget(AutoHideDockContainer *autoHideWidget);

    /**
     * Remove the given auto hide widget from the list of registered auto hide widgets
     */
    void removeAutoHideWidget(AutoHideDockContainer *autoHideWidget);

    /**
     * Handles widget events of auto hide widgets to trigger delayed show
     * or hide actions for auto hide container on auto hide tab mouse over
     */
    void handleAutoHideWidgetEvent(QEvent *e, QWidget *w);

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
                                  DockAreaWidget *dockAreaWidget = nullptr,
                                  int index = -1);

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
     * Returns a list for all open dock widgets in all open dock areas
     */
    QList<DockWidget *> openedDockWidgets() const;

    /**
     * This function returns true, if the container has open dock areas.
     * This functions is a little bit faster than calling openedDockAreas().isEmpty()
     * because it returns as soon as it finds an open dock area
     */
    bool hasOpenDockAreas() const;

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

    /**
     * Returns the side tab widget for the given area
     */
    AutoHideSideBar *autoHideSideBar(SideBarLocation area) const;

    /**
     * Access function for auto hide widgets
     */
    QList<AutoHideDockContainer *> autoHideWidgets() const;

    /**
     * Returns the content rectangle without the autohide side bars
     */
    QRect contentRect() const;

    /**
     * Returns the content rectangle mapped to global space.
     * The content rectangle is the rectangle of the dock manager widget minus
     * the auto hide side bars. So that means, it is the geometry of the
     * internal root splitter mapped to global space.
     */
    QRect contentRectGlobal() const;

    /**
     * Returns the dock manager that owns this container
     */
    DockManager *dockManager() const;

signals:
    /**
     * This signal is emitted if one or multiple dock areas has been added to
     * the internal list of dock areas.
     * If multiple dock areas are inserted, this signal is emitted only once
     */
    void dockAreasAdded();

    /**
     * This signal is emitted, if a new auto hide widget has been created.
     */
    void autoHideWidgetCreated(AutoHideDockContainer *autoHideWidget);

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
