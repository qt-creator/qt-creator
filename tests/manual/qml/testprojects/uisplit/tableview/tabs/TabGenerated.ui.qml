// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Window 2.1
import QtQuick.Controls 1.2

TableView {
    model: largeModel

    width: 540
    height: 360

    TableViewColumn {
        role: "name"
        title: "Name"
        width: 120
    }
    TableViewColumn {
        role: "age"
        title: "Age"
        width: 120
    }
    TableViewColumn {
        role: "gender"
        title: "Gender"
        width: 120
    }
}
