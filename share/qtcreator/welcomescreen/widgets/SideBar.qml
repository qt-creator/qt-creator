/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

    Item {
        id: modeArea

        z: 1
        Layout.fillWidth: true
        Layout.preferredWidth: tabs.width + 16 * 2
        Layout.preferredHeight: tabs.height + screenDependHeightDistance * 2

        Component {
            id: imageBackground
            Image {
                fillMode: Image.Tile
                source: "images/background.png"
                anchors.fill: parent
            }
        }
        Component {
            id: flatBackground
            Rectangle {
                color: creatorTheme.Welcome_SideBar_BackgroundColor
            }
        }
        Loader {
            id: topLeftLoader
            anchors.fill: parent
            sourceComponent: creatorTheme.WidgetStyle === 'StyleFlat' ? flatBackground : imageBackground;
        }

        Tabs {
            anchors.verticalCenter: parent.verticalCenter
            x: 16
            width: Math.max(modeArea.width - 16 * 2, implicitWidth)

            id: tabs
            spacing: Math.round((screenDependHeightDistance / count) + 10)
        }

        Rectangle {
            color: creatorTheme.WidgetStyle === 'StyleFlat' ?
                       creatorTheme.Welcome_SideBar_BackgroundColor : creatorTheme.Welcome_DividerColor
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 0
            height: 1
        }

        Rectangle {
            color: creatorTheme.WidgetStyle === 'StyleFlat' ?
                       creatorTheme.Welcome_SideBar_BackgroundColor : creatorTheme.Welcome_DividerColor
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.bottomMargin: -1
            height: 1
            opacity: 0.6
        }

        Rectangle {
            color: creatorTheme.WidgetStyle === 'StyleFlat' ?
                       creatorTheme.Welcome_SideBar_BackgroundColor : creatorTheme.Welcome_DividerColor
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.bottomMargin: -2
            height: 1
            opacity: 0.2
        }
    }

    Rectangle {
        color: creatorTheme.Welcome_SideBar_BackgroundColor

        Layout.fillWidth: true
        Layout.preferredWidth: innerColumn.width + 20
        Layout.fillHeight: true

        ColumnLayout {
            id: innerColumn

            x: 12

            spacing: 4

            property int spacerHeight: screenDependHeightDistance - 14

            Item {
                Layout.preferredHeight: innerColumn.spacerHeight
            }

            NativeText {
                text: qsTr("New to Qt?")
                color: creatorTheme.Welcome_TextColorNormal
                font.pixelSize: 18
            }

            NativeText {
                id: gettingStartedText

                Layout.preferredWidth: innerColumn.width

                text: qsTr("Learn how to develop your own applications and explore Qt Creator.")
                color: creatorTheme.Welcome_TextColorNormal
                font.pixelSize: 12
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            }

            Item {
                Layout.preferredHeight: innerColumn.spacerHeight
            }

            Button {
                id: gettingStartedButton

                x: 4

                text: qsTr("Get Started Now")
                onClicked: gettingStarted.openHelp("qthelp://org.qt-project.qtcreator/doc/index.html")
            }

            Item {
                Layout.preferredHeight: innerColumn.spacerHeight
            }

            ColumnLayout {
                spacing: 16

                IconAndLink {
                    iconSource: "images/icons/qt_account.png"
                    title: qsTr("Qt Account")
                    openUrl: "https://account.qt.io"
                }
                IconAndLink {
                    iconSource: "images/icons/qt_cloud.png"
                    title: qsTr("Qt Cloud Services")
                    openUrl: "https://developer.qtcloudservices.com"
                }
                IconAndLink {
                    iconSource: "images/icons/onlineCommunity.png"
                    title: qsTr("Online Community")
                    openUrl: "http://forum.qt.io"
                }
                IconAndLink {
                    iconSource: "images/icons/blogs.png"
                    title: qsTr("Blogs")
                    openUrl: "http://planet.qt.io"
                }
                IconAndLink {
                    iconSource: "images/icons/userGuide.png"
                    title: qsTr("User Guide")
                    openHelpUrl: "qthelp://org.qt-project.qtcreator/doc/index.html"
                }
            }
        }
    }
}
