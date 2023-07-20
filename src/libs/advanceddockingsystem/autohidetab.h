// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#pragma once

#include "pushbutton.h"

#include "ads_globals.h"

namespace ADS {

struct AutoHideTabPrivate;
class DockWidget;
class AutoHideSideBar;
class DockWidgetTab;
class DockContainerWidgetPrivate;

/**
 * A dock widget Side tab that shows a title or an icon.
 * The dock widget tab is shown in the side tab bar to switch between pinned dock widgets.
 */
class ADS_EXPORT AutoHideTab : public PushButton
{
    Q_OBJECT

    Q_PROPERTY(int sideBarLocation READ sideBarLocation CONSTANT)
    Q_PROPERTY(Qt::Orientation orientation READ orientation CONSTANT)
    Q_PROPERTY(bool activeTab READ isActiveTab CONSTANT)
    Q_PROPERTY(bool iconOnly READ iconOnly CONSTANT)

private:
    AutoHideTabPrivate *d; ///< private data (pimpl)
    friend struct AutoHideTabPrivate;
    friend class DockWidget;
    friend class AutoHideDockContainer;
    friend class AutoHideSideBar;
    friend class DockAreaWidget;
    friend class DockContainerWidget;
    friend DockContainerWidgetPrivate;

    void onAutoHideToActionClicked();

protected:
    void setSideBar(AutoHideSideBar *sideTabBar);
    void removeFromSideBar();
    virtual bool event(QEvent *event) override;
    virtual void contextMenuEvent(QContextMenuEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;

public:
    using Super = PushButton;

    /**
     * Default Constructor
     * param[in] parent The parent widget of this title bar
     */
    AutoHideTab(QWidget *parent = nullptr);

    /**
     * Virtual Destructor
     */
    virtual ~AutoHideTab();

    /**
     * Update stylesheet style if a property changes.
     */
    void updateStyle();

    /**
     * Getter for side tab bar area property.
     */
    SideBarLocation sideBarLocation() const;

    /**
     * Set orientation vertical or horizontal.
     */
    void setOrientation(Qt::Orientation value);

    /**
     * Returns the current orientation.
     */
    Qt::Orientation orientation() const;

    /**
     * Returns true, if this is the active tab. The tab is active if the auto hide widget is visible.
     */
    bool isActiveTab() const;

    /**
     * Returns the dock widget this belongs to.
     */
    DockWidget *dockWidget() const;

    /**
     * Sets the dock widget that is controlled by this tab.
     */
    void setDockWidget(DockWidget *dockWidget);

    /**
     * Returns true if the auto hide config flag AutoHideSideBarsIconOnly is set and if
     * the tab has an icon - that means the icon is not null.
     */
    bool iconOnly() const;

    /**
     * Returns the side bar that contains this tab or a nullptr if the tab is not in a side bar.
     */
    AutoHideSideBar *sideBar() const;

    /**
     * Returns the index of this tab in the sideBar.
     */
    int tabIndex() const;

    /**
     * Set the dock widget floating, if it is floatable
     */
    void setDockWidgetFloating();

    /**
     * Unpin and dock the auto hide widget.
     */
    void unpinDockWidget();

    /**
     * Calls the requestCloseDockWidget() function for the assigned dock widget.
     */
    void requestCloseDockWidget();
}; // class AutoHideTab

} // namespace ADS
