// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import WelcomeScreen 1.0

Item {
    id: brandBar
    width: 850
    height: 150

    Image {
        id: brandIcon
        width: 100
        height: 100
        anchors.verticalCenter: parent.verticalCenter
        source: "images/ds.png"
        fillMode: Image.PreserveAspectFit
    }

    Text {
        id: welcomeTo
        color: Constants.currentGlobalText
        text: qsTr("Welcome to")
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: brandIcon.right
        anchors.leftMargin: 5
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: 36
        font.family: "titillium web"
    }

    Text {
        id: brandLabel
        color: Constants.currentBrand
        text: qsTr("Qt Design Studio")
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: welcomeTo.right
        anchors.leftMargin: 8
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: 36
        font.family: "titillium web"
    }

    Text {
        width: 291
        height: 55
        color: Constants.currentGlobalText
        text: {
            if (Constants.communityEdition)
                return qsTr("Community Edition")
            if (Constants.enterpriseEdition)
                 return qsTr("Enterprise Edition")
            return qsTr("Professional Edition")
        }
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: brandLabel.right
        anchors.leftMargin: 8
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: 36
        font.family: "titillium web"
    }
}
