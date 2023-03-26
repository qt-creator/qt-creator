// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Window 2.1
import QtQuick.Controls 1.2

import "../delegates"
import "../models"

TableView {
    model: NestedModel {
    }

    width: 540
    height: 360

    TableViewColumn {
        role: "content"
        title: "Text and Color"
        width: 220
    }

    itemDelegate: MultiValueDelegate {
    }

    frameVisible: frameCheckbox.checked
    headerVisible: headerCheckbox.checked
    sortIndicatorVisible: sortableCheckbox.checked
    alternatingRowColors: alternateCheckbox.checked
}
