// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

Menu {
    id: control

    required property Item __parentControl // TextInput or TextEdit

    MenuItem {
        style: control.style
        text: qsTr("Undo")
        enabled: control.__parentControl.canUndo
        onTriggered: control.__parentControl.undo()
        /* shortcut: StandardKey.Undo Shortcuts in QQC2 seem to override global shortcuts */
    }
    MenuItem {
        style: control.style
        text: qsTr("Redo")
        enabled: control.__parentControl.canRedo
        onTriggered: control.__parentControl.redo()
        /* shortcut: StandardKey.Redo Shortcuts in QQC2 seem to override global shortcuts */
    }

    MenuSeparator { style: control.style }

    MenuItem {
        style: control.style
        text: qsTr("Copy")
        enabled: control.__parentControl.selectedText !== ""
        onTriggered: control.__parentControl.copy()
        /* shortcut: StandardKey.Copy Shortcuts in QQC2 seem to override global shortcuts */
    }
    MenuItem {
        style: control.style
        text: qsTr("Cut")
        enabled: control.__parentControl.selectedText !== "" && !control.__parentControl.readOnly
        onTriggered: control.__parentControl.cut()
        /* shortcut: StandardKey.Cut Shortcuts in QQC2 seem to override global shortcuts */
    }
    MenuItem {
        style: control.style
        text: qsTr("Paste")
        enabled: control.__parentControl.canPaste
        onTriggered: control.__parentControl.paste()
        /* shortcut: StandardKey.Paste Shortcuts in QQC2 seem to override global shortcuts */
    }
    MenuItem {
        style: control.style
        text: qsTr("Delete")
        enabled: control.__parentControl.selectedText !== ""
        onTriggered: control.__parentControl.remove(control.__parentControl.selectionStart,
                                                    control.__parentControl.selectionEnd)
        /* shortcut: StandardKey.Delete Shortcuts in QQC2 seem to override global shortcuts */
    }
    MenuItem {
        style: control.style
        text: qsTr("Clear")
        enabled: control.__parentControl.text !== ""
        onTriggered: control.__parentControl.clear()
        /* shortcut: StandardKey.DeleteCompleteLine  Shortcuts in QQC2 seem to override global shortcuts */
    }

    MenuSeparator { style: control.style }

    MenuItem {
        style: control.style
        text: qsTr("Select All")
        enabled: control.__parentControl.text !== ""
                 && control.__parentControl.selectedText !== control.__parentControl.text
        onTriggered: control.__parentControl.selectAll()
        /* shortcut: StandardKey.SelectAll Shortcuts in QQC2 seem to override global shortcuts */
    }
}
