/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

import Qt 4.6

Item {
    id: selector

    // public
    
    property var flickable

    // internal

    ItemsViewStyle { id: style }
    
    visible: false

    property var section: null
    property var item: null
    property int sectionXOffset: 0
    property int sectionYOffset: 0

    x: (section && item)? (section.x + item.x + sectionXOffset):0;
    y: (section && item)? (section.y + style.selectionSectionOffset + item.y + sectionYOffset):0;

    Connection {
        sender: itemLibraryModel
        signal: "visibilityUpdated()"
        script: unselect()
    }

    function select(section, item, sectionXOffset, sectionYOffset) {

        selector.item = item
        selector.section = section
        selector.sectionXOffset = sectionXOffset
        selector.sectionYOffset = sectionYOffset

        visible = true
        focusSelection();
    }

    function unselect() {
        section = null
        item = null
        sectionXOffset = 0
        sectionYOffset = 0
        visible = false
    }

    function focusSelection() {
        var pos = -1;

        if (y < flickable.viewportY)
            pos = Math.max(0, y)

        else if ((y + height) >
                 (flickable.viewportY + flickable.height - 1))
            pos = Math.min(Math.max(0, flickable.viewportHeight - flickable.height),
		y + height - flickable.height + 1)

        if (pos >= 0)
            flickable.viewportY = pos;
    }
    
    SystemPalette { id:systemPalette }

    Rectangle {
        anchors.fill: parent
        color: systemPalette.highlight
        clip:true
        Rectangle {
            width:parent.width-1
            x:1
            y:-parent.height/2
            height:parent.height
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.lighter(systemPalette.highlight) }
                GradientStop { position: 1.0; color: systemPalette.highlight }
            }
        }

        Rectangle {
            width:parent.width-1
            x:1
            y:parent.height/2
            height:parent.height
            gradient: Gradient {
                GradientStop { position: 0.0; color: systemPalette.highlight }
                GradientStop { position: 1.0; color: Qt.darker(systemPalette.highlight) }
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
        color:Qt.lighter(systemPalette.highlight)
    }
    Rectangle {
        anchors.right:parent.right
        anchors.left:parent.left
        anchors.bottom:parent.bottom
        anchors.bottomMargin:1
        anchors.leftMargin:2
        height:1
        color:Qt.darker(systemPalette.highlight)
    }
    Rectangle {
        anchors.left:parent.left
        anchors.top:parent.top
        anchors.bottom:parent.bottom
        anchors.leftMargin:1
        anchors.bottomMargin:2
        width:1
        //color:Qt.lighter(systemPalette.highlight)
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.lighter(systemPalette.highlight) }
            GradientStop { position: 1.0; color: systemPalette.highlight }
        }
    }
    Rectangle {
        anchors.right:parent.right
        anchors.top:parent.top
        anchors.bottom:parent.bottom
        anchors.rightMargin:1
        anchors.topMargin:2
        width:1
        //color:Qt.darker(systemPalette.highlight)
        gradient: Gradient {
            GradientStop { position: 0.0; color: systemPalette.highlight }
            GradientStop { position: 1.0; color: Qt.darker(systemPalette.highlight) }
        }
    }
}
