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

#include <QFrame>
#include <QToolButton>

QT_BEGIN_NAMESPACE
class QAbstractButton;
QT_END_NAMESPACE

namespace ADS {

class DockAreaTabBar;
class DockAreaWidget;
class DockAreaTitleBarPrivate;

using TitleBarButtonType = QToolButton;

/**
 * Title bar button of a dock area that customizes TitleBarButtonType appearance/behavior
 * according to various config flags such as:
 * DockManager::DockAreaHas_xxx_Button - if set to 'false' keeps the button always invisible
 * DockManager::DockAreaHideDisabledButtons - if set to 'true' hides button when it is disabled
 */
class TitleBarButton : public TitleBarButtonType
{
    Q_OBJECT
    bool m_visible = true;
    bool m_hideWhenDisabled = false;
public:
    using Super = TitleBarButtonType;
    TitleBarButton(bool visible = true, QWidget *parent = nullptr);

    /**
     * Adjust this visibility change request with our internal settings:
     */
    void setVisible(bool visible) override;

protected:
    /**
     * Handle EnabledChanged signal to set button invisible if the configured
     */
    bool event(QEvent *event) override;
};

/**
 * This spacer widget is here because of the following problem.
 * The dock area title bar handles mouse dragging and moving the floating widget.
 * The  problem is, that if the title bar becomes invisible, i.e. if the dock
 * area contains only one single dock widget and the dock area is moved
 * into a floating widget, then mouse events are not handled anymore and dragging
 * of the floating widget stops.
 */
class SpacerWidget : public QWidget
{
    Q_OBJECT
public:
    SpacerWidget(QWidget *parent = nullptr);
    QSize sizeHint() const override {return QSize(0, 0);}
    QSize minimumSizeHint() const override {return QSize(0, 0);}
};

/**
 * Title bar of a dock area.
 * The title bar contains a tabbar with all tabs for a dock widget group and
 * with a tabs menu button, a undock button and a close button.
 */
class ADS_EXPORT DockAreaTitleBar : public QFrame
{
    Q_OBJECT
private:
    DockAreaTitleBarPrivate *d; ///< private data (pimpl)
    friend class DockAreaTitleBarPrivate;

    void onTabsMenuAboutToShow();
    void onCloseButtonClicked();
    void onUndockButtonClicked();
    void onTabsMenuActionTriggered(QAction *action);
    void onCurrentTabChanged(int index);

protected:
    /**
     * Stores mouse position to detect dragging
     */
    void mousePressEvent(QMouseEvent *event) override;

    /**
     * Stores mouse position to detect dragging
     */
    void mouseReleaseEvent(QMouseEvent *event) override;

    /**
     * Starts floating the complete docking area including all dock widgets,
     * if it is not the last dock area in a floating widget
     */
    void mouseMoveEvent(QMouseEvent *event) override;

    /**
     * Double clicking the title bar also starts floating of the complete area
      */
    void mouseDoubleClickEvent(QMouseEvent *event) override;

    /**
     * Show context menu
     */
    void contextMenuEvent(QContextMenuEvent *event) override;

public:
    /**
     * Call this slot to tell the title bar that it should update the tabs menu
     * the next time it is shown.
     */
    void markTabsMenuOutdated();

    using Super = QFrame;
    /**
     * Default Constructor
     */
    DockAreaTitleBar(DockAreaWidget *parent);

    /**
     * Virtual Destructor
     */
    ~DockAreaTitleBar() override;

    /**
     * Returns the pointer to the tabBar()
     */
    DockAreaTabBar *tabBar() const;

    /**
     * Returns the button corresponding to the given title bar button identifier
     */
    QAbstractButton *button(eTitleBarButton which) const;

    /**
     * Updates the visibility of the dock widget actions in the title bar
     */
    void updateDockWidgetActionsButtons();

    /**
     * Marks the tabs menu outdated before it calls its base class
     * implementation
     */
    void setVisible(bool visible) override;

    /**
     * Inserts a custom widget at position index into this title bar.
     * If index is negative, the widget is added at the end.
     * You can use this function to insert custom widgets into the title bar.
     */
    void insertWidget(int index, QWidget *widget);

    /**
     * Searches for widget widget in this title bar.
     * You can use this function, to get the position of the default
     * widget in the tile bar.
     * \code
     * int tabBarIndex = TitleBar->indexOf(TitleBar->tabBar());
     * int closeButtonIndex = TitleBar->indexOf(TitleBar->button(TitleBarButtonClose));
     * \endcode
     */
    int indexOf(QWidget *widget) const;

signals:
    /**
     * This signal is emitted if a tab in the tab bar is clicked by the user
     * or if the user clicks on a tab item in the title bar tab menu.
     */
    void tabBarClicked(int index);
}; // class DockAreaTitleBar

} // namespace ADS
