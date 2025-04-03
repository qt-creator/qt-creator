// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtGraphs

GraphsView {
    width: 300
    height: 300

    PieSeries {
        id: pieSeries
        PieSlice { label: "Slice1"; value: 13.5 }
        PieSlice { label: "Slice2"; value: 10.9 }
        PieSlice { label: "Slice3"; value: 8.6 }
    }
}
