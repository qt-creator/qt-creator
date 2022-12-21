// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Item {
    id: root

    focus: true
    Keys.onPressed: (event)=> {
        if (event.key == Qt.Key_Escape)
            root.doNotShowAgain = false
            root.closeClicked()
    }

    width: 600 * root.mainScale
    height: 720 * root.mainScale

    property int maxHeight: Screen.desktopAvailableHeight - 100

    property real mainScale: root.maxHeight > 720 ? 1 : root.maxHeight / 720


    signal closeClicked
    signal checkBoxToggled
    signal configureClicked

    property alias doNotShowAgain: welcome_splash.doNotShowAgain

    function onPluginInitialized(crashReportingEnabled: bool, crashReportingOn: bool)
    {
        welcome_splash.onPluginInitialized(crashReportingEnabled, crashReportingOn);
    }

    Welcome_splash {
        scale: root.mainScale
        id: welcome_splash

        transformOrigin: Item.TopLeft
        antialiasing: true
        onCloseClicked: root.closeClicked()
        onConfigureClicked: root.configureClicked()
    }
}
