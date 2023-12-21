// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import WelcomeScreen 1.0
import projectmodel 1.0

Item {
    id: appFrame
    clip: true
    width: Constants.width
    height: Constants.height

    property int loadingProgress: 50

    onLoadingProgressChanged: Constants.loadingProgress = appFrame.loadingProgress

    NumberAnimation {
        target: appFrame
        property: "loadingProgress"
        from: 0
        to: 100
        loops: Animation.Infinite
        running: false
        duration: 1000
    }

    MainScreen {
        id: screen
        anchors.fill: parent
        anchors.leftMargin: screen.designMode ? 0 : -45 // hide sidebar
    }

    property int pageIndex: 0
    property int minimumWidth: 1200
    property int minimumHeight: 720

    onHeightChanged: {
        if (appFrame.height > appFrame.minimumHeight)
            appFrame.anchors.fill = parent
        else if (appFrame.height < appFrame.minimumHeight)
            appFrame.height = appFrame.minimumHeight
    }
    onWidthChanged: {
        if (appFrame.width > appFrame.minimumWidth)
            appFrame.anchors.fill = parent
        else if (appFrame.width < appFrame.minimumWidth)
            appFrame.width = appFrame.minimumWidth
    }
}
