/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

import QtQuick.Window
import QtQuick.Controls

import QtQuick
import QtQuick.Layouts
import StudioControls as SC
import StudioTheme as StudioTheme

Item {
    width: DialogValues.detailsPaneWidth

    Component.onCompleted: {
        dialogBox.detailsLoaded = true;
    }

    Component.onDestruction: {
        dialogBox.detailsLoaded = false;
    }

    Rectangle {
        color: DialogValues.darkPaneColor
        anchors.fill: parent

        Item {
            x: DialogValues.detailsPanePadding                               // left padding
            width: parent.width - DialogValues.detailsPanePadding * 2        // right padding
            height: parent.height

            Column {
                anchors.fill: parent
                spacing: DialogValues.defaultPadding

                Text {
                    id: detailsHeading
                    text: qsTr("Details")
                    height: DialogValues.dialogTitleTextHeight
                    width: parent.width;
                    font.weight: Font.DemiBold
                    font.pixelSize: DialogValues.paneTitlePixelSize
                    lineHeight: DialogValues.paneTitleLineHeight
                    lineHeightMode: Text.FixedHeight
                    color: DialogValues.textColor
                    verticalAlignment: Qt.AlignVCenter
                }

                Flickable {
                    width: parent.width
                    height: parent.height - detailsHeading.height - DialogValues.defaultPadding

                    contentWidth: parent.width
                    contentHeight: scrollContent.height
                    boundsBehavior: Flickable.StopAtBounds
                    clip: true

                    ScrollBar.vertical:  ScrollBar {
                        implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                                                implicitContentWidth + leftPadding + rightPadding)
                        implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                                                 implicitContentHeight + topPadding + bottomPadding)

                        property bool scrollBarVisible: parent.childrenRect.height > parent.height

                        minimumSize: orientation == Qt.Horizontal ? height / width : width / height

                        orientation: Qt.Vertical
                        policy: scrollBarVisible ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
                        x: parent.width - width
                        y: 0
                        height: parent.availableHeight
                                - (parent.bothVisible ? parent.horizontalThickness : 0)
                        padding: 0

                        background: Rectangle {
                            color: StudioTheme.Values.themeScrollBarTrack
                        }

                        contentItem: Rectangle {
                            implicitWidth: StudioTheme.Values.scrollBarThickness
                            color: StudioTheme.Values.themeScrollBarHandle
                        }
                    } // ScrollBar

                    Column {
                        id: scrollContent
                        width: parent.width - DialogValues.detailsPanePadding
                        height: DialogValues.detailsScrollableContentHeight
                        spacing: DialogValues.defaultPadding

                        SC.TextField {
                            id: projectNameTextField
                            actionIndicatorVisible: false
                            translationIndicatorVisible: false
                            text: dialogBox.projectName
                            width: parent.width
                            color: DialogValues.textColor
                            selectByMouse: true

                            onEditingFinished: {
                                text = text.charAt(0).toUpperCase() + text.slice(1)
                            }

                            font.pixelSize: DialogValues.defaultPixelSize
                        }

                        Binding {
                            target: dialogBox
                            property: "projectName"
                            value: projectNameTextField.text
                        }

                        Item { width: parent.width; height: DialogValues.narrowSpacing(11) }

                        RowLayout { // Project location
                            width: parent.width

                            SC.TextField {
                                Layout.fillWidth: true
                                id: projectLocationTextField
                                actionIndicatorVisible: false
                                translationIndicatorVisible: false
                                text: dialogBox.projectLocation
                                color: DialogValues.textColor
                                selectByMouse: true
                                font.pixelSize: DialogValues.defaultPixelSize
                            }

                            Binding {
                                target: dialogBox
                                property: "projectLocation"
                                value: projectLocationTextField.text
                            }

                            SC.AbstractButton {
                                implicitWidth: 30
                                iconSize: 20
                                visible: true
                                buttonIcon: "…"
                                iconFont: StudioTheme.Constants.font

                                onClicked: {
                                    var newLocation = dialogBox.chooseProjectLocation()
                                    if (newLocation)
                                        projectLocationTextField.text = newLocation
                                }
                            } // SC.AbstractButton
                        } // Project location RowLayout

                        Item { width: parent.width; height: DialogValues.narrowSpacing(7) }

                        RowLayout { // StatusMessage
                            width: parent.width
                            spacing: 0

                            Image {
                                id: statusIcon
                                Layout.alignment: Qt.AlignTop
                                asynchronous: false
                            }

                            Text {
                                id: statusMessage
                                text: dialogBox.statusMessage
                                font.pixelSize: DialogValues.defaultPixelSize
                                lineHeight: DialogValues.defaultLineHeight
                                lineHeightMode: Text.FixedHeight
                                color: DialogValues.textColor
                                wrapMode: Text.Wrap
                                elide: Text.ElideRight
                                maximumLineCount: 3
                                Layout.fillWidth: true

                                states: [
                                    State {
                                        name: "warning"
                                        when: dialogBox.statusType === "warning"
                                        PropertyChanges {
                                            target: statusMessage
                                            color: DialogValues.textWarning
                                        }
                                        PropertyChanges {
                                            target: statusIcon
                                            source: "image://newprojectdialog_library/status-warning"
                                        }
                                    },

                                    State {
                                        name: "error"
                                        when: dialogBox.statusType === "error"
                                        PropertyChanges {
                                            target: statusMessage
                                            color: DialogValues.textError
                                        }
                                        PropertyChanges {
                                            target: statusIcon
                                            source: "image://newprojectdialog_library/status-error"
                                        }
                                    }
                                ]
                            } // Text
                        } // RowLayout

                        SC.CheckBox {
                            id: defaultLocationCheckbox
                            actionIndicatorVisible: false
                            text: qsTr("Use as default project location")
                            checked: false
                            font.pixelSize: DialogValues.defaultPixelSize
                        }

                        Binding {
                            target: dialogBox
                            property: "saveAsDefaultLocation"
                            value: defaultLocationCheckbox.checked
                        }

                        Rectangle { width: parent.width; height: 1; color: DialogValues.dividerlineColor }

                        SC.ComboBox {   // Screen Size ComboBox
                            id: screenSizeComboBox
                            actionIndicatorVisible: false
                            currentIndex: -1
                            model: screenSizeModel
                            textRole: "display"
                            width: parent.width
                            font.pixelSize: DialogValues.defaultPixelSize

                            onActivated: (index) => {
                                dialogBox.setScreenSizeIndex(index);

                                var size = screenSizeModel.screenSizes(index);
                                widthField.realValue = size.width;
                                heightField.realValue = size.height;
                            }

                            Connections {
                                target: screenSizeModel
                                function onModelReset() {
                                    var newIndex = screenSizeComboBox.currentIndex > -1
                                            ? screenSizeComboBox.currentIndex
                                            : dialogBox.screenSizeIndex()

                                    screenSizeComboBox.currentIndex = newIndex
                                    screenSizeComboBox.activated(newIndex)
                                }
                            }
                        } // Screen Size ComboBox

                        GridLayout { // orientation + width + height
                            width: parent.width
                            height: 85

                            columns: 4
                            rows: 2

                            columnSpacing: 10
                            rowSpacing: 10

                            // header items
                            Text {
                                text: qsTr("Width")
                                font.pixelSize: DialogValues.defaultPixelSize
                                lineHeight: DialogValues.defaultLineHeight
                                lineHeightMode: Text.FixedHeight
                                color: DialogValues.textColor
                            }

                            Text {
                                text: qsTr("Height")
                                font.pixelSize: DialogValues.defaultPixelSize
                                lineHeight: DialogValues.defaultLineHeight
                                lineHeightMode: Text.FixedHeight
                                color: DialogValues.textColor
                            }

                            Item { Layout.fillWidth: true }

                            Text {
                                text: qsTr("Orientation")
                                font.pixelSize: DialogValues.defaultPixelSize
                                lineHeight: DialogValues.defaultLineHeight
                                lineHeightMode: Text.FixedHeight
                                color: DialogValues.textColor
                            }

                            // content items
                            SC.RealSpinBox {
                                id: widthField
                                actionIndicatorVisible: false
                                implicitWidth: 70
                                labelColor: DialogValues.textColor
                                realFrom: 100
                                realTo: 100000
                                realValue: 100
                                realStepSize: 10
                                font.pixelSize: DialogValues.defaultPixelSize

                                onRealValueChanged: {
                                    var height = heightField.realValue
                                    var width = realValue

                                    if (width >= height)
                                        orientationButton.setHorizontal()
                                    else
                                        orientationButton.setVertical()
                                }
                            } // Width Text Field

                            Binding {
                                target: dialogBox
                                property: "customWidth"
                                value: widthField.realValue
                            }

                            SC.RealSpinBox {
                                id: heightField
                                actionIndicatorVisible: false
                                implicitWidth: 70
                                labelColor: DialogValues.textColor
                                realFrom: 100
                                realTo: 100000
                                realValue: 100
                                realStepSize: 10
                                font.pixelSize: DialogValues.defaultPixelSize

                                onRealValueChanged: {
                                    var height = realValue
                                    var width = widthField.realValue

                                    if (width >= height)
                                        orientationButton.setHorizontal()
                                    else
                                        orientationButton.setVertical()
                                }
                            } // Height Text Field

                            Binding {
                                target: dialogBox
                                property: "customHeight"
                                value: heightField.realValue
                            }

                            Item { Layout.fillWidth: true }

                            Button {
                                id: orientationButton
                                implicitWidth: 100
                                implicitHeight: 50

                                checked: false
                                hoverEnabled: false
                                background: Rectangle {
                                    width: parent.width
                                    height: parent.height
                                    color: "transparent"

                                    Row {
                                        Item {
                                            width: orientationButton.width / 2
                                            height: orientationButton.height
                                            Rectangle {
                                                id: horizontalBar
                                                color: "white"
                                                width: parent.width
                                                height: orientationButton.height / 2
                                                anchors.verticalCenter: parent.verticalCenter
                                            }
                                        }

                                        Item {
                                            width: orientationButton.width / 4
                                            height: orientationButton.height
                                        }

                                        Rectangle {
                                            id: verticalBar
                                            width: orientationButton.width / 4
                                            height: orientationButton.height
                                            color: "white"
                                        }
                                    }
                                }

                                onClicked: {
                                    if (widthField.realValue && heightField.realValue) {
                                        [widthField.realValue, heightField.realValue] = [heightField.realValue, widthField.realValue];
                                        checked = !checked
                                    }
                                }

                                function setHorizontal() {
                                    checked = false
                                    horizontalBar.color = DialogValues.textColorInteraction
                                    verticalBar.color = "white"
                                }

                                function setVertical() {
                                    checked = true
                                    horizontalBar.color = "white"
                                    verticalBar.color = DialogValues.textColorInteraction
                                }
                            } // Orientation button

                        } // GridLayout: orientation + width + height

                        Rectangle { width: parent.width; height: 1; color: DialogValues.dividerlineColor }

                        SC.CheckBox {
                            id: useQtVirtualKeyboard
                            actionIndicatorVisible: false
                            text: qsTr("Use Qt Virtual Keyboard")
                            font.pixelSize: DialogValues.defaultPixelSize
                            checked: dialogBox.useVirtualKeyboard
                            visible: dialogBox.haveVirtualKeyboard
                        }

                        RowLayout { // Target Qt Version
                            width: parent.width
                            visible: dialogBox.haveTargetQtVersion

                            Text {
                                text: "Target Qt Version:"
                                font.pixelSize: DialogValues.defaultPixelSize
                                lineHeight: DialogValues.defaultLineHeight
                                lineHeightMode: Text.FixedHeight
                                color: DialogValues.textColor
                            }

                            SC.ComboBox {   // Target Qt Version ComboBox
                                id: qtVersionComboBox
                                actionIndicatorVisible: false
                                implicitWidth: 70
                                Layout.alignment: Qt.AlignRight
                                currentIndex: 1
                                font.pixelSize: DialogValues.defaultPixelSize

                                model: ListModel {
                                    ListElement {
                                        name: "Qt 5"
                                    }
                                    ListElement {
                                        name: "Qt 6"
                                    }
                                }

                                onActivated: (index) => {
                                    dialogBox.setTargetQtVersion(index)
                                }
                            } // Target Qt Version ComboBox
                        } // RowLayout

                        Binding {
                            target: dialogBox
                            property: "useVirtualKeyboard"
                            value: useQtVirtualKeyboard.checked
                        }
                    } // ScrollContent Column
                } // ScrollView

            } // Column
        } // Item
    }
}
