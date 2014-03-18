/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

import QtQuick 2.1
import widgets 1.0

Item {
    id: root
    property var model
    property int topMargin: 6
    height: content.contentHeight + 200

    ListView {
        id: content
        model: root.model

        anchors.fill: parent
        anchors.topMargin: topMargin
        snapMode: ListView.SnapToItem
        spacing: 4
        clip: true
        interactive: false

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
    }
}
