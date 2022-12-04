// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick.Window 2.0
import QtQuick 2.0

Window {
    property Item containedObject: null
    property bool __resizeGuard: false
    onContainedObjectChanged: {
        if (containedObject == undefined || containedObject == null) {
            visible = false
            return
        }
        __resizeGuard = true
        width = containedObject.width
        height = containedObject.height
        containedObject.parent = contentItem
        visible = true
        __resizeGuard = false
    }
    onWidthChanged: if (!__resizeGuard && containedObject)
                        containedObject.width = width
    onHeightChanged: if (!__resizeGuard && containedObject)
                         containedObject.height = height
}
