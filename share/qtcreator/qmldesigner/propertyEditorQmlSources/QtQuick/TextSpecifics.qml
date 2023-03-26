// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    CharacterSection {
        richTextEditorAvailable: true
        showLineHeight: true
        showVerticalAlignment: true
    }

    TextExtrasSection {
        showElide: true
        showWrapMode: true
        showFormatProperty: true
        showFontSizeMode: true
        showLineHeight: true
    }

    FontExtrasSection {
        showStyle: true
    }

    PaddingSection {}
}
