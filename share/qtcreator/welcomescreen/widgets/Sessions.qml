/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 1.0
import qtcomponents 1.0

Item {
    property alias model: root.model
    property int topMargin: 6
    height: Math.min(root.contentHeight + topMargin, parent.height - 260)

    property alias scrollBarVisible: vscrollbar.visible

    ListView {
        id: root

        anchors.fill: parent
        anchors.topMargin: topMargin

        snapMode: ListView.SnapToItem
        property int delegateHeight: currentItem.height + spacing
        //property int delegateHeight: 22

        interactive: false

        spacing: 4
        cacheBuffer: 800 //We need a big cache to avoid artefacts caused by delegate recreation
        clip: true


        delegate: SessionItem {
            function fullSessionName()
            {
                var newSessionName = sessionName
                if (model.lastSession && sessionList.isDefaultVirgin())
                    newSessionName = qsTr("%1 (last session)").arg(sessionName);
                else if (model.activeSession && !sessionList.isDefaultVirgin())
                    newSessionName = qsTr("%1 (current session)").arg(sessionName);
                return newSessionName;
            }

            name: fullSessionName()
        }

        WheelArea {
            id: wheelarea
            anchors.fill: parent
            verticalMinimumValue: vscrollbar.minimumValue
            verticalMaximumValue: vscrollbar.maximumValue
            onVerticalValueChanged: root.contentY =  verticalValue
        }

        ScrollBar {
            id: vscrollbar
            orientation: Qt.Vertical
            property int availableHeight : root.height
            visible: root.contentHeight > availableHeight
            maximumValue: root.contentHeight > availableHeight ? root.contentHeight - availableHeight : 0
            minimumValue: 0
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            singleStep: root.delegateHeight
            anchors.topMargin: styleitem.style === "mac" ? 1 : 0
            onValueChanged: root.contentY = value
            anchors.rightMargin: styleitem.frameoffset
            anchors.bottomMargin: styleitem.frameoffset
            value: root.contentY
        }
    }

}
