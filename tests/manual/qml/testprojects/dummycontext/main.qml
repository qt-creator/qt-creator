// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.4

/* This is a manual test for the dummy context described here:
http://blog.qt.io/blog/2011/07/28/mockup-data-for-the-qml-designer

The dummy context that defines parent and root is dummydata/context/main.qml.
Without the dummy context the root item does not have a defined size and the
color of the inner rectangle is not defined.

When Designer shows an error message about the missing parent,
click "Ignore" to see the rendering.

*/

Rectangle {
    width: parent.width
    height: parent.height

    Rectangle {
       anchors.centerIn: parent
        width: parent.width / 2
        height: parent.height / 2
        color: root.color
    }
}
