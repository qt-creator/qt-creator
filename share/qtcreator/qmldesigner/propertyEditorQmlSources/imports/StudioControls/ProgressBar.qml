// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

T.ProgressBar {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    contentItem: Item {
       implicitWidth: 200
       implicitHeight: 6

       Rectangle {
           width: control.visualPosition * parent.width
           height: parent.height
           color: control.style.interaction
       }
   }

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: 6
        color: control.style.thumbnailLabelBackground
    }
}
