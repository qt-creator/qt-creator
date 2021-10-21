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

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

PropertyEditorPane {
    id: itemPane

    ComponentSection {
        showState: true
    }

    GeometrySection {}

    Section {
        caption: qsTr("Visibility")
        anchors.left: parent.left
        anchors.right: parent.right

        SectionLayout {
            PropertyLabel { text: qsTr("Visibility") }

            SecondColumnLayout {
                CheckBox {
                    text: qsTr("Visible")
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.visible
                }

                Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

                CheckBox {
                    text: qsTr("Clip")
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.clip
                }

                ExpandingSpacer {}
            }

            PropertyLabel { text: qsTr("Opacity") }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    sliderIndicatorVisible: true
                    backendValue: backendValues.opacity
                    decimals: 2
                    minimumValue: 0
                    maximumValue: 1
                    hasSlider: true
                    stepSize: 0.1
                }

                ExpandingSpacer {}
            }
        }
    }

    Item {
        height: 4
        width: 4
    }

    StudioControls.TabBar {
        id: tabBar

        anchors.left: parent.left
        anchors.right: parent.right

        StudioControls.TabButton {
            text: backendValues.className.value
        }
        StudioControls.TabButton {
            text: qsTr("Layout")
        }
    }

    StackLayout {
        id: tabView
        property int currentHeight: children[tabView.currentIndex].implicitHeight
        property int extraHeight: 40

        anchors.left: parent.left
        anchors.right: parent.right
        currentIndex: tabBar.currentIndex
        height: currentHeight + extraHeight

        Column {
            width: parent.width

            Loader {
                id: specificsTwo

                property string theSource: specificQmlData

                anchors.left: parent.left
                anchors.right: parent.right
                visible: theSource !== ""
                sourceComponent: specificQmlComponent

                onTheSourceChanged: {
                    active = false
                    active = true
                }
            }

            Loader {
                id: specificsOne
                anchors.left: parent.left
                anchors.right: parent.right
                source: specificsUrl
                visible: specificsOne.source.toString() !== ""
            }

            AdvancedSection {
                expanded: false
            }

            LayerSection {
                expanded: false
            }
        }

        Column {
            width: parent.width

            LayoutSection {}

            MarginSection {
                visible: anchorBackend.isInLayout
                backendValueTopMargin: backendValues.Layout_topMargin
                backendValueBottomMargin: backendValues.Layout_bottomMargin
                backendValueLeftMargin: backendValues.Layout_leftMargin
                backendValueRightMargin: backendValues.Layout_rightMargin
                backendValueMargins: backendValues.Layout_margins
            }

            AlignDistributeSection {
                visible: !anchorBackend.isInLayout
            }
        }
    }
}
