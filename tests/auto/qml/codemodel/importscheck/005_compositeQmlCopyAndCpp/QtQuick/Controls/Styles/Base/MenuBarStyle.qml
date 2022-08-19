// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

/*!
    \qmltype MenuBarStyle
    \internal
    \ingroup applicationwindowstyling
    \inqmlmodule QtQuick.Controls.Styles
*/

Style {
    readonly property color __backgroundColor: "#dcdcdc"

    property Component frame: Rectangle {
        width: control.__contentItem.width
        height: contentHeight
        color: __backgroundColor
    }

    property Component menuItem: Rectangle {
        width: text.width + 12
        height: text.height + 4
        color: sunken ? "#49d" :__backgroundColor

        SystemPalette { id: syspal }

        Text {
            id: text
            text: StyleHelpers.stylizeMnemonics(menuItem.title)
            anchors.centerIn: parent
            renderType: Text.NativeRendering
            color: sunken ? "white" : syspal.windowText
        }
    }
}
