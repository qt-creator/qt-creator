// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick3D
import QtGraphs

Bars3D {
    id: bars3d
    width: 300
    height: 300
    Bar3DSeries {
        id: bars3dSeries
        ItemModelBarDataProxy {
            id: barsDataProxy
            itemModel: ListModel {
                ListElement{ row: "row 1"; column: "column 1"; value: "1"; }
                ListElement{ row: "row 1"; column: "column 2"; value: "2"; }
                ListElement{ row: "row 1"; column: "column 3"; value: "3"; }
            }

            rowRole: "row"
            columnRole: "column"
            valueRole: "value"
        }
    }
}

