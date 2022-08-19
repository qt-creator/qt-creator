// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

/*!
    \qmltype Button
    \inqmlmodule QtQuick.Controls
    \since 5.1
    \ingroup controls
    \brief A push button with a text label.

    The push button is perhaps the most commonly used widget in any graphical
    user interface. Pushing (or clicking) a button commands the computer to
    perform some action or answer a question. Common examples of buttons are
    OK, Apply, Cancel, Close, Yes, No, and Help buttons.

    Button is similar to the QPushButton widget.

    You can create a custom appearance for a Button by
    assigning a \l {QtQuick.Controls.Styles::ButtonStyle}{ButtonStyle}.
 */
BasicButton {
    id: button

    /*! This property holds whether the push button is the default button.
        Default buttons decide what happens when the user presses enter in a
        dialog without giving a button explicit focus. \note This property only
        changes the appearance of the button. The expected behavior needs to be
        implemented by the user.

        The default value is \c false.
    */
    property bool isDefault: false

    /*! Assign a \l Menu to this property to get a pull-down menu button.

        The default value is \c null.
     */
    property Menu menu: null

    __effectivePressed: __behavior.effectivePressed || menu && menu.__popupVisible

    activeFocusOnTab: true

    Accessible.name: text

    style: Qt.createComponent(Settings.style + "/ButtonStyle.qml", button)

    Binding {
        target: menu
        property: "__minimumWidth"
        value: button.__panel.width
    }

    Binding {
        target: menu
        property: "__visualItem"
        value: button
    }

    Connections {
        target: __behavior
        onEffectivePressedChanged: {
            if (__behavior.effectivePressed && menu)
                popupMenuTimer.start()
        }
    }

    Timer {
        id: popupMenuTimer
        interval: 10
        onTriggered: {
            __behavior.keyPressed = false
            if (Qt.application.layoutDirection === Qt.RightToLeft)
                menu.__popup(button.width, button.height, 0)
            else
                menu.__popup(0, button.height, 0)
        }
    }
}
