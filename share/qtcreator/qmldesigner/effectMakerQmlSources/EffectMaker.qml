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
                width: scrollView.width
                height: compositionRepeater.height
                spacing: 1

                Repeater {
                    id: compositionRepeater

                    width: root.width
                    model: EffectMakerBackend.effectMakerModel

                    delegate: EffectCompositionNode {
                        width: root.width
                    }
                }
            }
        }
    }
}
