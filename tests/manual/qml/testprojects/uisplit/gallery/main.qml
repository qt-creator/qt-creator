/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick 2.3
import QtQuick.Controls 1.3
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.0
import "content"

ApplicationWindow {
    visible: true
    title: "Component Gallery"

    width: 640
    height: 420
    minimumHeight: 400
    minimumWidth: 600

    property string loremIpsum:
            "Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor "+
            "Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor "+
            "incididunt ut labore et dolore magna aliqua.\n Ut enim ad minim veniam, quis nostrud "+
            "exercitation ullamco laboris nisi ut aliquip ex ea commodo cosnsequat. ";

    ImageViewer { id: imageViewer }

    FileDialog {
        id: fileDialog
        nameFilters: [ "Image files (*.png *.jpg)" ]
        onAccepted: imageViewer.open(fileUrl)
    }

    AboutDialog { id: aboutDialog }

    Action {
        id: openAction
        text: "&Open"
        shortcut: StandardKey.Open
        iconSource: "images/document-open.png"
        onTriggered: fileDialog.open()
        tooltip: "Open an image"
    }

    Action {
        id: copyAction
        text: "&Copy"
        shortcut: StandardKey.Copy
        iconName: "edit-copy"
        enabled: (!!activeFocusItem && !!activeFocusItem["copy"])
        onTriggered: activeFocusItem.copy()
    }

    Action {
        id: cutAction
        text: "Cu&t"
        shortcut: StandardKey.Cut
        iconName: "edit-cut"
        enabled: (!!activeFocusItem && !!activeFocusItem["cut"])
        onTriggered: activeFocusItem.cut()
    }

    Action {
        id: pasteAction
        text: "&Paste"
        shortcut: StandardKey.Paste
        iconName: "edit-paste"
        enabled: (!!activeFocusItem && !!activeFocusItem["paste"])
        onTriggered: activeFocusItem.paste()
    }

    Action {
        id: aboutAction
        text: "About"
        onTriggered: aboutDialog.open()
    }

    ExclusiveGroup {
        id: textFormatGroup

        Action {
            id: a1
            text: "Align &Left"
            checkable: true
            Component.onCompleted: checked = true
        }

        Action {
            id: a2
            text: "&Center"
            checkable: true
        }

        Action {
            id: a3
            text: "Align &Right"
            checkable: true
        }
    }

    ChildWindow { id: window1 }

    Component {
        id: editmenu
        Menu {
            MenuItem { action: cutAction }
            MenuItem { action: copyAction }
            MenuItem { action: pasteAction }
            MenuSeparator {}
            Menu {
                title: "Text &Format"
                MenuItem { action: a1 }
                MenuItem { action: a2 }
                MenuItem { action: a3 }
                MenuSeparator { }
                MenuItem { text: "Allow &Hyphenation"; checkable: true }
            }
            Menu {
                title: "Font &Style"
                MenuItem { text: "&Bold"; checkable: true }
                MenuItem { text: "&Italic"; checkable: true }
                MenuItem { text: "&Underline"; checkable: true }
            }
        }
    }

    toolBar: ToolBar {
        id: toolbar
        RowLayout {
            id: toolbarLayout
            spacing: 0
            anchors.fill: parent
            ToolButton {
                iconSource: "images/window-new.png"
                onClicked: window1.visible = !window1.visible
                Accessible.name: "New window"
                tooltip: "Toggle visibility of the second window"
            }
            ToolButton { action: openAction }
            ToolButton {
                Accessible.name: "Save as"
                iconSource: "images/document-save-as.png"
                tooltip: "(Pretend to) Save as..."
            }
            Item { Layout.fillWidth: true }
            CheckBox {
                id: enabledCheck
                text: "Enabled"
                checked: true
            }
        }
    }

    menuBar: MenuBar {
        Menu {
            title: "&File"
            MenuItem { action: openAction }
            MenuItem {
                text: "Close"
                shortcut: StandardKey.Quit
                onTriggered: Qt.quit()
            }
        }
        Menu {
            title: "&Edit"
            MenuItem { action: cutAction }
            MenuItem { action: copyAction }
            MenuItem { action: pasteAction }
            MenuSeparator { }
            MenuItem {
                text: "Do Nothing"
                shortcut: "Ctrl+E,Shift+Ctrl+X"
                enabled: false
            }
            MenuItem {
                text: "Not Even There"
                shortcut: "Ctrl+E,Shift+Ctrl+Y"
                visible: false
            }
            Menu {
                title: "Me Neither"
                visible: false
            }
        }
        Menu {
            title: "&Help"
            MenuItem { action: aboutAction }
        }
    }


    SystemPalette {id: syspal}
    color: syspal.window
    ListModel {
        id: choices
        ListElement { text: "Banana" }
        ListElement { text: "Orange" }
        ListElement { text: "Apple" }
        ListElement { text: "Coconut" }
    }

    MainTabView {
        id: frame

        enabled: enabledCheck.checked
        anchors.fill: parent
        anchors.margins: Qt.platform.os === "osx" ? 12 : 2
    }
}

