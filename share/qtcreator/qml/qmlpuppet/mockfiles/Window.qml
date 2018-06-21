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

Rectangle {
    property string title

    property Item toolBar
    property Item statusBar

    property alias contentItem : contentArea
    default property alias data: contentArea.data

    onStatusBarChanged: { if (statusBar) { statusBar.parent = statusBarArea } }
    onToolBarChanged: { if (toolBar) { toolBar.parent = toolBarArea } }

    property int maximumWidth: 0
    property int minimumWidth: 0

    property int maximumHeight: 0
    property int minimumHeight: 0

    Item {
        id: contentArea
        anchors.top: toolBarArea.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: statusBarArea.top
    }

    Item {
        id: toolBarArea
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        implicitHeight: childrenRect.height
        height: visibleChildren.length > 0 ? implicitHeight: 0
    }

    Item {
        id: statusBarArea
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        implicitHeight: childrenRect.height
        height: 0
        //The status bar is not visible for now
        //height: visibleChildren.length > 0 ? implicitHeight: 0
    }

}
