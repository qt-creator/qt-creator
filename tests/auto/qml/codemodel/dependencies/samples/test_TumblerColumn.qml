// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Extras 1.4

Tumbler {
    TumblerColumn {
        model: [1, 2, 3]
    }
    TumblerColumn {
        model: ["A", "B", "C"]
        visible: false
    }
}
