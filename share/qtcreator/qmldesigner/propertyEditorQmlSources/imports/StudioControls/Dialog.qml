/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

import QtQuick
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

T.Dialog {
    id: root

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding,
                            implicitHeaderWidth,
                            implicitFooterWidth)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding
                             + (implicitHeaderHeight > 0 ? implicitHeaderHeight + spacing : 0)
                             + (implicitFooterHeight > 0 ? implicitFooterHeight + spacing : 0))

    padding: StudioTheme.Values.dialogPadding

    background: Rectangle {
        color: StudioTheme.Values.themeDialogBackground
        border.color: StudioTheme.Values.themeDialogOutline
        border.width: StudioTheme.Values.border
    }

    header: T.Label {
        text: root.title
        visible: root.title
        elide: T.Label.ElideRight
        font.bold: true
        padding: StudioTheme.Values.dialogPadding
        color: StudioTheme.Values.themeTextColor

        background: Rectangle {
            x: StudioTheme.Values.border
            y: StudioTheme.Values.border
            width: parent.width - (2 * StudioTheme.Values.border)
            height: parent.height - (2 * StudioTheme.Values.border)
            color: StudioTheme.Values.themeDialogBackground
        }
    }

    footer: DialogButtonBox {
        visible: count > 0
    }

    T.Overlay.modal: Rectangle {
        color: Qt.alpha(StudioTheme.Values.themeDialogBackground, 0.5)
    }
}
