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
