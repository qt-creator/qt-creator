// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Studio.EventSimulator
import QtQuick.Studio.EventSystem

QtObject {
    id: simulator
    property bool active: true

    property Timer __timer: Timer {
        id: timer
        interval: 100
        onTriggered: {
            EventSimulator.show()
        }
    }

    Component.onCompleted: {
        EventSystem.init(Qt.resolvedUrl("EventListModel.qml"))
        if (simulator.active)
            timer.start()
    }
}
