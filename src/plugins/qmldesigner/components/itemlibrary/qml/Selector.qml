/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

import Qt 4.7

// the coloured selector of the items view

Item {
    id: selector

    ItemsViewStyle { id: style }

    Rectangle {
        anchors.fill: parent
        color: highlightColor
        clip:true
        Rectangle {
            width:parent.width-1
            x:1
            y:-parent.height/2
            height:parent.height
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.lighter(highlightColor) }
                GradientStop { position: 1.0; color: highlightColor }
            }
        }

        Rectangle {
            width:parent.width-1
            x:1
            y:parent.height/2
            height:parent.height
            gradient: Gradient {
                GradientStop { position: 0.0; color: highlightColor }
                GradientStop { position: 1.0; color: Qt.darker(highlightColor) }
            }
        }
    }
    Rectangle {
        anchors.right:parent.right
        anchors.left:parent.left
        anchors.top:parent.top
        anchors.topMargin:1
        anchors.rightMargin:2
        height:1
        color:Qt.lighter(highlightColor)
    }
    Rectangle {
        anchors.right:parent.right
        anchors.left:parent.left
        anchors.bottom:parent.bottom
        anchors.bottomMargin:1
        anchors.leftMargin:2
        height:1
        color:Qt.darker(highlightColor)
    }
    Rectangle {
        anchors.left:parent.left
        anchors.top:parent.top
        anchors.bottom:parent.bottom
        anchors.leftMargin:1
        anchors.bottomMargin:2
        width:1
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.lighter(highlightColor) }
            GradientStop { position: 1.0; color: highlightColor }
        }
    }
    Rectangle {
        anchors.right:parent.right
        anchors.top:parent.top
        anchors.bottom:parent.bottom
        anchors.rightMargin:1
        anchors.topMargin:2
        width:1
        gradient: Gradient {
            GradientStop { position: 0.0; color: highlightColor }
            GradientStop { position: 1.0; color: Qt.darker(highlightColor) }
        }
    }
}
