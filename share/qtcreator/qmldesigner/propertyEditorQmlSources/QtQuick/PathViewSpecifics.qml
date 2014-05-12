/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

import QtQuick 2.1
import HelperWidgets 2.0
import QtQuick.Layouts 1.0

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Path View")

        SectionLayout {

            Label {
                text: qsTr("Drag margin")
                toolTip: qsTr("Drag margin")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.dragMargin
                    minimumValue: 100;
                    maximumValue: 100;
                    decimals: 0
                }

                ExpandingSpacer {

                }
            }

            Label {
                text: qsTr("Flick deceleration")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.flickDeceleration
                    minimumValue: 0;
                    maximumValue: 1000;
                    decimals: 0
                }

                ExpandingSpacer {

                }
            }

            Label {
                text: qsTr("Interactive")
                toolTip: qsTr("A user cannot drag or flick a PathView that is not interactive.")
            }

            SecondColumnLayout {
                CheckBox {
                    backendValue: backendValues.interactive
                }

                ExpandingSpacer {

                }
            }

            Label {
                text: qsTr("Offset")
                toolTip: qsTr("Specifies how far along the path the items are from their initial positions. This is a real number that ranges from 0.0 to the count of items in the model.")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.offset
                    minimumValue: 0;
                    maximumValue: 1000;
                    decimals: 0
                }

                ExpandingSpacer {

                }
            }

            Label {
                text: qsTr("Item count")
                toolTip: qsTr("pathItemCount: number of items visible on the path at any one time.")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.pathItemCount
                    minimumValue: -1;
                    maximumValue: 1000;
                    decimals: 0
                }

                ExpandingSpacer {

                }
            }
        }
    }
    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Path View Highlight")

        SectionLayout {

            Label {
                text: qsTr("Range")
                toolTip: qsTr("Highlight range")
            }

            SecondColumnLayout {
                ComboBox {
                    model: ["NoHighlightRange", "ApplyRange", "StrictlyEnforceRange"]
                    backendValue: backendValues.highlightRangeMode
                    Layout.fillWidth: true
                    scope: "PathView"
                }

                ExpandingSpacer {

                }
            }


            Label {
                text: qsTr("Move duration")
                toolTip: qsTr("Move animation duration of the highlight delegate.")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.highlightMoveDuration
                    minimumValue: 0;
                    maximumValue: 1000;
                    decimals: 0
                }

                ExpandingSpacer {

                }
            }

            Label {
                text: qsTr("Preferred begin")
                toolTip: qsTr("Preferred highlight begin - must be smaller than Preferred end. Note that the user has to add a highlight component.")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.preferredHighlightBegin
                    minimumValue: 0;
                    maximumValue: 1;
                    decimals: 2
                }

                ExpandingSpacer {

                }
            }

            Label {
                text: qsTr("Preferred end")
                toolTip: qsTr("Preferred highlight end - must be larger than Preferred begin. Note that the user has to add a highlight component.")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.preferredHighlightEnd
                    minimumValue: 0;
                    maximumValue: 1;
                    decimals: 2
                }

                ExpandingSpacer {

                }
            }

            Label {
                text: qsTr("Follows current")
                toolTip: qsTr("Determines whether the highlight is managed by the view.")
            }

            SecondColumnLayout {
                CheckBox {
                    backendValue: backendValues.highlightFollowsCurrentItem
                }

                ExpandingSpacer {

                }
            }

        }
    }
}
