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

Item {
    width: grid.width
    height: grid.height
    id: originControl


    property variant backendValue
    property color borderColorSelected: colorLogic.textColor
    property color borderColor: creatorTheme.QmlDesignerBorderColor

    property bool showTranslateCheckBox: true

    readonly property color selectedColor: creatorTheme.QmlDesignerBackgroundColorDarkAlternate
    readonly property color unselectedColor: creatorTheme.QmlDesignerBackgroundColorDarker

    ExtendedFunctionButton {
        backendValue: originControl.backendValue
        visible: originControl.enabled
        anchors.left: grid.right
    }

    ColorLogic {
        id: colorLogic
        backendValue: originControl.backendValue
        onValueFromBackendChanged: {
            var enumString = originControl.backendValue.enumeration;
            if (enumString === "")
                enumString = originControl.backendValue.value
            grid.select(enumString);
        }
    }

    Grid {
        rows: 3
        columns: 3
        spacing: 5

        id: grid

        function setValue(myValue) {
            originControl.backendValue.setEnumeration("Item", myValue)
        }

        function select(myValue) {

            for (var i = 0; i < grid.children.length; i++) {
                grid.children[i].selected = false
            }

            if (myValue === "TopLeft") {
                grid.children[0].selected = true
            } else if (myValue === "Top") {
                grid.children[1].selected = true
            } else if (myValue === "TopRight") {
                grid.children[2].selected = true
            } else if (myValue === "Left") {
                grid.children[3].selected = true
            } else if (myValue === "Center") {
                grid.children[4].selected = true
            }  else if (myValue === "Right") {
                grid.children[5].selected = true
            }  else if (myValue === "BottomLeft") {
                grid.children[6].selected = true
            }  else if (myValue === "Bottom") {
                grid.children[7].selected = true
            }  else if (myValue === "BottomRight") {
                grid.children[8].selected = true
            }
        }


        Rectangle {
            property bool selected: false
            id: topLeft
            width: 15
            height: 15
            color: selected ? selectedColor : unselectedColor
            border.width: selected ? 2 : 1
            border.color: selected ? originControl.borderColorSelected : originControl.borderColor
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    grid.setValue("TopLeft")
                }
            }
        }

        Rectangle {
            property bool selected: false
            width: topLeft.width
            height: topLeft.height
            color: selected ? selectedColor : unselectedColor
            border.width: selected ? 2 : 1
            border.color: selected ? originControl.borderColorSelected : originControl.borderColor
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    grid.setValue("Top")
                }
            }
        }

        Rectangle {
            property bool selected: false
            width: topLeft.width
            height: topLeft.height
            color: selected ? selectedColor : unselectedColor
            border.width: selected ? 2 : 1
            border.color: selected ? originControl.borderColorSelected : originControl.borderColor
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    grid.setValue("TopRight")
                }
            }
        }

        Rectangle {
            property bool selected: false
            width: topLeft.width
            height: topLeft.height
            color: selected ? selectedColor : unselectedColor
            border.width: selected ? 2 : 1
            border.color: selected ? originControl.borderColorSelected : originControl.borderColor
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    grid.setValue("Left")
                }
            }
        }

        Rectangle {
            property bool selected: false
            width: topLeft.width
            height: topLeft.height
            color: selected ? selectedColor : unselectedColor
            border.width: selected ? 2 : 1
            border.color: selected ? originControl.borderColorSelected : originControl.borderColor
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    grid.setValue("Center")
                }
            }
        }

        Rectangle {
            property bool selected: false
            width: topLeft.width
            height: topLeft.height
            color: selected ? selectedColor : unselectedColor
            border.width: selected ? 2 : 1
            border.color: selected ? originControl.borderColorSelected : originControl.borderColor
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    grid.setValue("Right")
                }
            }
        }

        Rectangle {
            property bool selected: false
            width: topLeft.width
            height: topLeft.height
            color: selected ? selectedColor : unselectedColor
            border.width: selected ? 2 : 1
            border.color: selected ? originControl.borderColorSelected : originControl.borderColor
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    grid.setValue("BottomLeft")
                }
            }
        }

        Rectangle {
            property bool selected: false
            width: topLeft.width
            height: topLeft.height
            color: selected ? selectedColor : unselectedColor
            border.width: selected ? 2 : 1
            border.color: selected ? originControl.borderColorSelected : originControl.borderColor
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    grid.setValue("Bottom")
                }
            }
        }

        Rectangle {
            property bool selected: false
            width: topLeft.width
            height: topLeft.height
            color: selected ? selectedColor : unselectedColor
            border.width: selected ? 2 : 1
            border.color: selected ? originControl.borderColorSelected : originControl.borderColor
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    grid.setValue("BottomRight")
                }
            }
        }
    }
}
