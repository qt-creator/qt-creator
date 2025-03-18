// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets

//! [StaticText compatibility]
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

//! [StaticText compatibility]
