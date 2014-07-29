/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.1
import QtQuick.Window 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

Style {
    id: styleRoot

    property string __menuItemType: "menuitem"

    property Component frame: StyleItem {
        elementType: "menu"

        contentWidth: parent ? parent.contentWidth : 0
        contentHeight: parent ? parent.contentHeight : 0
        width: implicitWidth
        height: implicitHeight

        property int subMenuOverlap: -2 * pixelMetric("menupanelwidth")
        property real maxHeight: Screen.desktopAvailableHeight * 0.99
        property int margin: pixelMetric("menuvmargin") + pixelMetric("menupanelwidth")

        Rectangle {
            visible: anchors.margins > 0
            anchors {
                fill: parent
                margins: pixelMetric("menupanelwidth")
            }
            color: __syspal.window
        }

        Accessible.role: Accessible.PopupMenu
    }

    property Component menuItem: StyleItem {
        elementType: __menuItemType
        x: pixelMetric("menuhmargin") + pixelMetric("menupanelwidth")
        y: pixelMetric("menuvmargin")

        text: !!parent && parent.text
        property string textAndShorcut: text + (properties.shortcut ? "\t" + properties.shortcut : "")
        contentWidth: textWidth(textAndShorcut)
        contentHeight: textHeight(textAndShorcut)

        enabled: !!parent && parent.enabled
        selected: !!parent && parent.selected
        on: !!menuItem && !!menuItem["checkable"] && menuItem.checked

        hints: { "showUnderlined": showUnderlined }

        properties: {
            "checkable": !!menuItem && !!menuItem["checkable"],
            "exclusive": !!menuItem && !!menuItem["exclusiveGroup"],
            "shortcut": !!menuItem && menuItem["shortcut"] || "",
            "isSubmenu": isSubmenu,
            "scrollerDirection": scrollerDirection,
            "icon": !!menuItem && menuItem.__icon
        }

        Accessible.role: Accessible.MenuItem
        Accessible.name: StyleHelpers.removeMnemonics(text)
    }

    property Component scrollerStyle: Style {
        padding { left: 0; right: 0; top: 0; bottom: 0 }
        property bool scrollToClickedPosition: false
        property Component frame: Item { visible: false }
        property Component corner: Item { visible: false }
        property Component __scrollbar: Item { visible: false }
        property bool useScrollers: true
    }
}
