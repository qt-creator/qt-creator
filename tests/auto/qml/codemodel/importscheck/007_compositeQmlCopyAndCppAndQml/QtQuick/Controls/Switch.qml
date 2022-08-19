// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

/*!
    \qmltype Switch
    \inqmlmodule QtQuick.Controls
    \since 5.2
    \ingroup controls
    \brief A switch.

    A Switch is an option button that can be switched on (checked) or off
    (unchecked). Switches are typically used to represent features in an
    application that can be enabled or disabled without affecting others.

    On mobile platforms, switches are commonly used to enable or disable
    features.

    \qml
    Column {
        Switch { checked: true }
        Switch { checked: false }
    }
    \endqml

    You can create a custom appearance for a Switch by
    assigning a \l {QtQuick.Controls.Styles::SwitchStyle}{SwitchStyle}.
*/

Control {
    id: root

    /*!
        This property is \c true if the control is checked.
        The default value is \c false.
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

    Keys.onPressed: {
        if (event.key === Qt.Key_Space && !event.isAutoRepeat)
            checked = !checked;
    }

    /*! \internal */
    onExclusiveGroupChanged: {
        if (exclusiveGroup)
            exclusiveGroup.bindCheckable(root)
    }

    MouseArea {
        id: internal

        property Item handle: __panel ? __panel.__handle : null
        property int min: __style ? __style.padding.left : 0
        property int max: handle.parent.width - (handle ? handle.width : 0) -
                          ( __style ? __style.padding.right : 0)
        focus: true
        anchors.fill: parent
        drag.threshold: 0
        drag.target: handle
        drag.axis: Drag.XAxis
        drag.minimumX: min
        drag.maximumX: max

        onPressed: {
            if (activeFocusOnPress)
                root.forceActiveFocus()
        }

        onReleased: {
            if (drag.active) {
                checked = (handle.x < max/2) ? false : true;
                internal.handle.x = checked ? internal.max : internal.min
            } else {
                checked = (handle.x === max) ? false : true
            }
        }
    }

    Component.onCompleted: {
        internal.handle.x = checked ? internal.max : internal.min
        __panel.enableAnimation = true
    }

    onCheckedChanged:  {
        if (internal.handle)
            internal.handle.x = checked ? internal.max : internal.min
    }

    activeFocusOnTab: true
    Accessible.role: Accessible.CheckBox
    Accessible.name: "switch"

    /*!
        The style that should be applied to the switch. Custom style
        components can be created with:

        \codeline Qt.createComponent("path/to/style.qml", switchId);
    */
    style: Qt.createComponent(Settings.style + "/SwitchStyle.qml", root)
}
