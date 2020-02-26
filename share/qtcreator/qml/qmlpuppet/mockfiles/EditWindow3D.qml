/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
import QtQuick.Window 2.12

Window {
    id: viewWindow
    width: 1024
    height: 768
    visible: true
    title: qsTr("3D Edit View [") + sceneId + qsTr("]")
    // need all those flags otherwise the title bar disappears after setting WindowStaysOnTopHint flag later
    flags: Qt.Window | Qt.WindowTitleHint | Qt.WindowSystemMenuHint | Qt.WindowMinMaxButtonsHint | Qt.WindowCloseButtonHint

    property alias editViewRoot: windowContentItem

    onWidthChanged: _generalHelper.storeWindowState();
    onHeightChanged: _generalHelper.storeWindowState();
    onXChanged: _generalHelper.storeWindowState();
    onYChanged: _generalHelper.storeWindowState();
    onWindowStateChanged: _generalHelper.storeWindowState();

    EditView3D {
        id: windowContentItem
        anchors.fill: parent
    }
}
