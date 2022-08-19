// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Styles 1.1
import QtQuick.Controls.Private 1.0

/*!
    \qmltype MenuBar
    \inqmlmodule QtQuick.Controls
    \since 5.1
    \ingroup applicationwindow
    \brief Provides a horizontal menu bar.

    \code
    MenuBar {
        Menu {
            title: "File"
            MenuItem { text: "Open..." }
            MenuItem { text: "Close" }
        }

        Menu {
            title: "Edit"
            MenuItem { text: "Cut" }
            MenuItem { text: "Copy" }
            MenuItem { text: "Paste" }
        }
    }
    \endcode

    \sa ApplicationWindow::menuBar
*/

MenuBarPrivate {
    id: root

    /*! \internal */
    property Component style: Qt.createComponent(Settings.style + "/MenuBarStyle.qml", root)

    /*! \internal */
    __contentItem: Loader {
        id: topLoader
        sourceComponent: __menuBarComponent
        active: !root.__isNative
        focus: true
        Keys.forwardTo: [item]
        property bool altPressed: item ? item.altPressed : false
    }

    /*! \internal */
    property Component __menuBarComponent: Loader {
        id: menuBarLoader

        property Style __style: styleLoader.item
        property Component menuItemStyle: __style ? __style.menuItem : null

        property var control: root
        onStatusChanged: if (status === Loader.Error) console.error("Failed to load panel for", root)

        visible: status === Loader.Ready
        sourceComponent: __style ? __style.frame : undefined

        Loader {
            id: styleLoader
            property alias __control: menuBarLoader
            sourceComponent: root.style
            onStatusChanged: {
                if (status === Loader.Error)
                    console.error("Failed to load Style for", root)
            }
        }

        property int openedMenuIndex: -1
        property bool preselectMenuItem: false
        property alias contentHeight: row.height

        Binding {
            // Make sure the styled menu bar is in the background
            target: menuBarLoader.item
            property: "z"
            value: menuMouseArea.z - 1
        }

        focus: true

        property bool altPressed: false
        property bool altPressedAgain: false
        property var mnemonicsMap: ({})

        Keys.onPressed: {
            var action = null
            if (event.key === Qt.Key_Alt) {
                if (!altPressed)
                    altPressed = true
                else
                    altPressedAgain = true
            } else if (altPressed && (action = mnemonicsMap[event.text.toUpperCase()])) {
                preselectMenuItem = true
                action.trigger()
                event.accepted = true
            }
        }

        function dismissActiveFocus(event, reason) {
            if (reason) {
                altPressedAgain = false
                altPressed = false
                openedMenuIndex = -1
                root.__contentItem.parent.forceActiveFocus()
            } else {
                event.accepted = false
            }
        }

        Keys.onReleased: dismissActiveFocus(event, altPressedAgain && openedMenuIndex === -1)
        Keys.onEscapePressed: dismissActiveFocus(event, openedMenuIndex === -1)

        function maybeOpenFirstMenu(event) {
            if (altPressed && openedMenuIndex === -1) {
                preselectMenuItem = true
                openedMenuIndex = 0
            } else {
                event.accepted = false
            }
        }

        Keys.onUpPressed: maybeOpenFirstMenu(event)
        Keys.onDownPressed: maybeOpenFirstMenu(event)

        Keys.onLeftPressed: {
            if (openedMenuIndex > 0) {
                preselectMenuItem = true
                openedMenuIndex--
            }
        }

        Keys.onRightPressed: {
            if (openedMenuIndex !== -1 && openedMenuIndex < root.menus.length - 1) {
                preselectMenuItem = true
                openedMenuIndex++
            }
        }

        MouseArea {
            id: menuMouseArea
            anchors.fill: parent
            hoverEnabled: true

            onPositionChanged: updateCurrentItem(mouse, false)
            onPressed: {
                if (updateCurrentItem(mouse)) {
                    menuBarLoader.preselectMenuItem = false
                    menuBarLoader.openedMenuIndex = currentItem.menuItemIndex
                }
            }
            onExited: hoveredItem = null

            property Item currentItem: null
            property Item hoveredItem: null
            function updateCurrentItem(mouse) {
                var pos = mapToItem(row, mouse.x, mouse.y)
                if (!hoveredItem || !hoveredItem.contains(Qt.point(pos.x - currentItem.x, pos.y - currentItem.y))) {
                    hoveredItem = row.childAt(pos.x, pos.y)
                    if (!hoveredItem)
                        return false;
                    currentItem = hoveredItem
                    if (menuBarLoader.openedMenuIndex !== -1) {
                        menuBarLoader.preselectMenuItem = false
                        menuBarLoader.openedMenuIndex = currentItem.menuItemIndex
                    }
                }
                return true;
            }

            Row {
                id: row
                width: parent.width
                LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft

                Repeater {
                    id: itemsRepeater
                    model: root.menus
                    Loader {
                        id: menuItemLoader

                        property var menuItem: modelData
                        property bool selected: menuMouseArea.hoveredItem === menuItemLoader
                        property bool sunken: menuItem.__popupVisible || menuBarLoader.openedMenuIndex === index
                        property bool showUnderlined: menuBarLoader.altPressed

                        sourceComponent: menuBarLoader.menuItemStyle
                        property int menuItemIndex: index
                        visible: menuItem.visible

                        Connections {
                            target: menuBarLoader
                            onOpenedMenuIndexChanged: {
                                if (menuBarLoader.openedMenuIndex === index) {
                                    if (row.LayoutMirroring.enabled)
                                        menuItem.__popup(menuItemLoader.width, menuBarLoader.height, 0)
                                    else
                                        menuItem.__popup(0, menuBarLoader.height, 0)
                                    if (menuBarLoader.preselectMenuItem)
                                        menuItem.__currentIndex = 0
                                } else {
                                    menuItem.__closeMenu()
                                }
                            }
                        }

                        Connections {
                            target: menuItem
                            onPopupVisibleChanged: {
                                if (!menuItem.__popupVisible && menuBarLoader.openedMenuIndex === index)
                                    menuBarLoader.openedMenuIndex = -1
                            }
                        }

                        Connections {
                            target: menuItem.__action
                            onTriggered: menuBarLoader.openedMenuIndex = menuItemIndex
                        }

                        Component.onCompleted: {
                            menuItem.__visualItem = menuItemLoader

                            var title = menuItem.title
                            var ampersandPos = title.indexOf("&")
                            if (ampersandPos !== -1)
                                menuBarLoader.mnemonicsMap[title[ampersandPos + 1].toUpperCase()] = menuItem.__action
                        }
                    }
                }
            }
        }
    }
}
