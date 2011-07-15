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
import components 1.0 as Components

Item {
    id: exampleBrowserRoot
    Item {
        id : lineEditRoot
        width: parent.width
        height: lineEdit.height

        LineEdit {
            Behavior on width { NumberAnimation{} }
            placeholderText: !checkBox.checked ? qsTr("Search in Tutorials") : qsTr("Search in Tutorials, Examples and Demos")
            focus: true
            id: lineEdit
            width: lineEditRoot.width - checkBox.width - 20 - tagFilterButton.width
            onTextChanged: examplesModel.filterRegExp = RegExp('.*'+text, "im")
        }

        CheckBox {
            id: checkBox
            text: qsTr("Show Examples and Demos")
            checked: false
            anchors.leftMargin: 6
            anchors.left: lineEdit.right
            anchors.verticalCenter: lineEdit.verticalCenter
            height: lineEdit.height
            onCheckedChanged: examplesModel.showTutorialsOnly = !checked;
        }

        Button {
            id: tagFilterButton
            property string tag
            Behavior on width { NumberAnimation{} }
            onTagChanged: { examplesModel.filterTag = tag; examplesModel.updateFilter() }
            anchors.leftMargin: 6
            anchors.left: checkBox.right
            anchors.verticalCenter: lineEdit.verticalCenter
            visible: !examplesModel.showTutorialsOnly
            text: tag === "" ? qsTr("Filter by Tag") : qsTr("Tag Filter: %1").arg(tag)
            onClicked: {
                tagBrowserLoader.source = "TagBrowser.qml"
                tagBrowserLoader.item.visible = true
            }
        }
    }
    Components.ScrollArea  {
        id: scrollArea
        anchors.topMargin: lineEditRoot.height+12
        anchors.fill: parent
        clip: true
        frame: false
        Column {
            Repeater {
                id: repeater
                model: examplesModel
                delegate: ExampleDelegate {
                    width: scrollArea.width-20;
                    property int count: repeater.count
                }
            }
        }
    }

    Loader {
        id: tagBrowserLoader
        anchors.fill: parent
    }
}
