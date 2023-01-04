// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15

Menu {
    id: contextMenu

    property Item myTextEdit

    MenuItem {
        text: "Undo"
        enabled: myTextEdit.canUndo
        onTriggered: myTextEdit.undo()
        /* shortcut: StandardKey.Undo Shortcuts in QQC2 seem to override global shortcuts */
    }
    MenuItem {
        text: "Redo"
        enabled: myTextEdit.canRedo
        onTriggered: myTextEdit.redo()
        /* shortcut: StandardKey.Redo Shortcuts in QQC2 seem to override global shortcuts */
    }

    MenuSeparator {
    }

    MenuItem {
        text: "Copy"
        enabled: myTextEdit.selectedText !== ""
        onTriggered: myTextEdit.copy()
        /* shortcut: StandardKey.Copy Shortcuts in QQC2 seem to override global shortcuts */
    }
    MenuItem {
        text: "Cut"
        enabled: myTextEdit.selectedText !== "" && !myTextEdit.readOnly
        onTriggered: myTextEdit.cut()
        /* shortcut: StandardKey.Cut Shortcuts in QQC2 seem to override global shortcuts */
    }
    MenuItem {
        text: "Paste"
        enabled: myTextEdit.canPaste
        onTriggered: myTextEdit.paste()
        /* shortcut: StandardKey.Paste Shortcuts in QQC2 seem to override global shortcuts */
    }
    MenuItem {
        text: "Delete"
        enabled: myTextEdit.selectedText !== ""
        onTriggered: myTextEdit.remove(myTextEdit.selectionStart,
                                       myTextEdit.selectionEnd)
        /* shortcut: StandardKey.Delete Shortcuts in QQC2 seem to override global shortcuts */
    }
    MenuItem {
        text: "Clear"
        enabled: myTextEdit.text !== ""
        onTriggered: myTextEdit.clear()
        /* shortcut: StandardKey.DeleteCompleteLine  Shortcuts in QQC2 seem to override global shortcuts */
    }

    MenuSeparator {
    }

    MenuItem {
        text: "Select All"
        enabled: myTextEdit.text !== ""
                 && myTextEdit.selectedText !== myTextEdit.text
        onTriggered: myTextEdit.selectAll()
        /* shortcut: StandardKey.SelectAll Shortcuts in QQC2 seem to override global shortcuts */
    }
}
