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

QT_BEGIN_NAMESPACE
class QToolBar;
class QXmlStreamWriter;
QT_END_NAMESPACE

namespace ADS {

class DockWidgetPrivate;
class DockWidgetTab;
class DockManager;
class DockContainerWidget;
class DockAreaWidget;
class DockContainerWidgetPrivate;
class FloatingDockContainer;

/**
 * The QDockWidget class provides a widget that can be docked inside a
 * DockManager or floated as a top-level window on the desktop.
 */
class ADS_EXPORT DockWidget : public QFrame
{
    Q_OBJECT
private:
    DockWidgetPrivate *d; ///< private data (pimpl)
    friend class DockWidgetPrivate;

    /**
     * Adjusts the toolbar icon sizes according to the floating state
     */
    void setToolbarFloatingStyle(bool topLevel);

protected:
    friend class DockContainerWidget;
    friend class DockAreaWidget;
    friend class FloatingDockContainer;
    friend class DockManager;
    friend class DockManagerPrivate;
    friend class DockContainerWidgetPrivate;
    friend class DockAreaTabBar;
    friend class DockWidgetTab;
    friend class DockWidgetTabPrivate;
    friend class DockAreaTitleBarPrivate;

    /**
     * Assigns the dock manager that manages this dock widget
     */
    void setDockManager(DockManager *dockManager);

    /**
     * If this dock widget is inserted into a dock area, the dock area will
     * be registered on this widget via this function. If a dock widget is
     * removed from a dock area, this function will be called with nullptr
     * value.
     */
    void setDockArea(DockAreaWidget *dockArea);

    /**
     * This function changes the toggle view action without emitting any
     * signal
     */
    void setToggleViewActionChecked(bool checked);

    /**
     * Saves the state into the given stream
     */
    void saveState(QXmlStreamWriter &stream) const;

    /**
     * This is a helper function for the dock manager to flag this widget
     * as unassigned.
     * When calling the restore function, it may happen, that the saved state
     * contains less dock widgets then currently available. All widgets whose
     * data is not contained in the saved state, are flagged as unassigned
     * after the restore process. If the user shows an unassigned dock widget,
     * a floating widget will be created to take up the dock widget.
     */
    void flagAsUnassigned();

    /**
     * Call this function to emit a topLevelChanged() signal and to update
     * the dock area tool bar visibility
     */
    static void emitTopLevelEventForWidget(DockWidget *topLevelDockWidget, bool floating);

    /**
     * Use this function to emit a top level changed event.
     * Do never use emit topLevelChanged(). Always use this function because
     * it only emits a signal if the floating state has really changed
     */
    void emitTopLevelChanged(bool floating);

    /**
     * Internal function for modifying the closed state when restoring
     * a saved docking state
     */
    void setClosedState(bool closed);

    /**
     * Internal toggle view function that does not check if the widget
     * already is in the given state
     */
    void toggleViewInternal(bool open);

    /**
     * Internal close dock widget implementation.
     * The function returns true if the dock widget has been closed or hidden
     */
    bool closeDockWidgetInternal(bool forceClose = false);

public:
    using Super = QFrame;

    enum DockWidgetFeature {
        DockWidgetClosable = 0x01,///< dock widget has a close button
        DockWidgetMovable = 0x02,///< dock widget is movable and can be moved to a new position in the current dock container
        DockWidgetFloatable = 0x04,
        DockWidgetDeleteOnClose = 0x08, ///< deletes the dock widget when it is closed
        CustomCloseHandling = 0x10,
        DefaultDockWidgetFeatures = DockWidgetClosable | DockWidgetMovable | DockWidgetFloatable,
        AllDockWidgetFeatures = DefaultDockWidgetFeatures | DockWidgetDeleteOnClose | CustomCloseHandling,
        NoDockWidgetFeatures = 0x00
    };
    Q_DECLARE_FLAGS(DockWidgetFeatures, DockWidgetFeature)

    enum eState { StateHidden, StateDocked, StateFloating };

    /**
     * Sets the widget for the dock widget to widget.
     * The InsertMode defines how the widget is inserted into the dock widget.
     * The content of a dock widget should be resizable do a very small size to
     * prevent the dock widget from blocking the resizing. To ensure, that a
     * dock widget can be resized very well, it is better to insert the content+
     * widget into a scroll area or to provide a widget that is already a scroll
     * area or that contains a scroll area.
     * If the InsertMode is AutoScrollArea, the DockWidget tries to automatically
     * detect how to insert the given widget. If the widget is derived from
     * QScrollArea (i.e. an QAbstractItemView), then the widget is inserted
     * directly. If the given widget is not a scroll area, the widget will be
     * inserted into a scroll area.
     * To force insertion into a scroll area, you can also provide the InsertMode
     * ForceScrollArea. To prevent insertion into a scroll area, you can
     * provide the InsertMode ForceNoScrollArea
     */
    enum eInsertMode { AutoScrollArea, ForceScrollArea, ForceNoScrollArea };

    /**
     * The mode of the minimumSizeHint() that is returned by the DockWidget
     * minimumSizeHint() function.
     * To ensure, that a dock widget does not block resizing, the dock widget
     * reimplements minimumSizeHint() function to return a very small minimum
     * size hint. If you would like to adhere the minimumSizeHint() from the
     * content widget, the set the minimumSizeHintMode() to
     * MinimumSizeHintFromContent.
     */
    enum eMinimumSizeHintMode { MinimumSizeHintFromDockWidget, MinimumSizeHintFromContent };

    /**
     * This mode configures the behavior of the toggle view action.
     * If the mode if ActionModeToggle, then the toggle view action is
     * a checkable action to show / hide the dock widget. If the mode
     * is ActionModeShow, then the action is not checkable an it will
     * always show the dock widget if clicked. If the mode is ActionModeShow,
     * the user can only close the DockWidget with the close button.
     */
    enum eToggleViewActionMode {
        ActionModeToggle, //!< ActionModeToggle
        ActionModeShow    //!< ActionModeShow
    };

    /**
     * This constructor creates a dock widget with the given title.
     * The title is the text that is shown in the window title when the dock
     * widget is floating and it is the title that is shown in the titlebar
     * or the tab of this dock widget if it is tabified.
     * The object name of the dock widget is also set to the title. The
     * object name is required by the dock manager to properly save and restore
     * the state of the dock widget. That means, the title needs to be unique.
     * If your title is not unique or if you would like to change the title
     * during runtime, you need to set a unique object name explicitly
     * by calling setObjectName() after construction.
     * Use the layoutFlags to configure the layout of the dock widget.
     */
    DockWidget(const QString &uniqueId, QWidget *parent = nullptr);

    /**
     * Virtual Destructor
     */
    ~DockWidget() override;

    /**
     * We return a fixed minimum size hint or the size hint of the content
     * widget if minimum size hint mode is MinimumSizeHintFromContent
     */
    QSize minimumSizeHint() const override;

    /**
     * Sets the widget for the dock widget to widget.
     * The InsertMode defines how the widget is inserted into the dock widget.
     * The content of a dock widget should be resizable do a very small size to
     * prevent the dock widget from blocking the resizing. To ensure, that a
     * dock widget can be resized very well, it is better to insert the content+
     * widget into a scroll area or to provide a widget that is already a scroll
     * area or that contains a scroll area.
     * If the InsertMode is AutoScrollArea, the DockWidget tries to automatically
     * detect how to insert the given widget. If the widget is derived from
     * QScrollArea (i.e. an QAbstractItemView), then the widget is inserted
     * directly. If the given widget is not a scroll area, the widget will be
     * inserted into a scroll area.
     * To force insertion into a scroll area, you can also provide the InsertMode
     * ForceScrollArea. To prevent insertion into a scroll area, you can
     * provide the InsertMode ForceNoScrollArea
     */
    void setWidget(QWidget *widget, eInsertMode insertMode = AutoScrollArea);

    /**
     * Remove the widget from the dock and give ownership back to the caller
     */
    QWidget *takeWidget();

    /**
     * Returns the widget for the dock widget. This function returns zero if
     * the widget has not been set.
     */
    QWidget *widget() const;

    /**
     * Returns the tab widget of this dock widget that is shown in the dock
     * area title bar
     */
    DockWidgetTab *tabWidget() const;

    /**
     * Sets, whether the dock widget is movable, closable, and floatable.
     */
    void setFeatures(DockWidgetFeatures features);

    /**
     * Sets the feature flag for this dock widget if on is true; otherwise
     * clears the flag.
     */
    void setFeature(DockWidgetFeature flag, bool on);

    /**
     * This property holds whether the dock widget is movable, closable, and
     * floatable.
     * By default, this property is set to a combination of DockWidgetClosable,
     * DockWidgetMovable and DockWidgetFloatable.
     */
    DockWidgetFeatures features() const;

    /**
     * Returns the dock manager that manages the dock widget or 0 if the widget
     * has not been assigned to any dock manager yet
     */
    DockManager *dockManager() const;

    /**
     * Returns the dock container widget this dock area widget belongs to or 0
     * if this dock widget has not been docked yet
     */
    DockContainerWidget *dockContainer() const;

    /**
     * Returns the dock area widget this dock widget belongs to or 0
     * if this dock widget has not been docked yet
     */
    DockAreaWidget *dockAreaWidget() const;

    /**
     * This property holds whether the dock widget is floating.
     * A dock widget is only floating, if it is the one and only widget inside
     * of a floating container. If there are more than one dock widget in a
     * floating container, the all dock widgets are docked and not floating.
     */
    bool isFloating() const;

    /**
     * This function returns true, if this dock widget is in a floating.
     * The function returns true, if the dock widget is floating and it also
     * returns true if it is docked inside of a floating container.
     */
    bool isInFloatingContainer() const;

    /**
     * Returns true, if this dock widget is closed.
     */
    bool isClosed() const;

    /**
     * Returns a checkable action that can be used to show or close this dock widget.
     * The action's text is set to the dock widget's window title.
     */
    QAction *toggleViewAction() const;

    /**
     * Configures the behavior of the toggle view action.
     * \see eToggleViewActionMode for a detailed description
     */
    void setToggleViewActionMode(eToggleViewActionMode mode);

    /**
     * Configures the minimum size hint that is returned by the
     * minimumSizeHint() function.
     * \see eMinimumSizeHintMode for a detailed description
     */
    void setMinimumSizeHintMode(eMinimumSizeHintMode mode);

    /**
     * Sets the dock widget icon that is shown in tabs and in toggle view
     * actions
     */
    void setIcon(const QIcon &icon);

    /**
     * Returns the icon that has been assigned to the dock widget
     */
    QIcon icon() const;

    /**
     * This function returns the dock widget top tool bar.
     * If no toolbar is assigned, this function returns nullptr. To get a valid
     * toolbar you either need to create a default empty toolbar via
     * createDefaultToolBar() function or you need to assign your custom
     * toolbar via setToolBar().
     */
    QToolBar *toolBar() const;

    /**
     * If you would like to use the default top tool bar, then call this
     * function to create the default tool bar.
     * After this function the toolBar() function will return a valid toolBar()
     * object.
     */
    QToolBar *createDefaultToolBar();

    /**
     * Assign a new tool bar that is shown above the content widget.
     * The dock widget will become the owner of the tool bar and deletes it
     * on destruction
     */
    void setToolBar(QToolBar *toolBar);

    /**
     * This function sets the tool button style for the given dock widget state.
     * It is possible to switch the tool button style depending on the state.
     * If a dock widget is floating, then here are more space and it is
     * possible to select a style that requires more space like
     * Qt::ToolButtonTextUnderIcon. For the docked state Qt::ToolButtonIconOnly
     * might be better.
     */
    void setToolBarStyle(Qt::ToolButtonStyle style, eState state);

    /**
     * Returns the tool button style for the given docking state.
     * \see setToolBarStyle()
     */
    Qt::ToolButtonStyle toolBarStyle(eState state) const;

    /**
     * This function sets the tool button icon size for the given state.
     * If a dock widget is floating, there is more space an increasing the
     * icon size is possible. For docked widgets, small icon sizes, eg. 16 x 16
     * might be better.
     */
    void setToolBarIconSize(const QSize &iconSize, eState state);

    /**
     * Returns the icon size for a given docking state.
     * \see setToolBarIconSize()
     */
    QSize toolBarIconSize(eState state) const;

    /**
     * Set the actions that will be shown in the dock area title bar
     * if this dock widget is the active tab.
     * You should not add to many actions to the title bar, because this
     * will remove the available space for the tabs. If you have a number
     * of actions, just add an action with a menu to show a popup menu
     * button in the title bar.
     */
    void setTitleBarActions(QList<QAction *> actions);

    /**
     * Returns a list of actions that will be inserted into the dock area title
     * bar if this dock widget becomes the current widget
     */
    virtual QList<QAction *> titleBarActions() const;

#ifndef QT_NO_TOOLTIP
    /**
     * This is function sets text tooltip for title bar widget
     * and tooltip for toggle view action
     */
    void setTabToolTip(const QString &text);
#endif

    /**
     * Returns true if the dock widget is floating and if the floating dock
     * container is full screen
     */
    bool isFullScreen() const;

    /**
     * Returns true if this dock widget is in a dock area, that contains at
     * least 2 opened dock widgets
     */
    bool isTabbed() const;

    /**
     * Returns true if this dock widget is the current one in the dock
     * area widget that contains it.
     * If the dock widget is the only opened dock widget in a dock area,
     * the true is returned
     */
    bool isCurrentTab() const;

public: // reimplements QFrame
    /**
     * Emits titleChanged signal if title change event occurs
     */
    bool event(QEvent *event) override;

    /**
     * This property controls whether the dock widget is open or closed.
     * The toogleViewAction triggers this slot
     */
    void toggleView(bool open = true);

    /**
     * Makes this dock widget the current tab in its dock area.
     * The function only has an effect, if the dock widget is open. A call
     * to this function will not toggle the view, so if it is closed,
     * nothing will happen
     */
    void setAsCurrentTab();

    /**
     * Brings the dock widget to the front
     * This means:
     * - If the dock widget is tabbed with other dock widgets but its tab is not current, it's made current.
     * - If the dock widget is floating, QWindow::raise() is called.
     * This only applies if the dock widget is already open. If closed, does nothing.
     */
    void raise();

    /**
     * This function will make a docked widget floating
     */
    void setFloating();

    /**
     * This function will delete the dock widget and its content from the
     * docking system
     */
    void deleteDockWidget();

    /**
     * Closes the dock widget
     */
    void closeDockWidget();

    /**
     * Shows the widget in full-screen mode.
     * Normally this function only affects windows. To make the interface
     * compatible to QDockWidget, this function also maximizes a floating
     * dock widget.
     *
     * \note Full-screen mode works fine under Windows, but has certain
     * problems (doe not work) under X (Linux). These problems are due to
     * limitations of the ICCCM protocol that specifies the communication
     * between X11 clients and the window manager. ICCCM simply does not
     * understand the concept of non-decorated full-screen windows.
     */
    void showFullScreen();

    /**
     * This function complements showFullScreen() to restore the widget
     * after it has been in full screen mode.
     */
    void showNormal();

signals:
    /**
     * This signal is emitted if the dock widget is opened or closed
     */
    void viewToggled(bool open);

    /**
     * This signal is emitted if the dock widget is closed
     */
    void closed();

    /**
     * This signal is emitted if the window title of this dock widget
     * changed
      */
    void titleChanged(const QString &title);

    /**
     * This signal is emitted when the floating property changes.
     * The topLevel parameter is true if the dock widget is now floating;
     * otherwise it is false.
     */
    void topLevelChanged(bool topLevel);

    /**
     * This signal is emitted, if close is requested
     */
    void closeRequested();

    /**
     * This signal is emitted when the dock widget becomes visible (or invisible).
     * This happens when the widget is hidden or shown, as well as when it is
     * docked in a tabbed dock area and its tab becomes selected or unselected.
     */
    void visibilityChanged(bool visible);

    /**
     * This signal is emitted when the features property changes.
     * The features parameter gives the new value of the property.
     */
    void featuresChanged(DockWidgetFeatures features);
}; // class DockWidget

} // namespace ADS
