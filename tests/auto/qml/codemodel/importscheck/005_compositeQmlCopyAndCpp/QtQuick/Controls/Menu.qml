// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Styles 1.1
import QtQuick.Controls.Private 1.0

/*!
    \qmltype Menu
    \inqmlmodule QtQuick.Controls
    \since 5.1
    \ingroup menus
    \brief Provides a menu component for use in menu bars, as context menu,
    and other popup menus.

    \code
    Menu {
        title: "Edit"

        MenuItem {
            text: "Cut"
            shortcut: "Ctrl+X"
            onTriggered: ...
        }

        MenuItem {
            text: "Copy"
            shortcut: "Ctrl+C"
            onTriggered: ...
        }

        MenuItem {
            text: "Paste"
            shortcut: "Ctrl+V"
            onTriggered: ...
        }

        MenuSeparator { }

        Menu {
            title: "More Stuff"

            MenuItem {
                text: "Do Nothing"
            }
        }
    }
    \endcode

    The main uses for menus:
    \list
    \li
       as a \e top-level menu in a \l MenuBar
    \li
       as a \e submenu inside another menu
    \li
       as a standalone or \e context menu
    \endlist

    Note that some properties, such as \c enabled, \c text, or \c iconSource,
    only make sense in a particular use case of the menu.

    \sa MenuBar, MenuItem, MenuSeparator
*/

MenuPrivate {
    id: root

    /*! \internal
      \omit
      Documented in qqquickmenu.cpp.
      \endomit
    */
    function addMenu(title) {
        return root.insertMenu(items.length, title)
    }

    /*! \internal
      \omit
      Documented in qquickmenu.cpp.
      \endomit
    */
    function insertMenu(index, title) {
        if (!__selfComponent)
            __selfComponent = Qt.createComponent("Menu.qml", root)
        var submenu = __selfComponent.createObject(__selfComponent, { "title": title })
        root.insertItem(index, submenu)
        return submenu
    }

    /*! \internal */
    property Component __selfComponent: null

    /*! \internal */
    property Component style: Qt.createComponent(Settings.style + "/MenuStyle.qml", root)

    /*! \internal */
    property var __parentContentItem: __parentMenu.__contentItem
    /*! \internal */
    property int __currentIndex: -1
    /*! \internal */
    on__MenuClosed: __currentIndex = -1

    /*! \internal */
    __contentItem: Loader {
        sourceComponent: MenuContentItem {
            menu: root
        }
        active: !root.__isNative && root.__popupVisible
        focus: true
        Keys.forwardTo: item ? [item, root.__parentContentItem] : []
        property bool altPressed: root.__parentContentItem ? root.__parentContentItem.altPressed : false
    }
}
