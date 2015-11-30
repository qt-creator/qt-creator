/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
