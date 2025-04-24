// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtCharts

ChartView {
    width: 300
    height: 300

    AreaSeries {
        name: "AreaSeries"
        upperSeries: LineSeries {
            XYPoint { x: 0; y: 1.5 }
            XYPoint { x: 1; y: 3 }
            XYPoint { x: 3; y: 4.3 }
            XYPoint { x: 6; y: 1.1 }
        }
    }
}
