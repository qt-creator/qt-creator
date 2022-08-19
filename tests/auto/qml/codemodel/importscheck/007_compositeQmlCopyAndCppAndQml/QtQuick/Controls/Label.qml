// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1

/*!
    \qmltype Label
    \inqmlmodule QtQuick.Controls
    \since 5.1
    \ingroup controls
    \brief A text label.

    In addition to the normal \l Text element, Label follows the font and
    color scheme of the system.
    Use the \c text property to assign a text to the label. For other properties
    check \l Text.

    A simple label looks like this:
    \qml
    Label {
        text: "Hello world"
    }
    \endqml

    You can use the properties of \l Text to change the appearance
    of the text as desired:
    \qml
    Label {
        text: "Hello world"
        font.pixelSize: 22
        font.italic: true
        color: "steelblue"
    }
    \endqml

    \sa Text, TextField, TextEdit
*/

Text {
    /*!
        \qmlproperty string Label::text

        The text to display. Use this property to get and set it.
    */

    id: label
    color: pal.text
    activeFocusOnTab: false
    renderType: Text.NativeRendering
    SystemPalette {
        id: pal
        colorGroup: enabled ? SystemPalette.Active : SystemPalette.Disabled
    }
}
