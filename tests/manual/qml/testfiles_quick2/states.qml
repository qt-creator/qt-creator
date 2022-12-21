// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Rectangle {
    id: rect
    width: 200
    height: 200
    Image {
        id: image1
        x: 41
        y: 46
        source: "images/qtcreator.png"
    }
    Text {
        id: textItem
        x: 66
        y: 93
        text: "Base State"
    }
    states: [
        State {
            name: "State1"
            PropertyChanges {
                target: rect
                color: "blue"
            }
            PropertyChanges {
                target: textItem
                text: "State1"
            }
        },
        State {
            name: "State2"
            PropertyChanges {
                target: rect
                color: "gray"
            }
            PropertyChanges {
                target: textItem
                text: "State2"
            }
        }
    ]
}
