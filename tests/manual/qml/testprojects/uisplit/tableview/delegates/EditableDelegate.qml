// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Window 2.1
import QtQuick.Controls 1.2

Component {
    Item {
        Text {
            width: parent.width
            anchors.margins: 4
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            elide: styleData.elideMode
            text: styleData.value !== undefined ? styleData.value : ""
            color: styleData.textColor
            visible: !styleData.selected
        }

        Loader { // Initialize text editor lazily to improve performance
            id: loaderEditor
            anchors.fill: parent
            anchors.margins: 4
            Connections {
                target: loaderEditor.item
                onAccepted: {
                    if (typeof styleData.value === 'number')
                        largeModel.setProperty(styleData.row, styleData.role, Number(parseFloat(loaderEditor.item.text).toFixed(0)))
                    else
                        largeModel.setProperty(styleData.row, styleData.role, loaderEditor.item.text)
                }
            }
            sourceComponent: styleData.selected ? editor : null
            Component {
                id: editor
                TextInput {
                    id: textinput
                    color: styleData.textColor
                    text: styleData.value
                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: textinput.forceActiveFocus()
                    }
                }
            }
        }
    }
}
