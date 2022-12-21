// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.4
import QtQuick.Extras 1.4

CircularGauge {
    value: accelerating ? maximumValue : 0
    anchors.centerIn: parent

    property bool accelerating: false

    Keys.onSpacePressed: accelerating = true
    Keys.onReleased: {
        if (event.key === Qt.Key_Space) {
            accelerating = false;
            event.accepted = true;
        }
    }

    Behavior on value {
        NumberAnimation {
            duration: 1000
        }
    }
}

