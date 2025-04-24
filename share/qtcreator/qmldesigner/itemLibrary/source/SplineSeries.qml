// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtCharts

ChartView {
    width: 300
    height: 300

    SplineSeries {
        name: "SplineSeries"
        XYPoint { x: 0; y: 1 }
        XYPoint { x: 3; y: 4.3 }
        XYPoint { x: 5; y: 3.1 }
        XYPoint { x: 8; y: 5.8 }
    }
}
