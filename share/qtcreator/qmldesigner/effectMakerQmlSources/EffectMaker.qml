// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import EffectMakerBackend

Item {
    id: root

    Column {
        id: col
        anchors.fill: parent
        spacing: 1

        EffectMakerTopBar {

        }

        EffectMakerPreview {

        }

        Rectangle {
            width: parent.width
            height: StudioTheme.Values.toolbarHeight
            color: StudioTheme.Values.themeToolbarBackground

            EffectNodesComboBox {
                mainRoot: root

                anchors.verticalCenter: parent.verticalCenter
            }

            HelperWidgets.AbstractButton {
                anchors.right: parent.right
                anchors.rightMargin: 5
                anchors.verticalCenter: parent.verticalCenter

                style: StudioTheme.Values.viewBarButtonStyle
                buttonIcon: StudioTheme.Constants.code
                tooltip: qsTr("Open Shader in Code Editor")

                onClicked: {} // TODO
            }
        }


        HelperWidgets.ScrollView {
            id: scrollView

            width: parent.width
            height: parent.height - y
            clip: true

            Behavior on contentY {
                id: contentYBehavior
                PropertyAnimation {
                    id: scrollViewAnim
                    easing.type: Easing.InOutQuad
                }
            }

            Column {
                Item {
                    width: scrollView.width
                    height: categories.height

                    Column {
                        Repeater {
                            id: categories
                            width: root.width
                            model: EffectMakerBackend.effectMakerModel

                            delegate: HelperWidgets.Section {
                                id: effectsSection
                                width: root.width
                                caption: model.categoryName
                                category: "EffectMaker"

                                property var effectList: model.effectNames

                                onExpandedChanged: {
                                    effects.visible = expanded // TODO: update
                                }

                                Repeater {
                                    id: effects
                                    model: effectList
                                    width: parent.width
                                    height: parent.height
                                    delegate: EffectNode {
                                        width: parent.width
                                        //height: StudioTheme.Values.checkIndicatorHeight * 2 // TODO: update or remove
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
