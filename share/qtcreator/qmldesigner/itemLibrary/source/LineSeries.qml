// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtCharts

ChartView {
    width: 300
    height: 300

    LineSeries {
        name: "LineSeries"
        XYPoint { x: 0; y: 2 }
        XYPoint { x: 1; y: 1.2 }
        XYPoint { x: 2; y: 3.3 }
        XYPoint { x: 5; y: 2.1 }
    }
}
