// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.12
import QtQuick.Window 2.12
import Charts 1.0 as Charts // Qt Creator displays "QML module not found (Charts)."
                            // if qmlImportPaths value is missing from pyproject.pyproject file.

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("pyproject")

    Charts.ChartBackground {  // Syntax highlight and code completion doesn't work
                              // if qmlImportPaths value is missing from pyproject.pyproject file.
        anchors.centerIn: parent
    }
}
