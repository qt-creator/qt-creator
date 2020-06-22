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

namespace ADS {

class DockWidget;
class DockAreaWidget;
class DockWidgetTabPrivate;

/**
 * A dock widget tab that shows a title and an icon.
 * The dock widget tab is shown in the dock area title bar to switch between
 * tabbed dock widgets
 */
class ADS_EXPORT DockWidgetTab : public QFrame
{
    Q_OBJECT
    Q_PROPERTY(bool activeTab READ isActiveTab WRITE setActiveTab NOTIFY activeTabChanged)

private:
    DockWidgetTabPrivate *d; ///< private data (pimpl)
    friend class DockWidgetTabPrivate;
    friend class DockWidget;
    friend class DockManager;
    void onDockWidgetFeaturesChanged();
    void detachDockWidget();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

    /**
     * Double clicking the tab widget makes the assigned dock widget floating
     */
    void mouseDoubleClickEvent(QMouseEvent *event) override;

public:
    using Super = QFrame;
    /**
     * Default Constructor
     * param[in] DockWidget The dock widget this title bar belongs to
     * param[in] parent The parent widget of this title bar
     */
    DockWidgetTab(DockWidget *DockWidget, QWidget *parent = nullptr);

    /**
     * Virtual Destructor
     */
    ~DockWidgetTab() override;

    /**
     * Returns true, if this is the active tab
     */
    bool isActiveTab() const;

    /**
     * Set this true to make this tab the active tab
     */
    void setActiveTab(bool active);

    /**
     * Returns the dock widget this title widget belongs to
     */
    DockWidget *dockWidget() const;

    /**
     * Sets the dock area widget the dockWidget returned by dockWidget()
     * function belongs to.
     */
    void setDockAreaWidget(DockAreaWidget *dockArea);

    /**
     * Returns the dock area widget this title bar belongs to.
     * \return This function returns 0 if the dock widget that owns this title
     * bar widget has not been added to any dock area yet.
     */
    DockAreaWidget *dockAreaWidget() const;

    /**
     * Sets the icon to show in title bar
     */
    void setIcon(const QIcon &icon);

    /**
     * Returns the icon
     */
    const QIcon &icon() const;

    /**
     * Returns the tab text
     */
    QString text() const;

    /**
     * Sets the tab text
     */
    void setText(const QString &title);

    /**
     * Returns true if text is elided on the tab's title
     */
    bool isTitleElided() const;

    /**
     * This function returns true if the assigned dock widget is closable
     */
    bool isClosable() const;

    /**
     * Track event ToolTipChange and set child ToolTip
     */
    bool event(QEvent *event) override;

    /**
     * Sets the text elide mode
     */
    void setElideMode(Qt::TextElideMode mode);

    /**
     * Update stylesheet style if a property changes
     */
    void updateStyle();

    void setVisible(bool visible) override;

signals:
    void activeTabChanged();
    void clicked();
    void closeRequested();
    void closeOtherTabsRequested();
    void moved(const QPoint &globalPosition);
    void elidedChanged(bool elided);
}; // class DockWidgetTab

} // namespace ADS
