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

#include <QScrollArea>

namespace ADS {

class DockAreaWidget;
class DockWidgetTab;
class DockAreaTabBarPrivate;
class DockAreaTitleBar;
class FloatingDockContainer;
class AbstractFloatingWidget;

/**
 * Custom tabbar implementation for tab area that is shown on top of a
 * dock area widget.
 * The tabbar displays the tab widgets of the contained dock widgets.
 * We cannot use QTabBar here because it does a lot of fancy animations
 * that will crash the application if a tab is removed while the animation
 * has not finished. And we need to remove a tab, if the user drags a
 * a dock widget out of a group of tabbed widgets
 */
class ADS_EXPORT DockAreaTabBar : public QScrollArea
{
    Q_OBJECT
private:
    DockAreaTabBarPrivate *d; ///< private data (pimpl)
    friend class DockAreaTabBarPrivate;
    friend class DockAreaTitleBar;

    void onTabClicked();
    void onTabCloseRequested();
    void onCloseOtherTabsRequested();
    void onTabWidgetMoved(const QPoint &globalPos);

protected:
    void wheelEvent(QWheelEvent *event) override;

public:
    using Super = QScrollArea;

    /**
     * Default Constructor
     */
    DockAreaTabBar(DockAreaWidget *parent);

    /**
     * Virtual Destructor
     */
    ~DockAreaTabBar() override;

    /**
     * Inserts the given dock widget tab at the given position.
     * Inserting a new tab at an index less than or equal to the current index
     * will increment the current index, but keep the current tab.
     */
    void insertTab(int Index, DockWidgetTab *tab);

    /**
     * Removes the given DockWidgetTab from the tabbar
     */
    void removeTab(DockWidgetTab *tab);

    /**
     * Returns the number of tabs in this tabbar
     */
    int count() const;

    /**
     * Returns the current index or -1 if no tab is selected
     */
    int currentIndex() const;

    /**
      * Returns the current tab or a nullptr if no tab is selected.
      */
    DockWidgetTab *currentTab() const;

    /**
     * Returns the tab with the given index
     */
    DockWidgetTab *tab(int index) const;

    /**
     * Filters the tab widget events
     */
    bool eventFilter(QObject *watched, QEvent *event) override;

    /**
     * This function returns true if the tab is open, that means if it is
     * visible to the user. If the function returns false, the tab is
     * closed
     */
    bool isTabOpen(int index) const;

    /**
     * Overrides the minimumSizeHint() function of QScrollArea
     * The minimumSizeHint() is bigger than the sizeHint () for the scroll
     * area because even if the scrollbars are invisible, the required speace
     * is reserved in the minimumSizeHint(). This override simply returns
     * sizeHint();
     */
    QSize minimumSizeHint() const override;

    /**
     * The function provides a sizeHint that matches the height of the
     * internal viewport.
     */
    QSize sizeHint() const override;

    /**
     * This property sets the index of the tab bar's visible tab
     */
    void setCurrentIndex(int index);

    /**
     * This function will close the tab given in Index param.
     * Closing a tab means, the tab will be hidden, it will not be removed
     */
    void closeTab(int index);

signals:
    /**
     * This signal is emitted when the tab bar's current tab is about to be changed. The new
     * current has the given index, or -1 if there isn't a new one.
     */
    void currentChanging(int index);

    /**
     * This signal is emitted when the tab bar's current tab changes. The new
     * current has the given index, or -1 if there isn't a new one
     */
    void currentChanged(int index);

    /**
     * This signal is emitted when user clicks on a tab
     */
    void tabBarClicked(int index);

    /**
     * This signal is emitted when the close button on a tab is clicked.
     * The index is the index that should be closed.
     */
    void tabCloseRequested(int index);

    /**
     * This signal is emitted if a tab has been closed
     */
    void tabClosed(int index);

    /**
     * This signal is emitted if a tab has been opened.
     * A tab is opened if it has been made visible
     */
    void tabOpened(int index);

    /**
     * This signal is emitted when the tab has moved the tab at index position
     * from to index position to.
     */
    void tabMoved(int from, int to);

    /**
     * This signal is emitted, just before the tab with the given index is
     * removed
     */
    void removingTab(int index);

    /**
     * This signal is emitted if a tab has been inserted
     */
    void tabInserted(int index);

    /**
     * This signal is emitted when a tab title elide state has been changed
     */
    void elidedChanged(bool elided);
}; // class DockAreaTabBar

} // namespace ADS
