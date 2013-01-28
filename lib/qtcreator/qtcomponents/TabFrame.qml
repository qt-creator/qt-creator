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
import "custom" as Components

Item {
    id: tabWidget
    width: 100
    height: 100
    focus: true
    property TabBar tabbar
    property int current: 0
    property int count: stack.children.length
    property bool frame:true
    property bool tabsVisible: true
    property string position: "North"
    default property alias tabs : stack.children

    onCurrentChanged: __setOpacities()
    Component.onCompleted: __setOpacities()
    onTabbarChanged: {
        tabbar.tabFrame = tabWidget
        tabbar.anchors.top = tabWidget.top
        tabbar.anchors.left = tabWidget.left
        tabbar.anchors.right = tabWidget.right
    }

    property int __baseOverlap : frameitem.pixelMetric("tabbaseoverlap")// add paintmargins;
    function __setOpacities() {
        for (var i = 0; i < stack.children.length; ++i) {
            stack.children[i].visible = (i == current ? true : false)
        }
    }

    QStyleItem {
        id: frameitem
        z: style == "oxygen" ? 1 : 0
        elementType: "tabframe"
        info: position
        value: tabbar && tabsVisible && tabbar.tab(current) ? tabbar.tab(current).x : 0
        minimum: tabbar && tabsVisible && tabbar.tab(current) ? tabbar.tab(current).width : 0
        maximum: tabbar && tabsVisible ? tabbar.tabWidth : width
        anchors.fill: parent

        property int frameWidth: pixelMetric("defaultframewidth")

        Item {
            id: stack
            anchors.fill: parent
            anchors.margins: (frame ? frameitem.frameWidth : 0)
            anchors.topMargin: anchors.margins + (frameitem.style =="mac" ? 6 : 0)
            anchors.bottomMargin: anchors.margins + (frameitem.style =="mac" ? 6 : 0)
        }

        anchors.topMargin: tabbar && tabsVisible && position == "North" ? tabbar.height - __baseOverlap : 0

        states: [
            State {
                name: "South"
                when: position == "South" && tabbar!= undefined
                PropertyChanges {
                    target: frameitem
                    anchors.topMargin: 0
                    anchors.bottomMargin: tabbar ? tabbar.height - __baseOverlap: 0
                }
                PropertyChanges {
                    target: tabbar
                    anchors.topMargin: -__baseOverlap
                }
                AnchorChanges {
                    target: tabbar
                    anchors.top: frameitem.bottom
                    anchors.bottom: undefined
                }
            }
        ]
    }
}
