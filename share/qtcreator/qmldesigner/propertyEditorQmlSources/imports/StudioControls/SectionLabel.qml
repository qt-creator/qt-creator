// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Templates 2.15 as T
import StudioTheme 1.0 as StudioTheme

T.Label {
    id: label

    width: Math.max(Math.min(240, parent.width - 220), 90)
    color: StudioTheme.Values.themeTextColor
    font.pixelSize: StudioTheme.Values.myFontSize // TODO
    elide: Text.ElideRight

    Layout.preferredWidth: width
    Layout.minimumWidth: width
    Layout.maximumWidth: width
}
