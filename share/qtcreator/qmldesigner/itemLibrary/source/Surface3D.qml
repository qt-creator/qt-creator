// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick3D
import QtGraphs

Surface3D {
    id: surface3d
    width: 300
    height: 300
    Surface3DSeries {
        id: surface3dSeries
        ItemModelSurfaceDataProxy {
            id: surfaceDataProxy
            itemModel: ListModel {
                ListElement{ row: "1"; column: "1"; y: "1"; }
                ListElement{ row: "1"; column: "2"; y: "2"; }
                ListElement{ row: "2"; column: "1"; y: "3"; }
                ListElement{ row: "2"; column: "2"; y: "4"; }
            }

            rowRole: "row"
            columnRole: "column"
            yPosRole: "y"
        }
    }
}
