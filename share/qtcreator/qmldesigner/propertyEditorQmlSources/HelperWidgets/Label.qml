/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

import QtQuick 2.1
import QtQuick.Controls 1.1 as Controls
import QtQuick.Layouts 1.0
import QtQuick.Controls.Private 1.0
import QtQuickDesignerTheme 1.0

Controls.Label {
    id: label

    property alias tooltip: toolTipArea.tooltip
    // workaround because PictureSpecifics.qml still use this
    property alias toolTip: toolTipArea.tooltip

    width: Math.max(Math.min(240, parent.width - 220), 80)
    color: Theme.color(Theme.PanelTextColorLight)
    elide: Text.ElideRight

    Layout.preferredWidth: width
    Layout.minimumWidth: width
    Layout.maximumWidth: width

    ToolTipArea {
        id: toolTipArea
        anchors.fill: parent
        tooltip: label.text
    }
}
