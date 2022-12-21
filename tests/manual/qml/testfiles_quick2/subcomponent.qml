// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Rectangle {
    width: 640
    height: 480
    Component {
        id: redSquare
        Rectangle {
            color: "red"
            width: 100
            height: 100
        }
    }

    Loader { sourceComponent: redSquare;}
    Loader { sourceComponent: redSquare; x: 20 }
}
