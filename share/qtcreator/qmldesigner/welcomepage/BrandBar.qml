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

import QtQuick
import QtQuick.Controls
import WelcomeScreen 1.0
import QtQuick.Layouts 1.0

Item {
    id: brandBar
    width: 850
    height: 150
    state: "brandQds"

    Image {
        id: brandIcon
        width: 100
        height: 100
        anchors.verticalCenter: parent.verticalCenter
        source: "images/place_holder.png"
        fillMode: Image.PreserveAspectFit
    }

    Text {
        id: welcomeTool
        color: Constants.currentGlobalText
        text: qsTr("Welcome to")
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: brandIcon.right
        font.pixelSize: 36
        verticalAlignment: Text.AlignVCenter
        anchors.leftMargin: 5
        font.family: "titillium web"
    }

    Text {
        id: brandLabel
        color: Constants.currentBrand
        text: qsTr("Qt Design Studio")
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: welcomeTool.right
        font.pixelSize: 36
        verticalAlignment: Text.AlignVCenter
        anchors.leftMargin: 8
        font.family: "titillium web"
    }

    Text {
        id: communityEdition
        width: 291
        height: 55
        visible: Constants.communityEdition
        color: Constants.currentGlobalText
        text: qsTr("Community Edition")
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: brandLabel.right
        font.pixelSize: 36
        verticalAlignment: Text.AlignVCenter
        anchors.leftMargin: 8
        font.family: "titillium web"
    }
    states: [
        State {
            name: "brandQds"
            when: Constants.defaultBrand

            PropertyChanges {
                target: brandIcon
                source: "images/ds.png"
            }
        },
        State {
            name: "brandCreator"
            when: !Constants.defaultBrand

            PropertyChanges {
                target: brandLabel
                text: qsTr("Qt Creator")
            }

            PropertyChanges {
                target: brandIcon
                source: "images/qc-1.png"
            }
        }
    ]
}

/*##^##
Designer {
    D{i:0;height:150;width:719}D{i:4}
}
##^##*/
