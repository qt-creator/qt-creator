// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtCharts

ChartView {
    width: 300
    height: 300

    HorizontalStackedBarSeries {
        name: "HorizontalStackedBarSeries"
        BarSet { label: "Set1"; values: [2, 2, 3] }
        BarSet { label: "Set2"; values: [5, 1, 2] }
        BarSet { label: "Set3"; values: [3, 5, 8] }
    }
}
