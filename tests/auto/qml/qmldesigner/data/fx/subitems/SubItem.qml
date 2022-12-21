// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Rectangle {
    property string text

    width : toprect.width
    height : 20
    color : "blue"
    Text {
        width : 100
        color : "white"
        text : parent.text
    }
}
