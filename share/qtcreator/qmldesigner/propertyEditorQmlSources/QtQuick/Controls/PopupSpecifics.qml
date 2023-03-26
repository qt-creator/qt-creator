// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    PopupSection {
        anchors.left: parent.left
        anchors.right: parent.right
    }

    MarginSection {
        anchors.left: parent.left
        anchors.right: parent.right
    }

    PaddingSection {
        anchors.left: parent.left
        anchors.right: parent.right
    }

    FontSection {}
}
