/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Item {
    id: root

    property alias text: searchFilterText.text

    function clearSearchFilter()
    {
        searchFilterText.text = "";
    }

    function isEmpty()
    {
        return searchFilterText.text === "";
    }

    implicitWidth: searchFilterText.width
    implicitHeight: searchFilterText.height

    TextField {
        id: searchFilterText

        placeholderText: qsTr("Search")
        placeholderTextColor: StudioTheme.Values.themePlaceholderTextColor
        color: StudioTheme.Values.themeTextColor
        selectionColor: StudioTheme.Values.themeTextSelectionColor
        selectedTextColor: StudioTheme.Values.themeTextSelectedTextColor
        background: Rectangle {
            id: textFieldBackground
            color: StudioTheme.Values.themeControlBackground
            border.color: StudioTheme.Values.themeControlOutline
            border.width: StudioTheme.Values.border

            Behavior on color {
                ColorAnimation {
                    duration: StudioTheme.Values.hoverDuration
                    easing.type: StudioTheme.Values.hoverEasing
                }
            }
        }

        height: StudioTheme.Values.defaultControlHeight

        leftPadding: 32
        rightPadding: 30
        topPadding: 6
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 5
        anchors.rightMargin: 5
        selectByMouse: true
        hoverEnabled: true

        onTextChanged: rootView.handleSearchfilterChanged(text)

        Label {
            text: StudioTheme.Constants.search
            font.family: StudioTheme.Constants.iconFont.family
            font.pixelSize: 16
            anchors.left: parent.left
            anchors.leftMargin: 7
            anchors.verticalCenter: parent.verticalCenter
            color: StudioTheme.Values.themeIconColor
        }

        Rectangle { // x button
            width: 16
            height: 15
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.verticalCenter: parent.verticalCenter
            visible: searchFilterText.text !== ""
            color: xMouseArea.containsMouse ? StudioTheme.Values.themePanelBackground
                                            : "transparent"

            Label {
                text: StudioTheme.Constants.closeCross
                font.family: StudioTheme.Constants.iconFont.family
                font.pixelSize: StudioTheme.Values.myIconFontSize
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                anchors.centerIn: parent
                color: StudioTheme.Values.themeIconColor
            }

            MouseArea {
                id: xMouseArea
                hoverEnabled: true
                anchors.fill: parent
                onClicked: searchFilterText.text = ""
            }
        }

        states: [
            State {
                name: "default"
                when: !searchFilterText.hovered && !searchFilterText.activeFocus
                PropertyChanges {
                    target: textFieldBackground
                    color: StudioTheme.Values.themeControlBackground
                    border.color: StudioTheme.Values.themeControlOutline
                }
                PropertyChanges {
                    target: searchFilterText
                    placeholderTextColor: StudioTheme.Values.themePlaceholderTextColor
                }
            },
            State {
                name: "hover"
                when: searchFilterText.hovered && !searchFilterText.activeFocus
                PropertyChanges {
                    target: textFieldBackground
                    color: StudioTheme.Values.themeControlBackgroundHover
                    border.color: StudioTheme.Values.themeControlOutline
                }

                PropertyChanges {
                    target: searchFilterText
                    placeholderTextColor: StudioTheme.Values.themePlaceholderTextColor
                }
            },
            State {
                name: "edit"
                when: searchFilterText.activeFocus
                PropertyChanges {
                    target: textFieldBackground
                    color: StudioTheme.Values.themeControlBackgroundInteraction
                    border.color: StudioTheme.Values.themeControlOutlineInteraction
                }
                PropertyChanges {
                    target: searchFilterText
                    placeholderTextColor: StudioTheme.Values.themePlaceholderTextColorInteraction
                }
            }
        ]
    }
}
