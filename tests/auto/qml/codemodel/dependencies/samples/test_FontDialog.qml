// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Dialogs 1.1

FontDialog {
    id: fontDialog
    title: "Please choose a font"
    font: Qt.font({ family: "Arial", pointSize: 24, weight: Font.Normal })
    onAccepted: {
        console.log("You chose: " + fontDialog.font)
        Qt.quit()
    }
    onRejected: {
        console.log("Canceled")
        Qt.quit()
    }
}

