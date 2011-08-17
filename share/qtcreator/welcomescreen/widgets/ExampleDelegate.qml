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

Rectangle {
    property int textItemsCombinedHeight: title.paintedHeight + description.paintedHeight + tagLine.height
    id: root
    height: textItemsCombinedHeight + 30
    color: "#00ffffff"
    property variant tags : model.tags
    signal tagClicked(string tag)

    Components.QStyleItem { cursor: "pointinghandcursor" ; anchors.fill: parent }

    Item {
        visible: parent.state=="hover"
        anchors.fill: parent
        Rectangle{
            height: 1
            color: "#eee"
            anchors.top: parent.top
            width:parent.width
        }
        Rectangle {
            height: 1
            color: "#eee"
            anchors.bottom: parent.bottom
            width:parent.width
        }
    }

    Text {
        id: title
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.right: imageWrapper.left
        anchors.rightMargin: 10
        anchors.top: parent.top
        anchors.topMargin: 10
        text: model.name
        font.bold: true
        font.pixelSize: 14
        elide: Text.ElideRight

    }
    RatingBar { id: rating; anchors.top: parent.top; anchors.topMargin: 10; anchors.right: parent.right; anchors.rightMargin: 10; rating: model.difficulty; visible: model.difficulty !== 0 }

    Item {
        id: imageWrapper
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: 90
        anchors.rightMargin: 30
        Image {
            property bool hideImage : model.imageUrl === "" || status === Image.Error
            id: image
            smooth: true
            anchors.centerIn: parent
            width: parent.width
            height: textItemsCombinedHeight
            asynchronous: true
            fillMode: Image.PreserveAspectFit
            source: model.imageUrl !== "" ? "image://helpimage/" + encodeURI(model.imageUrl) : ""
        }
    }

    Text {
        id: description
        anchors.left: parent.left
        anchors.right: imageWrapper.left
        anchors.leftMargin: 10
        anchors.top: rating.bottom
        wrapMode: Text.WordWrap
        text: model.description
        color:"#444"
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: {
            if (model.hasSourceCode)
                gettingStarted.openProject(model.projectPath, model.filesToOpen, model.docUrl)
            else
                gettingStarted.openSplitHelp(model.docUrl);
        }
        onEntered: parent.state = "hover"
        onExited: parent.state = ""
    }

    Flow {
        id: tagLine;
        anchors.topMargin: 5
        anchors.bottomMargin: 20
        anchors.top: description.bottom
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.rightMargin: 26
        anchors.right: imageWrapper.left
        spacing: 4
        Text { id: labelText; text: qsTr("Tags:") ; color: "#999"; font.pixelSize: 11}
        Repeater {
            model: tags;
            Text {
                states: [ State { name: "hover"; PropertyChanges { target: tagText; color: "black" } } ]
                id: tagText
                text: model.modelData
                color: "#bbb"
                font.pixelSize: 11
                MouseArea {
                    anchors.fill: parent;
                    hoverEnabled: true;
                    onEntered: {
                        root.state = "hover"
                        parent.state = "hover"
                    }
                    onExited:{
                        root.state = ""
                        parent.state = ""
                    }
                    onClicked: root.tagClicked(model.modelData)
                }
            }
        }
    }

    states: [ State { name: "hover"; PropertyChanges { target: root; color: "#f9f9f9" } } ]
}
