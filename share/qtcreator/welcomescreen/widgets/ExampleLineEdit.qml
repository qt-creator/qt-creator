/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

import QtQuick 1.1
import qtcomponents 1.0 as Components

Rectangle {
    id : exampleLineEdit
    color: "#f2f2f2"
    width: parent.width
    height: lineEdit.height + 22

    property Item tagFilterButton: tagFilterButton
    property Item lineEdit: lineEdit

    Rectangle {
        height: 1
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.bottom
        anchors.topMargin: -1
        color: "black"
    }


    CheckBox {
        id: checkBox
        text: qsTr("Show Examples and Demos")
        checked: false
        anchors.left: parent.left
        anchors.leftMargin: 8
        anchors.verticalCenter: lineEdit.verticalCenter
        height: lineEdit.height
        onCheckedChanged: examplesModel.showTutorialsOnly = !checked;
    }

    LineEdit {
        id: lineEdit
        placeholderText: !checkBox.checked ? qsTr("Search in Tutorials") : qsTr("Search in Tutorials, Examples and Demos")
        focus: true
        anchors.left: checkBox.right
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: tagFilterButton.left
        anchors.rightMargin: 12
        onTextChanged: examplesModel.parseSearchString(text)
    }

    Button {
        id: tagFilterButton
        property string tag
        property Item browser;
        width: 64
        onTagChanged: exampleBrowserRoot.appendTag(tag)
        anchors.verticalCenter: lineEdit.verticalCenter
        text: qsTr("Tag List")
        anchors.right: parent.right
        anchors.rightMargin: 8
        checkable: true
        Connections {
            target: tagBrowserLoader.item
            onVisibleChanged: tagFilterButton.checked = tagBrowserLoader.item.visible
        }

        onCheckedChanged: {
            if (checked) {
                tagBrowserLoader.source = "TagBrowser.qml"
                var item = tagBrowserLoader.item;
                item.bottomMargin = exampleLineEdit.height
                item.visible = true
            } else { tagBrowserLoader.item.visible = false }
        }
    }
}
