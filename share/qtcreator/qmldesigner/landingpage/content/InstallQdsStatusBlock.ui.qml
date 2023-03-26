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
import LandingPageApi
import LandingPage as Theme

Rectangle {
    id: root

    color: Theme.Colors.backgroundSecondary
    height: column.childrenRect.height + (2 * Theme.Values.spacing)

    Connections {
        target: installButton
        function onClicked() { LandingPageApi.installQds() }
    }

    Column {
        id: column

        width: parent.width
        anchors.centerIn: parent

        PageText {
            id: statusText
            width: parent.width
            topPadding: 0
            padding: Theme.Values.spacing
            text: qsTranslate("QtC::QmlProjectManager", "No Qt Design Studio installation found")
        }

        PageText {
            id: suggestionText
            width: parent.width
            padding: Theme.Values.spacing
            text: qsTranslate("QtC::QmlProjectManager", "Would you like to install it now?")
        }

        PushButton {
            id: installButton
            text: qsTranslate("QtC::QmlProjectManager", "Install")
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
