/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuickDesignerTheme 1.0

Column {
    id: root

    Text {
        id: header
        text: qsTr("Select a Module to Add")
        color: "#ffffff"
        font.pixelSize: 16
        width: parent.width
        height: 50
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    ScrollView { // ListView not used because of QTBUG-52941
        id: listView
        width: parent.width
        height: parent.height - header.height
        clip: true

        Column {
            spacing: 2

            Repeater {
                model: addImportModel

                delegate: Rectangle {
                    width: listView.width
                    height: isSeparator ? 4 : 25
                    color: isSeparator ? Theme.color(Theme.BackgroundColorNormal)
                                       : mouseArea.containsMouse
                                         ? Qt.lighter(Theme.qmlDesignerButtonColor(), 1.3)
                                         : Theme.qmlDesignerButtonColor()
                    visible: importVisible

                    Text {
                        text: importUrl
                        color: Theme.color(Theme.PanelTextColorLight)
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                    }

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: rootView.handleAddImport(index)
                        enabled: !isSeparator
                    }
                }
            }
        }
    }
}

