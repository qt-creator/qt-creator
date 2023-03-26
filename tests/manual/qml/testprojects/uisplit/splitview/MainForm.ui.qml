// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.0

SplitView {
    width: 600
    height: 400

    Rectangle {
        id: column
        width: 200
        Layout.minimumWidth: 100
        Layout.maximumWidth: 300
        color: "lightsteelblue"
    }

    SplitView {
        orientation: Qt.Vertical
        Layout.fillWidth: true

        Rectangle {
            id: row1
            height: 200
            color: "lightblue"
            Layout.minimumHeight: 1
        }

        Rectangle {
            id: row2
            color: "lightgray"
        }
    }
}


