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

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import StudioTheme as StudioTheme
import StudioControls as SC

import NewProjectDialog

Item {
    width: DialogValues.dialogWidth
    height: DialogValues.dialogHeight

    Rectangle { // the main dialog panel
        anchors.fill: parent
        color: DialogValues.darkPaneColor

        ColumnLayout {
            anchors.fill: parent

            Layout.alignment: Qt.AlignHCenter
            spacing: 0

            Item { // Header Item
                Layout.fillWidth: true
                implicitHeight: 218

                Column {
                    anchors.fill: parent

                    Item { width: parent.width; height: 74 } // spacer

                    Text {
                        text: qsTr("Welcome to Qt Design Studio. Let's Create Something Wonderful!")
                        font.pixelSize: 32
                        width: parent.width
                        height: 47
                        lineHeight: 49
                        lineHeightMode: Text.FixedHeight
                        color: DialogValues.textColor
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Item { width: parent.width; height: 11 } // spacer

                    Text {
                        width: parent.width
                        text: qsTr("Get started by selecting from Presets or start from empty screen. You may also include your design file.")
                        color: DialogValues.textColor
                        font.pixelSize: DialogValues.paneTitlePixelSize
                        lineHeight: DialogValues.paneTitleLineHeight
                        lineHeightMode: Text.FixedHeight
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            } // Header Item

            Item { // Content Item
                Layout.fillWidth: true
                Layout.fillHeight: true

                RowLayout {
                    x: 35
                    width: parent.width - 70
                    height: parent.height
                    spacing: 0

                    Rectangle { // Left pane
                        color: DialogValues.lightPaneColor
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumWidth: 379 // figured out this number visually
                        Layout.minimumHeight: 326 // figured out this number visually

                        Column {
                            x: DialogValues.defaultPadding                          // left padding
                            width: parent.width - DialogValues.defaultPadding * 2   // right padding
                            height: parent.height

                            Text {
                                text: qsTr("Presets")
                                width: parent.width
                                font.weight: Font.DemiBold
                                font.pixelSize: DialogValues.paneTitlePixelSize
                                lineHeight: DialogValues.paneTitleLineHeight
                                lineHeightMode: Text.FixedHeight
                                color: DialogValues.textColor
                            }

                            NewProjectView {
                                id: projectViewId
                                x: 10                       // left padding
                                width: parent.width - 64    // right padding
                                height: DialogValues.projectViewHeight
                                loader: projectDetailsLoader
                            }

                            Item { height: 5; width: parent.width }

                            Text {
                                id: descriptionText
                                text: dialogBox.projectDescription
                                font.pixelSize: DialogValues.defaultPixelSize
                                lineHeight: DialogValues.defaultLineHeight
                                lineHeightMode: Text.FixedHeight
                                leftPadding: 14
                                width: projectViewId.width
                                color: DialogValues.textColor
                                wrapMode: Text.WordWrap
                                maximumLineCount: 4
                                elide: Text.ElideRight
                            }
                        }
                    }   // Left pane

                    Loader {
                        id: projectDetailsLoader
                        // we need to specify width because the loaded item needs to use parent sizes
                        width: DialogValues.loadedPanesWidth
                        Layout.fillHeight: true
                        source: ""
                    }
                } // RowLayout
            } //Content Item

            Item { // Footer
                implicitHeight: DialogValues.footerHeight
                implicitWidth: parent.width
                RowLayout {
                    anchors.fill: parent
                    spacing: DialogValues.defaultPadding

                    Item { Layout.fillWidth: true }

                    SC.AbstractButton {
                        implicitWidth: DialogValues.dialogButtonWidth
                        width: DialogValues.dialogButtonWidth
                        visible: true
                        buttonIcon: qsTr("Cancel")
                        iconSize: DialogValues.defaultPixelSize
                        iconFont: StudioTheme.Constants.font

                        onClicked: {
                            dialogBox.reject();
                        }
                    }

                    SC.AbstractButton {
                        implicitWidth: DialogValues.dialogButtonWidth
                        width: DialogValues.dialogButtonWidth
                        visible: true
                        buttonIcon: qsTr("Create")
                        iconSize: DialogValues.defaultPixelSize
                        enabled: dialogBox.fieldsValid
                        iconFont: StudioTheme.Constants.font

                        onClicked: {
                            dialogBox.accept();
                        }
                    }
                    Item { implicitWidth: 35 - DialogValues.defaultPadding }
                } // RowLayout
            } // Footer
        } // ColumnLayout
    } // Rectangle
}
