// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtCharts

PolarChartView {
    width: 300
    height: 300

    ScatterSeries {
        name: "ScatterSeries"
        axisRadial: CategoryAxis {
            min: 0
            max: 20
        }
        axisAngular: ValueAxis {
            tickCount: 9
        }
        XYPoint { x: 0; y: 4.3 }
        XYPoint { x: 2; y: 4.7 }
        XYPoint { x: 4; y: 5.2 }
        XYPoint { x: 8; y: 12.9 }
        XYPoint { x: 9; y: 19.2 }
    }
}
