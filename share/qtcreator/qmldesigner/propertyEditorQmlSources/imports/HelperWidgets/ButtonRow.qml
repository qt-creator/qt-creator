// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.0

RowLayout {

    id: buttonRow

    property bool exclusive: false

    property int initalChecked: 0

    property int checkedIndex: -1
    spacing: 0

    onCheckedIndexChanged: {
        __checkButton(checkedIndex)
    }

    signal toggled (int index, bool checked)

    Component.onCompleted: {
        if (exclusive) {
            checkedIndex = initalChecked
            __checkButton(checkedIndex)
        }
    }

    function __checkButton(index) {

        if (buttonRow.exclusive) {
            for (var i = 0; i < buttonRow.children.length; i++) {

                if (i !== index) {
                    if (buttonRow.children[i].checked) {
                        buttonRow.children[i].checked = false
                    }
                }
            }

            buttonRow.children[index].checked = true
            buttonRow.checkedIndex = index
        } else {
            buttonRow.children[index].checked = true
        }

    }

    function __unCheckButton(index) {
        if (buttonRow.exclusive) {
            //nothing
        } else {
            buttonRow.children[index].checked = false
        }
    }

}
