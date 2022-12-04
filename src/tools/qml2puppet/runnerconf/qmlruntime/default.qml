// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QmlRuntime.QmlConfiguration 1.0

Configuration {
    PartialScene {
        itemType: "QQuickItem"
        container: Qt.resolvedUrl("content/resizeItemToWindow.qml")
    }
}
