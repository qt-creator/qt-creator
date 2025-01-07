// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import QtQuick3D as QtQuick3D

import QuickQanava as Qan
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls

import NodeGraphEditorBackend

BaseNode {
    id: root

    readonly property QtObject value: QtObject {
        property QtQuick3D.PrincipledMaterial data: QtQuick3D.PrincipledMaterial {
        }
        property string type: "undefined"
    }

    height: 150
    width: 150

    metadata: QtObject {
        property var ports: [
            {
                dock: Qan.NodeItem.Left,
                type: Qan.PortItem.In,
                text: "",
                id: "value_input",
                binding: value => {
                    root.value.data = Qt.binding(() => {
                        return value.data;
                    });
                },
                reset: () => {
                    root.value.data = null;
                }
            }
        ]
    }

    Rectangle {
        anchors.fill: parent
        color: "white"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 3
            spacing: 0

            Item {
                Layout.fillWidth: true
                Layout.margins: 2
                Layout.preferredHeight: 35

                Text {
                    id: label

                    anchors.centerIn: parent
                    text: "Material Viewer"
                }
            }

            Item {
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.margins: 2

                QtQuick3D.View3D {
                    id: view3D

                    anchors.fill: parent

                    camera: QtQuick3D.PerspectiveCamera {
                        clipFar: 1000
                        clipNear: 1
                        eulerRotation.x: -5.71
                        y: 70
                        z: 200
                    }
                    environment: QtQuick3D.SceneEnvironment {
                        antialiasingMode: QtQuick3D.SceneEnvironment.MSAA
                        antialiasingQuality: QtQuick3D.SceneEnvironment.High
                        backgroundMode: QtQuick3D.SceneEnvironment.Transparent
                        clearColor: "#000000"
                    }

                    QtQuick3D.Node {
                        QtQuick3D.DirectionalLight {
                            brightness: 1
                            eulerRotation.x: -26
                            eulerRotation.y: -50
                        }

                        QtQuick3D.Node {
                            y: 60

                            QtQuick3D.Model {
                                id: model

                                eulerRotation.y: 45
                                materials: root.value.data
                                scale: Qt.vector3d(1.7, 1.7, 1.7)
                                source: "#Sphere"
                            }
                        }
                    }
                }
            }
        }
    }
}
