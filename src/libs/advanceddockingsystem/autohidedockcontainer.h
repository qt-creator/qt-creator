// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#pragma once

#include "ads_globals.h"
#include "autohidetab.h"

#include <QSplitter>

QT_BEGIN_NAMESPACE
class QXmlStreamWriter;
QT_BEGIN_NAMESPACE

namespace ADS {

struct AutoHideDockContainerPrivate;
class DockManager;
class DockWidget;
class DockContainerWidget;
class AutoHideSideBar;
class DockAreaWidget;
class DockingStateReader;
struct SideTabBarPrivate;

/**
 * Auto hide container for hosting an auto hide dock widget
 */
class ADS_EXPORT AutoHideDockContainer : public QFrame
{
    Q_OBJECT
    Q_PROPERTY(int sideBarLocation READ sideBarLocation CONSTANT) // TODO
private:
    AutoHideDockContainerPrivate *d; ///< private data (pimpl)
    friend struct AutoHideDockContainerPrivate;
    friend AutoHideSideBar;
    friend SideTabBarPrivate;

protected:
    virtual bool eventFilter(QObject *watched, QEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void leaveEvent(QEvent *event) override;
    virtual bool event(QEvent *event) override;

    /**
     * Updates the size considering the size limits and the resize margins
     */
    void updateSize();

    /*
     * Saves the state and size
     */
    void saveState(QXmlStreamWriter &Stream);

public:
    /**
     * Create Auto Hide widget with the given dock widget
     */
    AutoHideDockContainer(DockWidget *dockWidget, SideBarLocation area, DockContainerWidget *parent);

    /**
     * Virtual Destructor
     */
    virtual ~AutoHideDockContainer();

    /**
     * Get's the side tab bar
     */
    AutoHideSideBar *autoHideSideBar() const;

    /**
     * Returns the side tab
     */
    AutoHideTab *autoHideTab() const;

    /**
     * Get's the dock widget in this dock container
     */
    DockWidget *dockWidget() const;

    /**
     * Adds a dock widget and removes the previous dock widget
     */
    void addDockWidget(DockWidget *dockWidget);

    /**
     * Returns the side tab bar area of this Auto Hide dock container
     */
    SideBarLocation sideBarLocation() const;

    /**
     * Sets a new SideBarLocation.
     * If a new side bar location is set, the auto hide dock container needs
     * to update its resize handle position
     */
    void setSideBarLocation(SideBarLocation sideBarLocation);

    /**
     * Returns the dock area widget of this Auto Hide dock container
     */
    DockAreaWidget *dockAreaWidget() const;

    /**
     * Returns the parent container that hosts this auto hide container
     */
    DockContainerWidget *dockContainer() const;

    /**
     * Moves the contents to the parent container widget
     * Used before removing this Auto Hide dock container
     */
    void moveContentsToParent();

    /**
     * Cleanups up the side tab widget and then deletes itself
     */
    void cleanupAndDelete();

    /**
     * Toggles the auto Hide dock container widget. This will also hide the side tab widget.
     */
    void toggleView(bool enable);

    /**
     * Collapses the auto hide dock container widget
     * Does not hide the side tab widget
     */
    void collapseView(bool enable);

    /**
     * Toggles the current collapse state
     */
    void toggleCollapseState();

    /**
     * Use this instead of resize.
     * Depending on the sidebar location this will set the width or height of this auto hide container.
     */
    void setSize(int size);

    /**
     * Returns orientation of this container.
     * Left and right containers have a Qt::Vertical orientation and top / bottom containers have
     * a Qt::Horizontal orientation. The function returns the orientation of the corresponding
     * auto hide side bar.
     */
    Qt::Orientation orientation() const;

    /**
     * Resets the with or hight to the initial dock widget size dependinng on the orientation.
     * If the orientation is Qt::Horizontal, then the height is reset to the initial size and if
     * orientation is Qt::Vertical, then the width is reset to the initial size.
     */
    void resetToInitialDockWidgetSize();

    /**
     * Removes the AutoHide container from the current side bar and adds it to the new side bar
     * given in SideBarLocation.
     */
    void moveToNewSideBarLocation(SideBarLocation sideBarLocation, int index = -1);

    /**
     * Returns the index of this container in the sidebar.
     */
    int tabIndex() const;
};

} // namespace ADS
