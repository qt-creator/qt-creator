// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import StudioTheme as StudioTheme

TextEdit {
    required property string html

    text: html
    textFormat: TextEdit.RichText
    wrapMode: TextEdit.Wrap
    font.pixelSize: StudioTheme.Values.baseFontSize
    readOnly: true
    selectByKeyboard: true
    color: StudioTheme.Values.themeTextColor

    onLinkActivated: function(link) { Qt.openUrlExternally(link) }
}
