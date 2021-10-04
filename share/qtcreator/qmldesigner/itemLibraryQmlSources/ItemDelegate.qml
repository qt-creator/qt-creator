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

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuickDesignerTheme 1.0
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Item {
    id: delegateRoot

    property alias textColor: text.color

    signal showContextMenu()

    Rectangle {
        anchors.rightMargin: 1
        anchors.topMargin: 1
        anchors.fill: parent

        color: mouseRegion.containsMouse ? StudioTheme.Values.themeControlBackgroundHover : StudioTheme.Values.themePanelBackground

        Image {
            id: itemIcon // to be set by model

            anchors.top: parent.top
            anchors.topMargin: styleConstants.cellVerticalMargin
            anchors.horizontalCenter: parent.horizontalCenter

            width: itemLibraryIconWidth  // to be set in Qml context
            height: itemLibraryIconHeight   // to be set in Qml context
            source: itemLibraryIconPath     // to be set by model
        }

        Text {
            id: text
            font.pixelSize: Theme.smallFontPixelSize()
            elide: Text.ElideMiddle
            wrapMode: Text.WordWrap
            anchors.top: itemIcon.bottom
            anchors.topMargin: styleConstants.cellVerticalSpacing
            anchors.left: parent.left
            anchors.leftMargin: styleConstants.cellHorizontalMargin
            anchors.right: parent.right
            anchors.rightMargin: styleConstants.cellHorizontalMargin
            anchors.bottom: parent.bottom
            anchors.bottomMargin: styleConstants.cellHorizontalMargin

            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignHCenter
            text: itemName  // to be set by model
            color: StudioTheme.Values.themeTextColor
            renderType: Text.NativeRendering
        }

        ImagePreviewTooltipArea {
            id: mouseRegion
            anchors.fill: parent

            onShowContextMenu: delegateRoot.showContextMenu()
            onPressed: (mouse)=> {
                if (mouse.button === Qt.LeftButton)
                    rootView.startDragAndDrop(itemLibraryEntry, mapToGlobal(mouse.x, mouse.y))
            }
        }
    }
}
