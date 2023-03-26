// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.3
import MyPlugin2 1.0

Rectangle {

    width: 640
    height: 480

    MyComponent {
        text: qsTr("Some Text")

    }
}
