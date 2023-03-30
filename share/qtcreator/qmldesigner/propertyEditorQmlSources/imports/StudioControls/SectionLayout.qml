// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import StudioTheme 1.0 as StudioTheme

GridLayout {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    columns: 2
    columnSpacing: control.style.sectionColumnSpacing
    rowSpacing: control.style.sectionRowSpacing
    width: parent.width - 16 // TODO parameterize
}
