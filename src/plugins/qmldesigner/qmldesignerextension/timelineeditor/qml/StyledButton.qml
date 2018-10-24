/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuickDesignerTheme 1.0

Button {
    id: control
    text: qsTr("")

    font.pixelSize: 12
    topPadding: 0.5
    bottomPadding: 0.5

    contentItem: Text {
        color: Theme.color(Theme.PanelTextColorLight)
        text: control.text
        font: control.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        implicitWidth: 70
        implicitHeight: 20
        color: Theme.qmlDesignerButtonColor()
        radius: 3

        border.width: 1
        border.color: Theme.qmlDesignerBackgroundColorDarker()
    }
}
