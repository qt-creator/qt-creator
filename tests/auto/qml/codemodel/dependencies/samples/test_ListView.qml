// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.4

ListView {
    anchors.fill: parent
    model: fruitModel
    delegate: Row {
        Text { text: "Fruit: " + name }
        Text { text: "Cost: $" + cost }
    }
}
