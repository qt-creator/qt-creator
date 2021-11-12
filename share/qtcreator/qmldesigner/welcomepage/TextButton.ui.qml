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
import WelcomeScreen 1.0

Item {
    id: textButton
    width: 41
    height: 22
    property alias textButtonLabelText: textButtonLabel.text
    state: "normal"

    Text {
        id: textButtonLabel
        color: Constants.currentBrand
        text: qsTr("Clone")
        anchors.fill: parent
        font.pixelSize: 12
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
    }
    states: [
        State {
            name: "normal"
            when: !mouseArea.containsMouse

            PropertyChanges {
                target: textButtonLabel
                color: Constants.currentGlobalText
            }
        },
        State {
            name: "hover"
            when: mouseArea.containsMouse
        }
    ]
}

/*##^##
Designer {
    D{i:0;height:22;width:41}D{i:1}D{i:2}
}
##^##*/

