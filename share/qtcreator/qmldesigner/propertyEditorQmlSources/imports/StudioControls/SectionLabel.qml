// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

T.Label {
    id: control

    property StudioTheme.ControlStyle controlStyle: StudioTheme.Values.controlStyle

    width: Math.max(Math.min(240, parent.width - 220), 90)
    color: control.controlStyle.text.idle
    font.pixelSize: control.controlStyle.baseFontSize
    elide: Text.ElideRight

    Layout.preferredWidth: width
    Layout.minimumWidth: width
    Layout.maximumWidth: width
}
