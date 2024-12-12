// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import QtQuick3D as QtQuick3D

import QuickQanava as Qan

import NodeGraphEditorBackend

Base {
    id: root

    readonly property QtQuick3D.PrincipledMaterial reset: QtQuick3D.PrincipledMaterial {
    }
    readonly property QtQuick3D.PrincipledMaterial value: QtQuick3D.PrincipledMaterial {
    }

    Layout.preferredHeight: 150
    Layout.preferredWidth: 150

    portsMetaData: QtObject {
        property var pin: [
            {
                id: "principledmaterial_in_basecolor",
                alias: "",
                name: "Base Color",
                type: "nge::BaseColor",
                binding: values => {
                    root.value.baseColor = Qt.binding(() => {
                        return values.color;
                    });
                    root.value.baseColorChannel = Qt.binding(() => {
                        return values.channel;
                    });
                    root.value.baseColorMap = Qt.binding(() => {
                        return values.map;
                    });
                    root.value.baseColorSingleChannelEnabled = Qt.binding(() => {
                        return values.singleChannelEnabled;
                    });
                }
            },
            {
                id: "principledmaterial_in_metalness",
                alias: "",
                name: "Metalness",
                type: "nge::Metalness",
                binding: values => {
                    root.value.metalness = Qt.binding(() => {
                        return values.metalness;
                    });
                    root.value.metalnessChannel = Qt.binding(() => {
                        return values.channel;
                    });
                    root.value.metalnessMap = Qt.binding(() => {
                        return values.map;
                    });
                }
            },
            {
                id: "principledmaterial_in_roughness",
                alias: "",
                name: "Roughness",
                type: "nge::Roughness",
                binding: values => {
                    root.value.roughness = Qt.binding(() => {
                        return values.roughness;
                    });
                    root.value.roughnessChannel = Qt.binding(() => {
                        return values.channel;
                    });
                    root.value.roughnessMap = Qt.binding(() => {
                        return values.map;
                    });
                }
            },
        ]
        property var pout: []
    }

    Component.onCompleted: {
        node.label = "Principled Material";
    }

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
                    materials: root.value
                    scale: Qt.vector3d(1.7, 1.7, 1.7)
                    source: "#Sphere"
                }
            }
        }
    }
}
