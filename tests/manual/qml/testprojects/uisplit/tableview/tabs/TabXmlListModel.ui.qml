// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Window 2.1
import QtQuick.Controls 1.2

import "../models"

TableView {
    model:  FlickerModel {
    }

    width: 540
    height: 360

    TableViewColumn {
        role: "title"
        title: "Title"
        width: 120
    }
    TableViewColumn {
        role: "credit"
        title: "Credit"
        width: 120
    }
    TableViewColumn {
        role: "imagesource"
        title: "Image source"
        width: 200
        visible: true
    }

    frameVisible: frameCheckbox.checked
    headerVisible: headerCheckbox.checked
    sortIndicatorVisible: sortableCheckbox.checked
    alternatingRowColors: alternateCheckbox.checked
}

