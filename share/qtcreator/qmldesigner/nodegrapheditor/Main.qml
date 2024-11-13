// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioTheme as StudioTheme
import NodeGraphEditorBackend

Rectangle {
    id: root

    color: StudioTheme.Values.themePanelBackground

    Rectangle {
        anchors.centerIn: parent
        color: "yellow"
        height: 100
        width: 100
    }
}
