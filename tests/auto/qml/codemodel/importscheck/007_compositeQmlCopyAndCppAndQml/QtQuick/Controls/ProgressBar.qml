// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

/*!
    \qmltype ProgressBar
    \inqmlmodule QtQuick.Controls
    \since 5.1
    \ingroup controls
    \brief A progress indicator.

    The ProgressBar is used to give an indication of the progress of an operation.
    \l value is updated regularly and must be between \l minimumValue and \l maximumValue.

    You can create a custom appearance for a ProgressBar by
    assigning a \l {QtQuick.Controls.Styles::ProgressBarStyle}{ProgressBarStyle}.
*/

Control {
    id: progressbar

    /*! This property holds the progress bar's current value.
        Attempting to change the current value to one outside the minimum-maximum
        range has no effect on the current value.

        The default value is \c{0}.
    */
    property real value: 0

    /*! This property is the progress bar's minimum value.
        The \l value is clamped to this value.
        The default value is \c{0}.
    */
    property real minimumValue: 0

    /*! This property is the progress bar's maximum value.
        The \l value is clamped to this value.
        If maximumValue is smaller than \l minimumValue, \l minimumValue will be enforced.
        The default value is \c{1}.
    */
    property real maximumValue: 1

    /*! This property toggles indeterminate mode.
        When the actual progress is unknown, use this option.
        The progress bar will be animated as a busy indicator instead.
        The default value is \c false.
    */
    property bool indeterminate: false

    /*! \qmlproperty enumeration orientation

        This property holds the orientation of the progress bar.

        \list
        \li Qt.Horizontal - Horizontal orientation. (Default)
        \li Qt.Vertical - Vertical orientation.
        \endlist
    */
    property int orientation: Qt.Horizontal

    /*! \qmlproperty bool ProgressBar::hovered

        This property indicates whether the control is being hovered.
    */
    readonly property alias hovered: hoverArea.containsMouse

    /*! \internal */
    style: Qt.createComponent(Settings.style + "/ProgressBarStyle.qml", progressbar)

    /*! \internal */
    property bool __initialized: false
    /*! \internal */
    onMaximumValueChanged: setValue(value)
    /*! \internal */
    onMinimumValueChanged: setValue(value)
    /*! \internal */
    onValueChanged: if (__initialized) setValue(value)
    /*! \internal */
    Component.onCompleted: {
        __initialized = true;
        setValue(value)
    }

    activeFocusOnTab: false

    Accessible.role: Accessible.ProgressBar
    Accessible.name: value

    implicitWidth:(__panel ? __panel.implicitWidth : 0)
    implicitHeight: (__panel ? __panel.implicitHeight: 0)

    MouseArea {
        id: hoverArea
        anchors.fill: parent
        hoverEnabled: true
    }

    /*! \internal */
    function setValue(v) {
        var newval = parseFloat(v)
        if (!isNaN(newval)) {
            // we give minimumValue priority over maximum if they are inconsistent
            if (newval > maximumValue) {
                if (maximumValue >= minimumValue)
                    newval = maximumValue;
                else
                    newval = minimumValue
            } else if (v < minimumValue) {
                newval = minimumValue
            }
            if (value !== newval)
                value = newval
        }
    }
}
