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

FocusScope {
    id: scrollarea
    width: 100
    height: 100

    property int frameWidth: frame ? styleitem.pixelMetric("defaultframewidth") : 0;
    property int contentHeight : content.childrenRect.height
    property int contentWidth: content.childrenRect.width
    property alias color: colorRect.color
    property bool frame: true
    property bool highlightOnFocus: false
    property bool frameAroundContents: styleitem.styleHint("framearoundcontents")
    property alias verticalValue: vscrollbar.value
    property alias horizontalValue: hscrollbar.value

    property alias horizontalScrollBar: hscrollbar
    property alias verticalScrollBar: vscrollbar

    default property alias data: content.data

    property int contentY
    property int contentX

    onContentYChanged: {
        vscrollbar.value = contentY
        wheelarea.verticalValue = contentY
    }
    onContentXChanged: {
        hscrollbar.value = contentX
        wheelarea.horizontalValue = contentX
    }

    Rectangle {
        id: colorRect
        color: "transparent"
        anchors.fill:styleitem
        anchors.margins: frameWidth
    }

    QStyleItem {
        id: styleitem
        elementType: "frame"
        onElementTypeChanged: scrollarea.frameWidth = styleitem.pixelMetric("defaultframewidth");
        sunken: true
        visible: frame
        anchors.fill: parent
        anchors.rightMargin: frame ? (frameAroundContents ? (vscrollbar.visible ? vscrollbar.width + 2 * frameMargins : 0) : -frameWidth) : 0
        anchors.bottomMargin: frame ? (frameAroundContents ? (hscrollbar.visible ? hscrollbar.height + 2 * frameMargins : 0) : -frameWidth) : 0
        anchors.topMargin: frame ? (frameAroundContents ? 0 : -frameWidth) : 0
        property int scrollbarspacing: styleitem.pixelMetric("scrollbarspacing");
        property int frameMargins : frame ? scrollbarspacing : 0
        property int frameoffset: style === "mac" ? -1 : 0
    }

    Item {
        id: flickable
        anchors.fill: styleitem
        anchors.margins: frameWidth
        clip: true

        Item {
            id: content
            x: -scrollarea.contentX
            y: -scrollarea.contentY
        }
    }

    WheelArea {
        id: wheelarea
        anchors.fill: parent
        horizontalMinimumValue: hscrollbar.minimumValue
        horizontalMaximumValue: hscrollbar.maximumValue
        verticalMinimumValue: vscrollbar.minimumValue
        verticalMaximumValue: vscrollbar.maximumValue

        onVerticalValueChanged: {
            contentY = verticalValue
        }

        onHorizontalValueChanged: {
            contentX = horizontalValue
        }
    }

    ScrollBar {
        id: hscrollbar
        orientation: Qt.Horizontal
        property int availableWidth : scrollarea.width - (frame ? (vscrollbar.width) : 0)
        visible: contentWidth > availableWidth
        maximumValue: contentWidth > availableWidth ? scrollarea.contentWidth - availableWidth: 0
        minimumValue: 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: styleitem.frameoffset
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: (frame ? frameWidth : 0)
        anchors.rightMargin: { vscrollbar.visible ? scrollbarExtent : (frame ? 1 : 0) }
        onValueChanged: contentX = value
        property int scrollbarExtent : styleitem.pixelMetric("scrollbarExtent");
    }

    ScrollBar {
        id: vscrollbar
        orientation: Qt.Vertical
        property int availableHeight : scrollarea.height - (frame ? (hscrollbar.height) : 0)
        visible: contentHeight > availableHeight
        maximumValue: contentHeight > availableHeight ? scrollarea.contentHeight - availableHeight : 0
        minimumValue: 0
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: styleitem.style == "mac" ? 1 : 0
        onValueChanged: contentY = value
        anchors.rightMargin: styleitem.frameoffset
        anchors.bottomMargin: hscrollbar.visible ? hscrollbar.height : styleitem.frameoffset
    }

    Rectangle {
        // This is the filled corner between scrollbars
        id: cornerFill
        anchors.left:  vscrollbar.left
        anchors.right: vscrollbar.right
        anchors.top: hscrollbar.top
        anchors.bottom: hscrollbar.bottom
        visible: hscrollbar.visible && vscrollbar.visible
        SystemPalette { id: syspal }
        color: syspal.window
    }

    QStyleItem {
        z: 2
        anchors.fill: parent
        anchors.margins: -3
        anchors.rightMargin: -4
        anchors.bottomMargin: -4
        visible: highlightOnFocus && parent.activeFocus && styleitem.styleHint("focuswidget")
        elementType: "focusframe"
    }
}
