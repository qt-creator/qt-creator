// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.6
import HelperWidgets 2.0

ComboBox {
    useInteger: true
    backendValue: backendValues.tabPosition
    implicitWidth: 180
    model:  [ "TopEdge", "BottomEdge" ]
    scope: "Qt"

    manualMapping: true

    property bool block: false

    onValueFromBackendChanged: {
        if (!__isCompleted)
            return;

        block = true

        if (backendValues.tabPosition.value === 1)
            currentIndex = 0;
        if (backendValues.tabPosition.value === 0)
            currentIndex = 0;
        if (backendValues.tabPosition.value === 8)
            currentIndex = 1;

        block = false
    }

    onCurrentTextChanged: {
        if (!__isCompleted)
            return;

        if (block)
            return;

        if (currentText === "TopEdge")
            backendValues.tabPosition.value = 1

        if (currentText === "BottomEdge")
            backendValues.tabPosition.value = 8
    }
}
