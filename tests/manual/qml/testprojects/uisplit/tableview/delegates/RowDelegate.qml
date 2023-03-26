// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Window 2.1
import QtQuick.Controls 1.2

Component {
    Rectangle {
        height: (delegateChooser.currentIndex == 1 && styleData.selected) ? 30 : 20
        Behavior on height{ NumberAnimation{} }

        color: styleData.selected ? "#448" : (styleData.alternate? "#eee" : "#fff")
        BorderImage{
            id: selected
            anchors.fill: parent
            source: "../images/selectedrow.png"
            visible: styleData.selected
            border{left:2; right:2; top:2; bottom:2}
            SequentialAnimation {
                running: true; loops: Animation.Infinite
                NumberAnimation { target:selected; property: "opacity"; to: 1.0; duration: 900}
                NumberAnimation { target:selected; property: "opacity"; to: 0.5; duration: 900}
            }
        }
    }
}
