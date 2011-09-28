/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

import QtQuick 1.1
import "widgets" as Widgets
import qtcomponents 1.0 as Components

Item {
    id: floater

    Image {
        source: "qrc:welcome/images/welcomebg.png"
        anchors.fill: parent
        opacity: 0.8
    }

    width: 920
    height: 600

    property int proposedWidth: 920
    property int proposedHeight: 600

    Rectangle {
        id: rectangle1
        height: 26
        opacity: 0.9
        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#cff7f7f7"
            }

            GradientStop {
                position: 1
                color: "#04f7f7f7"
            }
        }
        anchors.rightMargin: -1
        anchors.topMargin: -4
        anchors.right: root.right
        anchors.left: root.left
        anchors.top: root.bottom
        Rectangle {
            width: 1
            gradient: Gradient {
                GradientStop {
                    position: 0
                    color: "#a41c1b1b"
                }

                GradientStop {
                    position: 1
                    color: "#0a1c1b1b"
                }
            }
            anchors.top: parent.top
            anchors.topMargin: 0
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.leftMargin: 0

        }
        Rectangle {
            width: 1
            gradient: Gradient {
                GradientStop {
                    position: 0
                    color: "#a41c1b1b"
                }

                GradientStop {
                    position: 1
                    color: "#0a000000"
                }
            }
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.top: parent.top

        }
    }

    Rectangle {

        width: Math.min(floater.width, floater.proposedWidth)
        height: Math.min(floater.height, floater.proposedHeight)
        id: root
        property int margin: 8
        anchors.centerIn: floater

        Components.ScrollArea {
            id: scrollArea
            anchors.fill:  parent
            anchors.margins: - margin
            frame: true
            Item {
                id: baseitem
                height: Math.max(recentSessions.height, recentProjects.height)
                width: root.width - 20
                Widgets.RecentSessions {
                    id: recentSessions
                    width: Math.floor(root.width / 2.5)
                    anchors.left: parent.left
                }
                Widgets.RecentProjects {
                    id: recentProjects
                    anchors.left:  recentSessions.right
                    anchors.right: parent.right
                    anchors.rightMargin: scrollArea.verticalScrollBar.visible ? 0 :
                                                                                -scrollArea.verticalScrollBar.width - 6
                }

            }
        }
        BorderImage {
            anchors.top: scrollArea.top
            height: root.height + 2 * margin
            width: 1
            x: recentProjects.x - margin
        }
    }
}
