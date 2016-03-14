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
import QtQuick.Window 2.1
import QtQuick.Layouts 1.0

ColumnLayout {
    id: root

    spacing: 0

    property alias currentIndex: tabs.currentIndex
    property alias model: tabs.model

    readonly property int lrPadding: 34

    Item {
        id: modeArea

        z: 1
        Layout.fillWidth: true
        Layout.preferredWidth: tabs.width + lrPadding * 2
        Layout.preferredHeight: tabs.height + screenDependHeightDistance * 2

        Rectangle {
            color: creatorTheme.Welcome_BackgroundColor
            anchors.fill: parent
        }

        Tabs {
            anchors.verticalCenter: parent.verticalCenter
            x: lrPadding
            width: Math.max(modeArea.width - lrPadding * 2, implicitWidth)

            id: tabs
            spacing: Math.round((screenDependHeightDistance / count) + 10)
        }
    }

    Rectangle {
        color: creatorTheme.Welcome_BackgroundColor

        Layout.fillWidth: true
        Layout.preferredWidth: upperColumn.width + 20
        Layout.fillHeight: true

        ColumnLayout {
            id: upperColumn

            x: lrPadding

            spacing: 8

            property int spacerHeight: screenDependHeightDistance

            Item {
                Layout.preferredHeight: upperColumn.spacerHeight
            }

            NativeText {
                text: qsTr("New to Qt?")
                color: creatorTheme.Welcome_TextColor
                font.pixelSize: 18
            }

            NativeText {
                id: gettingStartedText

                Layout.preferredWidth: upperColumn.width

                text: qsTr("Learn how to develop your own applications and explore Qt Creator.")
                color: creatorTheme.Welcome_ForegroundPrimaryColor
                font.pixelSize: 12
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            }

            Item {
                Layout.preferredHeight: 4
            }

            Button {
                id: gettingStartedButton

                x: 4

                text: qsTr("Get Started Now")
                onClicked: gettingStarted.openHelp("qthelp://org.qt-project.qtcreator/doc/index.html")
            }

            Item {
                Layout.preferredHeight: upperColumn.spacerHeight * 2
            }
        }

        ColumnLayout {
            anchors.top: upperColumn.bottom
            anchors.topMargin: 8

            IconAndLink {
                iconSource: "image://icons/qtaccount"
                title: qsTr("Qt Account")
                openUrl: "https://account.qt.io"
            }
            IconAndLink {
                iconSource: "image://icons/community"
                title: qsTr("Online Community")
                openUrl: "http://forum.qt.io"
            }
            IconAndLink {
                iconSource: "image://icons/blogs"
                title: qsTr("Blogs")
                openUrl: "http://planet.qt.io"
            }
            IconAndLink {
                iconSource: "image://icons/userguide"
                title: qsTr("User Guide")
                openHelpUrl: "qthelp://org.qt-project.qtcreator/doc/index.html"
            }
        }
    }
}
