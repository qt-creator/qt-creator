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

import QtQuick 1.0
import qtcomponents 1.0 as Components

Item {
    id: exampleBrowserRoot
    function appendTag(tag) {
        var tagStr = "tag:" + '"' + tag + '"'
        if (lineEdit.text == "")
            lineEdit.text = tagStr
        else
            lineEdit.text += " " + tagStr
    }


    Rectangle {
        id : lineEditRoot
        color:"#f4f4f4"
        width: parent.width
        height: lineEdit.height + 6
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottomMargin: - 8
        anchors.leftMargin: -8
        anchors.rightMargin: -8


        CheckBox {
            id: checkBox
            text: qsTr("Show Examples and Demos")
            checked: false
            anchors.left: parent.left
            anchors.leftMargin: 6
            anchors.verticalCenter: lineEdit.verticalCenter
            height: lineEdit.height
            onCheckedChanged: examplesModel.showTutorialsOnly = !checked;
        }

        LineEdit {
            id: lineEdit
            placeholderText: !checkBox.checked ? qsTr("Search in Tutorials") : qsTr("Search in Tutorials, Examples and Demos")
            focus: true
            anchors.left: checkBox.right
            anchors.leftMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            width: Math.max(lineEditRoot.width - checkBox.width - 21 - tagFilterButton.width, 100)
            onTextChanged: examplesModel.parseSearchString(text)
        }

        Button {
            id: tagFilterButton
            property string tag
            property Item browser;
            onTagChanged: exampleBrowserRoot.appendTag(tag)
            anchors.left: lineEdit.right
            anchors.leftMargin: 6
            anchors.verticalCenter: lineEdit.verticalCenter
            text: qsTr("Tag List")
            checkable: true
            Connections {
                target: tagBrowserLoader.item
                onVisibleChanged: tagFilterButton.checked = tagBrowserLoader.item.visible
            }

            onCheckedChanged: {
                if (checked) {
                    tagBrowserLoader.source = "TagBrowser.qml"
                    var item = tagBrowserLoader.item;
                    item.bottomMargin = lineEditRoot.height
                    item.visible = true
                } else { tagBrowserLoader.item.visible = false }
            }
        }
    }
    Components.ScrollArea  {
        id: scrollArea
        anchors.bottomMargin: lineEditRoot.height - 8
        anchors.margins:-8
        anchors.fill: parent
        frame: false
        Column {
            Repeater {
                id: repeater
                model: examplesModel
                delegate: ExampleDelegate { width: scrollArea.width; onTagClicked: exampleBrowserRoot.appendTag(tag) }
            }
        }
    }

    Rectangle {
        anchors.bottom: scrollArea.bottom
        height:4
        anchors.left: scrollArea.left
        anchors.right: scrollArea.right
        anchors.rightMargin: scrollArea.verticalScrollBar.visible ?
                               scrollArea.verticalScrollBar.width : 0
        width:parent.width
        gradient: Gradient{
            GradientStop{position:1 ; color:"#10000000"}
            GradientStop{position:0 ; color:"#00000000"}
        }
        Rectangle{
            height:1
            color:"#ccc"
            anchors.bottom: parent.bottom
            width:parent.width
        }
    }


    Loader {
        id: tagBrowserLoader
        anchors.fill: parent
    }
}
