// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QtQuick.Layouts 1.3

ColumnLayout{
    spacing: 2

    Rectangle {
        Layout.alignment: Qt.AlignCenter
        color: "red"
        Layout.preferredWidth: 40
        Layout.preferredHeight: 40
    }

    Rectangle {
        Layout.alignment: Qt.AlignRight
        color: "green"
        Layout.preferredWidth: 40
        Layout.preferredHeight: 70
    }

    Rectangle {
        Layout.alignment: Qt.AlignBottom
        Layout.fillHeight: true
        color: "blue"
        Layout.preferredWidth: 70
        Layout.preferredHeight: 40
    }
}
