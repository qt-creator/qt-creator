// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import LandingPageApi
import LandingPage as Theme

Rectangle {
    id: root

    property bool qdsInstalled: qdsVersionText.text.length > 0
    property bool projectFileExists: false
    property string qtVersion: qsTranslate("QtC::QmlProjectManager", "Unknown")
    property string qdsVersion: qsTranslate("QtC::QmlProjectManager", "Unknown")
    property alias generateProjectFileButton: generateProjectFileButton

    color: Theme.Colors.backgroundSecondary
    height: column.childrenRect.height + (2 * Theme.Values.spacing)

    Connections {
        target: generateProjectFileButton
        function onClicked() { LandingPageApi.generateProjectFile() }
    }

    Column {
        id: column

        width: parent.width
        anchors.centerIn: parent
        spacing: Theme.Values.spacing

        PageText {
            id: projectFileInfoTitle
            width: parent.width
            text: qsTranslate("QtC::QmlProjectManager", "QML PROJECT FILE INFO")
        }

        Column {
            id: projectFileInfoVersionBox
            width: parent.width
            visible: root.projectFileExists

            PageText {
                id: qtVersionText
                width: parent.width
                padding: Theme.Values.spacing
                text: qsTranslate("QtC::QmlProjectManager", "Qt Version - ") + root.qtVersion
            }

            PageText {
                id: qdsVersionText
                width: parent.width
                padding: Theme.Values.spacing
                text: qsTranslate("QtC::QmlProjectManager", "Qt Design Studio Version - ") + root.qdsVersion
            }
        }

        Column {
            id: projectFileInfoMissingBox
            width: parent.width
            visible: !projectFileInfoVersionBox.visible

            PageText {
                id: projectFileInfoMissingText
                width: parent.width
                padding: Theme.Values.spacing
                text: qsTranslate("QtC::QmlProjectManager", "No QML project file found - Would you like to create one?")
            }

            PushButton {
                id: generateProjectFileButton
                text: qsTranslate("QtC::QmlProjectManager", "Generate")
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }
}
