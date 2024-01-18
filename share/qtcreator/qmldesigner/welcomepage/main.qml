// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import WelcomeScreen 1.0

Loader {
    id: loader
    asynchronous: false
    source: "MainScreen.qml"

    property int loadingProgress: 50

    onLoadingProgressChanged: {
        Constants.loadingProgress = loader.loadingProgress
    }

    Rectangle {
        anchors.fill: parent
        color: Constants.currentThemeBackground
    }
}
