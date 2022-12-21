// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

QtObject {
    id: root

    signal downloadFailed

    property bool finished: false

    property bool url

    property int progress: 55
    Behavior on progress { PropertyAnimation {
            duration: 2000
        }
    }

    function start() {
        timer.start()
        root.progress = 100

    }

    property Timer timer: Timer {
        interval: 2000
        repeat: false
        onTriggered: {
            root.finished
            root.progress = 1000
            finished = true
        }

    }
}
