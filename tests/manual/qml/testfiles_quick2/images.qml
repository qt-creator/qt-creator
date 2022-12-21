// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Rectangle {
    width: 640
    height: 480

    Image {
        id: image1
        x: 20
        y: 18
        source: "images/qtcreator.png"
    }

    Image {
        id: image2
        x: 327
        y: 18
        source: "images/qtcreator.jpg"
    }

    Image {
        id: image3
        x: 20
        y: 288
        source: "images/qtcreator.ico"
    }
}
