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

import QtQuick 2.9
import QtQuick.Controls 2.3
import welcome 1.0
import StudioFonts 1.0

Button {
    id: button

    property color hoverColor: Constants.textHoverColor
    property color defaultColor: Constants.textDefaultColor
    property color checkedColor: Constants.textDefaultColor

    contentItem: Text {
        id: textButton
        text: button.text
        color: checked ? button.checkedColor :
                         button.hovered ? button.hoverColor :
                                          button.defaultColor
        font.family: StudioFonts.titilliumWeb_regular
        renderType: Text.NativeRendering
        font.pixelSize: 18
    }

    background: Item {
    }
}
