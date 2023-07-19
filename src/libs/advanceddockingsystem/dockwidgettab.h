// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#pragma once

#include "ads_globals.h"

#include <QFrame>
#include <QToolButton>

namespace ADS {

class DockWidget;
class DockAreaWidget;
class DockWidgetTabPrivate;

using TabButtonType = QToolButton;

class TabButton : public TabButtonType
{
    Q_OBJECT

public:
    using Super = TabButtonType;
    TabButton(QWidget *parent = nullptr);

    void setActive(bool value);
    void setFocus(bool value);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    bool m_active;
    bool m_focus;
};

/**
 * A dock widget tab that shows a title and an icon.
 * The dock widget tab is shown in the dock area title bar to switch between tabbed dock widgets.
 */
class ADS_EXPORT DockWidgetTab : public QFrame
{
    Q_OBJECT
    Q_PROPERTY(bool activeTab READ isActiveTab WRITE setActiveTab NOTIFY activeTabChanged)
    Q_PROPERTY(QSize iconSize READ iconSize WRITE setIconSize NOTIFY iconSizeChanged)

private:
    DockWidgetTabPrivate *d; ///< private data (pimpl)
    friend class DockWidgetTabPrivate;
    friend class DockWidget;
    friend class DockManager;
    friend class AutoHideDockContainer;

    void onDockWidgetFeaturesChanged();
    void detachDockWidget();
    void autoHideDockWidget();
    void onAutoHideToActionClicked();

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
     * Sets the dock area widget the dockWidget returned by dockWidget() function belongs to.
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

    /**
     * Returns the icon size.
     * If no explicit icon size has been set, the function returns an invalid QSize.
     */
    QSize iconSize() const;

    /**
     * Set an explicit icon size.
     * If no icon size has been set explicitly, than the tab sets the icon size depending
     * on the style.
     */
    void setIconSize(const QSize &size);

    void setVisible(bool visible) override;

signals:
    void activeTabChanged();
    void clicked();
    void closeRequested();
    void closeOtherTabsRequested();
    void moved(const QPoint &globalPosition);
    void elidedChanged(bool elided);
    void iconSizeChanged();
}; // class DockWidgetTab

} // namespace ADS
