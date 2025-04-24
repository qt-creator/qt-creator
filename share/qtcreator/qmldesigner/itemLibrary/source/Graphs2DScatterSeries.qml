// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtGraphs

GraphsView {
    width: 300
    height: 300

    axisX: valueAxisX
    axisY: valueAxisY

    ValueAxis {
        id: valueAxisX
        min: 0
        max: 10
    }
    ValueAxis {
        id: valueAxisY
        min: 0
        max: 10
    }

    ScatterSeries {
        id: lineSeries
        XYPoint {
            x: 0
            y: 2
        }

        XYPoint {
            x: 3
            y: 1.2
        }

        XYPoint {
            x: 7
            y: 3.3
        }

        XYPoint {
            x: 10
            y: 2.1
        }
    }
}
