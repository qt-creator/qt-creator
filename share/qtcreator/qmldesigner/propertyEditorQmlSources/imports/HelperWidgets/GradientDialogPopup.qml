/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

import QtQuick 2.1
import QtQuick.Layouts 1.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme
import QtQuickDesignerTheme 1.0

Loader {
    id: gradientDialogLoader
    parent: itemPane
    anchors.fill: parent

    visible: false
    active: visible

    function toggle() {
        gradientDialogLoader.visible = !gradientDialogLoader.visible
    }

    property Component content

    property int dialogHeight: 240
    property int dialogWidth: 440

    sourceComponent: Component {
        FocusScope {
            id: popup

            Keys.onEscapePressed: {
                event.accepted = true
                gradientDialogLoader.visible = false
            }

            Component.onCompleted: {
                popup.forceActiveFocus()
            }

            Rectangle {
                anchors.fill: parent
                color: Theme.qmlDesignerBackgroundColorDarker()
                opacity: 0.6
            }

            MouseArea {
                anchors.fill: parent
                onClicked: gradientDialogLoader.visible = false
                preventStealing: true
                hoverEnabled: true
            }
            Rectangle {
                id: background

                property int xOffset: itemPane.width - gradientDialogLoader.dialogWidth
                x: 4 + xOffset
                Component.onCompleted: {
                    var pos = itemPane.mapFromItem(buttonRow.parent, 0, 0)
                    y = pos.y + 32
                }

                width: parent.width - 8 - xOffset
                height: gradientDialogLoader.dialogHeight

                color: Theme.qmlDesignerBackgroundColorDarkAlternate()
                border.color: Theme.qmlDesignerBorderColor()

                Label {
                    x: 8
                    y: 6
                    font.bold: true
                    text: qsTr("Gradient Properties")
                }

                StudioControls.AbstractButton {
                    width: 16
                    height: 16
                    buttonIcon: StudioTheme.Constants.closeCross
                    onClicked: gradientDialogLoader.visible = false
                    backgroundRadius: 2
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 4
                }

                Loader {
                    anchors.fill: parent
                    sourceComponent: gradientDialogLoader.content
                }
            }
        }
    }
}
