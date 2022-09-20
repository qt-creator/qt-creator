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

T.DialogButtonBox {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            (control.count === 1 ? implicitContentWidth * 2 : implicitContentWidth) + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    contentWidth: contentItem.contentWidth

    spacing: StudioTheme.Values.dialogButtonSpacing
    padding: StudioTheme.Values.dialogPadding
    alignment: Qt.AlignRight | Qt.AlignBottom


    delegate: DialogButton {
        width: control.count === 1 ? control.availableWidth / 2 : undefined
        implicitHeight: StudioTheme.Values.height
        highlighted: DialogButtonBox.buttonRole === DialogButtonBox.AcceptRole
                     || DialogButtonBox.buttonRole === DialogButtonBox.ApplyRole
    }

    contentItem: ListView {
        implicitWidth: contentWidth
        model: control.contentModel
        spacing: control.spacing
        orientation: ListView.Horizontal
        boundsBehavior: Flickable.StopAtBounds
        snapMode: ListView.SnapToItem
    }

    background: Rectangle {
        implicitHeight: 30
        x: StudioTheme.Values.border
        y: StudioTheme.Values.border
        width: parent.width - (2 * StudioTheme.Values.border)
        height: parent.height - (2 * StudioTheme.Values.border)
        color: StudioTheme.Values.themeDialogBackground
    }
}
