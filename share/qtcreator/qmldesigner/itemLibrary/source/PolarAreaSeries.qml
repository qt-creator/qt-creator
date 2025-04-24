// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtCharts

PolarChartView {
    width: 300
    height: 300
    legend.visible: false

    ValueAxis {
        id: axis1
        tickCount: 9
    }
    ValueAxis {
        id: axis2
    }
    LineSeries {
        id: lowerLine
        axisAngular: axis1
        axisRadial: axis2

        XYPoint { x: 1; y: 5 }
        XYPoint { x: 2; y: 10 }
        XYPoint { x: 3; y: 12 }
        XYPoint { x: 4; y: 17 }
        XYPoint { x: 5; y: 20 }
    }
    LineSeries {
        id: upperLine
        axisAngular: axis1
        axisRadial: axis2

        XYPoint { x: 1; y: 5 }
        XYPoint { x: 2; y: 14 }
        XYPoint { x: 3; y: 20 }
        XYPoint { x: 4; y: 32 }
        XYPoint { x: 5; y: 35 }
    }
    AreaSeries {
        name: "AreaSeries"
        axisAngular: axis1
        axisRadial: axis2
        lowerSeries: lowerLine
        upperSeries: upperLine
    }
}
