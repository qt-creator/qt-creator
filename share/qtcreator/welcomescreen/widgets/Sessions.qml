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
import widgets 1.0

Rectangle {
    id: root
    property var model
    height: content.contentHeight + 200
    color: creatorTheme.Welcome_BackgroundColor

    ListView {
        id: content
        model: root.model

        anchors.fill: parent
        snapMode: ListView.SnapToItem
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

            function maybeNumber() {
                return index < 9 ? "%1".arg(index + 1) : "";
            }

            function tooltipText() {
                var shortcutText = welcomeMode.sessionsShortcuts[index];
                if (shortcutText)
                    return qsTr("Opens session \"%1\" (%2)").arg(sessionName).arg(shortcutText);
                else
                    return qsTr("Opens session \"%1\"").arg(sessionName);
            }

            number: maybeNumber()
            name: fullSessionName()
            tooltip: tooltipText()
        }
    }

    Connections {
        target: welcomeMode
        // this is coming from session shortcuts
        onOpenSessionTriggered: {
            if (index < content.count) {
                content.currentIndex = index;
                // calling a javascript method on the SessionItem which knows its own current sessionName
                content.currentItem.requestSession();
            }
        }
    }
}
