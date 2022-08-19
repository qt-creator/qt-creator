// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

/*!
    \qmltype RadioButton
    \inqmlmodule QtQuick.Controls
    \since 5.1
    \ingroup controls
    \brief A radio button with a text label.

    A RadioButton is an option button that can be switched on (checked) or off
    (unchecked). Radio buttons typically present the user with a "one of many"
    choice. In a group of radio buttons, only one radio button at a time can be
    checked; if the user selects another button, the previously selected button
    is switched off.

    \qml
    GroupBox {
        title: qsTr("Search")
        Column {
            ExclusiveGroup { id: group }
            RadioButton {
                text: qsTr("From top")
                exclusiveGroup: group
                checked: true
            }
            RadioButton {
                text: qsTr("From cursor")
                exclusiveGroup: group
            }
        }
    }
    \endqml

    You can create a custom appearance for a RadioButton by
    assigning a \l {QtQuick.Controls.Styles::RadioButtonStyle}{RadioButtonStyle}.
*/

AbstractCheckable {
    id: radioButton

    activeFocusOnTab: true

    Accessible.role: Accessible.RadioButton

    /*!
        The style that should be applied to the radio button. Custom style
        components can be created with:

        \codeline Qt.createComponent("path/to/style.qml", radioButtonId);
    */
    style: Qt.createComponent(Settings.style + "/RadioButtonStyle.qml", radioButton)

    __cycleStatesHandler: function() { checked = !checked; }
}
