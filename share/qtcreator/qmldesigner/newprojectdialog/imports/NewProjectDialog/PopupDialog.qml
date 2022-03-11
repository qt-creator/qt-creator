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
import QtQuick.Controls
import QtQuick.Layouts

import StudioTheme as StudioTheme

import BackendApi

Dialog {
    id: root
    padding: DialogValues.popupDialogPadding

    background: Rectangle {
        color: DialogValues.darkPaneColor
        border.color: StudioTheme.Values.themeInteraction
        border.width: StudioTheme.Values.border
    }

    header: Label {
        text: root.title
        visible: root.title
        elide: Label.ElideRight
        font.bold: true
        font.pixelSize: DialogValues.defaultPixelSize
        padding: DialogValues.popupDialogPadding
        color: DialogValues.textColor
        horizontalAlignment: Text.AlignHCenter

        background: Rectangle {
            x: 1
            y: 1
            width: parent.width - 2
            height: parent.height - 1
            color: DialogValues.darkPaneColor
        }
    }

    footer: PopupDialogButtonBox {
        visible: count > 0
    }
}
