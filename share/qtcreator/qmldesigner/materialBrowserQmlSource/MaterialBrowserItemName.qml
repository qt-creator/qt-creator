// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets
import StudioTheme as StudioTheme
import MaterialBrowserBackend

TextInput {
    id: root

    clip: true
    readOnly: true
    selectByMouse: !root.readOnly

    horizontalAlignment: TextInput.AlignHCenter

    font.pixelSize: StudioTheme.Values.myFontSize

    color: StudioTheme.Values.themeTextColor
    selectionColor: StudioTheme.Values.themeTextSelectionColor
    selectedTextColor: StudioTheme.Values.themeTextSelectedTextColor

    // allow only alphanumeric characters, underscores, no space at start, and 1 space between words
    validator: RegExpValidator { regExp: /^(\w+\s)*\w+$/ }

    signal renamed(string newName)
    signal clicked()

    function startRename()
    {
        root.readOnly = false
        root.selectAll()
        root.forceActiveFocus()
        root.ensureVisible(root.text.length)
        mouseArea.enabled = false
    }

    function commitRename()
    {
        if (root.readOnly)
            return;

        root.renamed(root.text)
    }

    onEditingFinished: root.commitRename()

    onActiveFocusChanged: {
        if (!activeFocus) {
            root.readOnly = true
            mouseArea.enabled = true
            ensureVisible(0)
        }
    }

    Component.onCompleted: ensureVisible(0)

    MouseArea {
        id: mouseArea
        anchors.fill: parent

        onClicked: root.clicked()
        onDoubleClicked: root.startRename()
    }
}
