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

    AreaSeries {
        name: "AreaSeries"
        upperSeries: lineSeries

        LineSeries {
            id: lineSeries
            XYPoint {
                x: 0
                y: 1.5
            }

            XYPoint {
                x: 1
                y: 3
            }

            XYPoint {
                x: 6
                y: 6.3
            }

            XYPoint {
                x: 10
                y: 3.1
            }
        }
    }
}
