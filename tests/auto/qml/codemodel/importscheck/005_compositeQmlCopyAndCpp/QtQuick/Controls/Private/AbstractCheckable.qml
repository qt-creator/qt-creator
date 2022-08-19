// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1

/*!
    \qmltype AbstractCheckable
    \inqmlmodule QtQuick.Controls
    \ingroup controls
    \brief An abstract representation of a checkable control with a label
    \qmlabstract
    \internal

    A checkable control is one that has two states: checked (on) and
    unchecked (off). AbstractCheckable encapsulates the basic behavior and
    states that are required by checkable controls.

    Examples of checkable controls are RadioButton and
    CheckBox. CheckBox extends AbstractCheckable's behavior by adding a third
    state: partially checked.
*/

Control {
    id: abstractCheckable

    /*!
        Emitted whenever the control is clicked.
    */
    signal clicked

    /*!
        \qmlproperty bool AbstractCheckable::pressed

        This property is \c true if the control is being pressed.
        Set this property to manually invoke a mouse click.
    */
    property alias pressed: mouseArea.effectivePressed

    /*! \qmlproperty bool AbstractCheckcable::hovered

        This property indicates whether the control is being hovered.
    */
    readonly property alias hovered: mouseArea.containsMouse

    /*!
        This property is \c true if the control is checked.
    */
    property bool checked: false

    /*!
        This property is \c true if the control takes the focus when it is
        pressed; \l{QQuickItem::forceActiveFocus()}{forceActiveFocus()} will be
        called on the control.
    */
    property bool activeFocusOnPress: false

    /*!
        This property stores the ExclusiveGroup that the control belongs to.
    */
    property ExclusiveGroup exclusiveGroup: null

    /*!
        This property holds the text that the label should display.
    */
    property string text

    /*! \internal */
    property var __cycleStatesHandler: cycleRadioButtonStates

    activeFocusOnTab: true

    MouseArea {
        id: mouseArea
        focus: true
        anchors.fill: parent
        hoverEnabled: true
        enabled: !keyPressed

        property bool keyPressed: false
        property bool effectivePressed: pressed && containsMouse || keyPressed

        onClicked: abstractCheckable.clicked();

        onPressed: if (activeFocusOnPress) forceActiveFocus();

        onReleased: {
            if (containsMouse && (!exclusiveGroup || !checked))
                __cycleStatesHandler();
        }
    }

    /*! \internal */
    onExclusiveGroupChanged: {
        if (exclusiveGroup)
            exclusiveGroup.bindCheckable(abstractCheckable)
    }

    Keys.onPressed: {
        if (event.key === Qt.Key_Space && !event.isAutoRepeat && !mouseArea.pressed)
            mouseArea.keyPressed = true;
    }

    Keys.onReleased: {
        if (event.key === Qt.Key_Space && !event.isAutoRepeat && mouseArea.keyPressed) {
            mouseArea.keyPressed = false;
            if (!exclusiveGroup || !checked)
                __cycleStatesHandler();
            clicked();
        }
    }
}
