// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuickDesignerTheme
import StudioControls as StudioControls
import StudioTheme 1.0 as StudioTheme
import EffectMakerBackend

Row {
    width: parent.width

    StudioControls.TextField {
        id: textField

        width: parent.width - 20

        actionIndicatorVisible: false
        translationIndicatorVisible: false

        text: uniformValue

        onEditingFinished: uniformValue = text
    }
}
