// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Dialogs 1.0

ColorDialog {
    id: colorDialog
    title: "Please choose a color"
    onAccepted: {
        console.log("You chose: " + colorDialog.color)
        Qt.quit()
    }
    onRejected: {
        console.log("Canceled")
        Qt.quit()
    }
}

