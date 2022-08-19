// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick.Window 2.1
import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.0
import QtQuick.Controls.Private 1.0

/*!
    \qmltype ApplicationWindow
    \since 5.1
    \inqmlmodule QtQuick.Controls
    \ingroup applicationwindow
    \brief Provides a top-level application window.

    ApplicationWindow is a \l Window that adds convenience for positioning items,
    such as \l MenuBar, \l ToolBar, and \l StatusBar in a platform independent
    manner.

    \code
    ApplicationWindow {
        id: window
        menuBar: MenuBar {
            Menu { MenuItem {...} }
            Menu { MenuItem {...} }
        }

        toolBar: ToolBar {
            RowLayout {
                anchors.fill: parent
                ToolButton {...}
            }
        }

        TabView {
            id: myContent
            anchors.fill: parent
            ...
        }
    }
    \endcode
*/

Window {
    id: root

    /*!
        \qmlproperty MenuBar ApplicationWindow::menuBar

        This property holds the \l MenuBar.

        By default, this value is not set.
    */
    property MenuBar menuBar: null

    /*!
        \qmlproperty Item ApplicationWindow::toolBar

        This property holds the toolbar \l Item.

        It can be set to any Item type, but is generally used with \l ToolBar.

        By default, this value is not set. When you set the toolbar item, it will
        be anchored automatically into the application window.
    */
    property Item toolBar

    /*!
        \qmlproperty Item ApplicationWindow::statusBar

        This property holds the status bar \l Item.

        It can be set to any Item type, but is generally used with \l StatusBar.

        By default, this value is not set. When you set the status bar item, it
        will be anchored automatically into the application window.
    */
    property Item statusBar

    onToolBarChanged: { if (toolBar) { toolBar.parent = toolBarArea } }

    onStatusBarChanged: { if (statusBar) { statusBar.parent = statusBarArea } }

    onVisibleChanged: { if (visible && menuBar) { menuBar.__parentWindow = root } }

    /*! \internal */
    default property alias data: contentArea.data

    color: syspal.window

    flags: Qt.Window | Qt.WindowFullscreenButtonHint |
        Qt.WindowTitleHint | Qt.WindowSystemMenuHint | Qt.WindowMinMaxButtonsHint |
        Qt.WindowCloseButtonHint | Qt.WindowFullscreenButtonHint
    // QTBUG-35049: Windows is removing features we didn't ask for, even though Qt::CustomizeWindowHint is not set
    // Otherwise Qt.Window | Qt.WindowFullscreenButtonHint would be enough

    SystemPalette {id: syspal}

    Item {
        id: backgroundItem
        anchors.fill: parent

        Keys.forwardTo: menuBar ? [menuBar.__contentItem] : []

        Item {
            id: contentArea
            anchors.top: toolBarArea.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: statusBarArea.top
        }

        Item {
            id: toolBarArea
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            implicitHeight: childrenRect.height
            height: visibleChildren.length > 0 ? implicitHeight: 0
        }

        Item {
            id: statusBarArea
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            implicitHeight: childrenRect.height
            height: visibleChildren.length > 0 ? implicitHeight: 0
        }

        onVisibleChanged: if (visible && menuBar) menuBar.__parentWindow = root

        states: State {
            name: "hasMenuBar"
            when: menuBar && !menuBar.__isNative

            ParentChange {
                target: menuBar.__contentItem
                parent: backgroundItem
            }

            PropertyChanges {
                target: menuBar.__contentItem
                x: 0
                y: 0
                width: backgroundItem.width
            }

            AnchorChanges {
                target: toolBarArea
                anchors.top: menuBar.__contentItem.bottom
            }
        }
    }
}
