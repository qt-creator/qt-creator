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

Rectangle {
    property int bottomMargin : 0
    id: tagChooser
    anchors.fill: parent
    anchors.bottomMargin: bottomMargin
    color: "darkgrey"
    opacity: 0.95
    radius: 6
    visible: false

    property bool needsUpdate: true;

    Connections {
        target: gettingStarted
        onTagsUpdated: needsUpdate = true
    }

    onVisibleChanged: {
        if (visible && needsUpdate) {
            needsUpdate = false;
            tagsModel.clear();
            var tagList = gettingStarted.tagList()
            for (var tag in tagList) {
                tagsModel.append({ "text": tagList[tag], "value": tagList[tag] });
            }
        }
    }

    ListModel {
        id: tagsModel
    }

    MouseArea { anchors.fill: parent; hoverEnabled: true } // disable mouse on background
    Text {
        id: descr;
        anchors.margins: 6;
        color: "white";
        text: qsTr("Please choose a tag to filter for:");
        anchors.top: parent.top;
        anchors.left: parent.left
        font.bold: true
    }

    Flickable {
        id: flickable
        anchors.fill: parent
        anchors.margins: 6
        anchors.topMargin: descr.height + anchors.margins*2
        contentHeight: flow.height
        contentWidth: flow.width
        flickableDirection: Flickable.VerticalFlick
        clip: true
        Flow {
            width: tagChooser.width
            id: flow
            spacing: 6
            Repeater {
                id: tagsList
                model: tagsModel
                delegate: Item {
                    width: btnRect.width
                    height: btnRect.height
                    Rectangle {
                        id: btnRect
                        radius: 4
                        opacity: 0
                        width: text.width+4
                        height: text.height+4
                        x: text.x-2
                        y: text.y-2
                    }
                    Text { id: text; text: model.text; color: "white"; anchors.centerIn: parent }
                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: { tagChooser.visible = false; tagFilterButton.tag = model.value }
                    }

                    states: [
                        State {
                            name: "hovered"
                            when: mouseArea.containsMouse
                            PropertyChanges { target: btnRect; color: "darkblue"; opacity: 0.3 }
                        }
                    ]

                }
            }
        }

    }
}
