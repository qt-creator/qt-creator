/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick 2.12

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
