// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

/*!
    \qmltype TextAreaStyle
    \inqmlmodule QtQuick.Controls.Styles
    \since 5.2
    \ingroup controlsstyling
    \brief Provides custom styling for TextArea.

    Example:
    \qml
    TextArea {
        style: TextAreaStyle {
            textColor: "#333"
            selectionColor: "steelblue"
            selectedTextColor: "#eee"
            backgroundColor: "#eee"
        }
    }
    \endqml
*/

ScrollViewStyle {
    id: style

    /*! The \l TextArea attached to this style. */
    readonly property TextArea control: __control

    /*! The current font. */
    property font font

    /*! The text color. */
    property color textColor: __syspal.text

    /*! The text highlight color, used behind selections. */
    property color selectionColor: __syspal.highlight

    /*! The highlighted text color, used in selections. */
    property color selectedTextColor: __syspal.highlightedText

    /*! The background color. */
    property color backgroundColor: control.backgroundVisible ? __syspal.base : "transparent"

    /*!
        \qmlproperty enumeration renderType

        Override the default rendering type for the control.

        Supported render types are:
        \list
        \li Text.QtRendering
        \li Text.NativeRendering - the default
        \endlist

        \sa Text::renderType
    */
    property int renderType: Text.NativeRendering
}
