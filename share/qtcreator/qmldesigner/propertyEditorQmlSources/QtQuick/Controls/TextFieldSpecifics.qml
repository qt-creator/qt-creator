// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Column {
    width: parent.width

    TextSection {
        caption: qsTr("Text Field")
    }

    CharacterSection {
        showVerticalAlignment: true
    }

    TextInputSection {
        isTextInput: true
    }

    TextExtrasSection {
        showWrapMode: true
    }

    FontExtrasSection {
        showStyle: false
    }

    PaddingSection {}

    InsetSection {}
}
