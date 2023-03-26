// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQml 2.2
import QtQuick 2.0

Item {
    Timer {
        interval: 500; running: true; repeat: true
        onTriggered: time.text = Date().toString()
    }

    Text { id: time }
}
