// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs 1.2

Dialog {
    visible: true
    title: "Blue sky dialog"

    contentItem: Rectangle {
        color: "lightskyblue"
        implicitWidth: 400
        implicitHeight: 100
        Text {
            text: "Hello blue sky!"
            color: "navy"
            anchors.centerIn: parent
        }
    }
}

