// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.6
import MyPlugin 1.0

Item {
    width: 40
    height: 40
    MyObject {
        test: true
    }
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#ffffff"
            }

            GradientStop {
                position: 1
                color: "#000000"
            }
        }

    }

}
