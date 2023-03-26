// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1
import QtQuick.Controls.Styles 1.1

ControlsForm {
    id: flickable
    button2.menu: Menu {
        MenuItem { text: "This Button" }
        MenuItem { text: "Happens To Have" }
        MenuItem { text: "A Menu Assigned" }
    }

    editableCombo.onAccepted: {
        if (editableCombo.find(currentText) === -1) {
            choices.append({text: editText})
            currentIndex = editableCombo.find(editText)
        }
    }

    fontComboBox.model: Qt.fontFamilies()

    rowLayout1.data: [ ExclusiveGroup { id: tabPositionGroup } ]
}
