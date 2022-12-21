// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QtQuick 2.0 as Qt2
import "subitems"
import "subitems" as Subdir

Qt2.Rectangle {
    id : toprect
    width : 100
    height : 50
    SubItem {
        text: "item1"
    }
    Subdir.SubItem {
        text : "item2"
        y: 25
    }
}
