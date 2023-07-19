// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#pragma once

#include "ads_globals.h"
#include "autohidetab.h"
#include "dockwidget.h"

#include <QFrame>

QT_BEGIN_NAMESPACE
class QAbstractButton;
class QXmlStreamWriter;
QT_END_NAMESPACE

namespace ADS {

struct DockAreaWidgetPrivate;
class DockManager;
class DockContainerWidget;
class DockContainerWidgetPrivate;
class DockAreaTitleBar;
class DockingStateReader;
class AutoHideDockContainer;

/**
 * DockAreaWidget manages multiple instances of DockWidgets.
 * It displays a title tab, which is clickable and will switch to
 * the contents associated to the title when clicked.
 */
class ADS_EXPORT DockAreaWidget : public QFrame
{
    Q_OBJECT
private:
    DockAreaWidgetPrivate *d; ///< private data (pimpl)
    friend struct DockAreaWidgetPrivate;
    friend class DockContainerWidget;
    friend class DockContainerWidgetPrivate;
    friend class DockWidgetTab;
    friend class DockWidgetPrivate;
    friend class DockWidget;
    friend class DockManagerPrivate;
    friend class DockManager;
    friend class AutoHideDockContainer;

    void onDockWidgetFeaturesChanged();

    void onTabCloseRequested(int index);

    /**
     * Reorder the index position of DockWidget at fromIndx to toIndex
     * if a tab in the tabbar is dragged from one index to another one
     */
    void reorderDockWidget(int fromIndex, int toIndex);

    /*
     * Update the auto hide button checked state based on if it's contained in an auto hide container or not
     */
    void updateAutoHideButtonCheckState();

    /*
     * Update the title bar button tooltips
     */
    void updateTitleBarButtonsToolTips();

    /**
     * Calculate the auto hide side bar location depending on the dock area
     * widget position in the container
     */
    SideBarLocation calculateSideTabBarArea() const;

protected:

#ifdef Q_OS_WIN
    /**
     * Reimplements QWidget::event to handle QEvent::PlatformSurface
     * This is here to fix issue #294 Tab refresh problem with a QGLWidget
     * that exists since Qt version 5.12.7. So this function is here to
     * work around a Qt issue.
     */
    virtual bool event(QEvent *event) override;
#endif

    /**
     * Inserts a dock widget into dock area.
     * All dockwidgets in the dock area tabified in a stacked layout with tabs.
     * The index indicates the index of the new dockwidget in the tabbar and
     * in the stacked layout. If the Activate parameter is true, the new
     * DockWidget will be the active one in the stacked layout
     */
    void insertDockWidget(int index, DockWidget *dockWidget, bool activate = true);

    /**
     * Add a new dock widget to dock area.
     * All dockwidgets in the dock area tabified in a stacked layout with tabs
     */
    void addDockWidget(DockWidget *dockWidget);

    /**
     * Removes the given dock widget from the dock area
     */
    void removeDockWidget(DockWidget *dockWidget);

    /**
     * Called from dock widget if it is opened or closed
     */
    void toggleDockWidgetView(DockWidget *dockWidget, bool open);

    /**
     * This is a helper function to get the next open dock widget to activate
     * if the given DockWidget will be closed or removed.
     * The function returns the next widget that should be activated or
     * nullptr in case there are no more open widgets in this area.
     */
    DockWidget *nextOpenDockWidget(DockWidget *dockWidget) const;

    /**
     * Returns the index of the given DockWidget in the internal layout
     */
    int indexOf(DockWidget *dockWidget);

    /**
     * Call this function, if you already know, that the dock does not
     * contain any visible content (any open dock widgets).
     */
    void hideAreaWithNoVisibleContent();

    /**
     * Updates the dock area layout and components visibility
     */
    void updateTitleBarVisibility();

    /**
     * This is the internal private function for setting the current widget.
     * This function is called by the public setCurrentDockWidget() function
     * and by the dock manager when restoring the state
     */
    void internalSetCurrentDockWidget(DockWidget *dockWidget);

    /**
     * Marks tabs menu to update
     */
    void markTitleBarMenuOutdated();

    /*
     * Update the title bar button visibility based on if it's top level or not
     */
    void updateTitleBarButtonVisibility(bool isTopLevel) const;

    void toggleView(bool open);

public:
    using Super = QFrame;

    /**
     * Dock area related flags
     */
    enum eDockAreaFlag
    {
        HideSingleWidgetTitleBar = 0x0001,
        DefaultFlags = 0x0000
    };
    Q_DECLARE_FLAGS(DockAreaFlags, eDockAreaFlag)

    /**
     * Default Constructor
     */
    DockAreaWidget(DockManager *dockManager, DockContainerWidget *parent);

    /**
     * Virtual Destructor
     */
    ~DockAreaWidget() override;

    /**
     * Returns the dock manager object this dock area belongs to
     */
    DockManager *dockManager() const;

    /**
     * Returns the dock container widget this dock area widget belongs to or 0
     * if there is no
     */
    DockContainerWidget *dockContainer() const;
    /**
     * Returns the auto hide dock container widget this dock area widget belongs to or 0
     * if there is no
     */
    AutoHideDockContainer *autoHideDockContainer() const;

    /**
     * Returns true if the dock area is in an auto hide container
     */
    bool isAutoHide() const;

    /**
     * Sets the current auto hide dock container
     */
    void setAutoHideDockContainer(AutoHideDockContainer *autoHideDockContainer);

    /**
     * Returns the largest minimumSizeHint() of the dock widgets in this area.
     * The minimum size hint is updated if a dock widget is removed or added.
     */
    virtual QSize minimumSizeHint() const override;

    /**
     * Returns the rectangle of the title area
     */
    QRect titleBarGeometry() const;

    /**
     * Returns the rectangle of the content
     */
    QRect contentAreaGeometry() const;

    /**
     * Returns the number of dock widgets in this area
     */
    int dockWidgetsCount() const;

    /**
     * Returns a list of all dock widgets in this dock area.
     * This list contains open and closed dock widgets.
     */
    QList<DockWidget *> dockWidgets() const;

    /**
     * Returns the number of open dock widgets in this area
     */
    int openDockWidgetsCount() const;

    /**
     * Returns a list of dock widgets that are not closed.
     */
    QList<DockWidget *> openedDockWidgets() const;

    /**
     * Returns a dock widget by its index
     */
    DockWidget *dockWidget(int indexOf) const;

    /**
     * Returns the index of the current active dock widget or -1 if there
     * are is no active dock widget (ie.e if all dock widgets are closed)
     */
    int currentIndex() const;

    /**
     * Returns the index of the first open dock widgets in the list of
     * dock widgets.
     * This function is here for performance reasons. Normally it would
     * be possible to take the first dock widget from the list returned by
     * openedDockWidgets() function. But that function enumerates all
     * dock widgets while this functions stops after the first open dock widget.
     * If there are no open dock widgets, the function returns -1.
     */
    int indexOfFirstOpenDockWidget() const;

    /**
     * Returns the current active dock widget or a nullptr if there is no
     * active dock widget (i.e. if all dock widgets are closed)
     */
    DockWidget *currentDockWidget() const;

    /**
     * Shows the tab with the given dock widget
     */
    void setCurrentDockWidget(DockWidget *dockWidget);

    /**
     * Saves the state into the given stream
     */
    void saveState(QXmlStreamWriter &stream) const;

    /**
     * Restores a dock area.
     * \see restoreChildNodes() for details
     */
    static bool restoreState(DockingStateReader& stream, DockAreaWidget*& createdWidget, bool testing, DockContainerWidget* parentContainer);

    /**
     * This functions returns the dock widget features of all dock widget in
     * this area.
     * A bitwise and is used to combine the flags of all dock widgets. That
     * means, if only one single dock widget does not support a certain flag,
     * the whole dock are does not support the flag. I.e. if one single
     * dock widget in this area is not closable, the whole dock are is not
     * closable.
     */
    DockWidget::DockWidgetFeatures features(eBitwiseOperator mode = BitwiseAnd) const;

    /**
     * Returns the title bar button corresponding to the given title bar
     * button identifier
     */
    QAbstractButton *titleBarButton(eTitleBarButton which) const;

    /**
     * Update the close button if visibility changed
     */
    void setVisible(bool visible) override;

    /**
     * Configures the areas of this particular dock area that are allowed for docking
     */
    void setAllowedAreas(DockWidgetAreas areas);

    /**
     * Returns flags with all allowed drop areas of this particular dock area
     */
    DockWidgetAreas allowedAreas() const;

    /**
     * Returns the title bar of this dock area
     */
    DockAreaTitleBar *titleBar() const;

    /**
     * Returns the dock area flags - a combination of flags that configure the
     * appearance and features of the dock area.
     * \see setDockAreaFlasg()
     */
    DockAreaFlags dockAreaFlags() const;

    /**
     * Sets the dock area flags - a combination of flags that configure the
     * appearance and features of the dock area
     */
    void setDockAreaFlags(DockAreaFlags flags);

    /**
     * Sets the dock area flag Flag on this widget if on is true; otherwise
     * clears the flag.
     */
    void setDockAreaFlag(eDockAreaFlag flag, bool on);

    /**
     * Returns true if the area has a single dock widget and contains the central widget of it's manager.
     */
    bool isCentralWidgetArea() const;

    /**
     * Returns true if the area contains the central widget of it's manager.
     */
    bool containsCentralWidget() const;

    /**
     * If this dock area is the one and only visible area in a container, then
     * this function returns true
     */
    bool isTopLevelArea() const;

    /**
     * This activates the tab for the given tab index.
     * If the dock widget for the given tab is not visible, the this function
     * call will make it visible.
     */
    void setCurrentIndex(int indexOf);

    /**
     * Closes the dock area and all dock widgets in this area
     */
    void closeArea();

    /**
     * Sets the dock area into auto hide mode or into normal mode.
     * If the dock area is switched to auto hide mode, then all dock widgets
     * that are pinable will be added to the sidebar
     */
    void setAutoHide(bool enable, SideBarLocation location = SideBarNone, int tabIndex = -1);

    /**
     * Switches the dock area to auto hide mode or vice versa depending on its
     * current state.
     */
    void toggleAutoHide(SideBarLocation location = SideBarNone);

    /**
     * This function closes all other areas except of this area
     */
    void closeOtherAreas();

    /**
     * Moves the dock area into its own floating widget if the area DockWidgetFloatable flag is true.
     */
    void setFloating();

signals:
    /**
     * This signal is emitted when user clicks on a tab at an index.
     */
    void tabBarClicked(int indexOf);

    /**
    * This signal is emitted when the tab bar's current tab is about to be changed. The new
    * current has the given index, or -1 if there isn't a new one.
    * @param index
    */
    void currentChanging(int indexOf);

    /**
     * This signal is emitted when the tab bar's current tab changes. The new
     * current has the given index, or -1 if there isn't a new one
     * @param index
     */
    void currentChanged(int indexOf);

    /**
     * This signal is emitted if the visibility of this dock area is toggled
     * via toggle view function
     */
    void viewToggled(bool open);
}; // class DockAreaWidget

} // namespace ADS
