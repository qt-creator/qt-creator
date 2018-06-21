/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick 2.1
import QtQuick.Controls 1.0 as Controls
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
