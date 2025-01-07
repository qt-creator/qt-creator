// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts

import QuickQanava as Qan
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls

import NodeGraphEditorBackend

BaseNode {
    id: root

    readonly property QtObject components: QtObject {
        readonly property Component checkBox: Component {
            StudioControls.CheckBox {
                id: checkBox

                actionIndicatorVisible: false

                //checked: root.value.data

                onCheckedChanged: () => {
                    root.value.data = checkBox.checked;
                }
            }
        }
        readonly property Component colorPicker: Component {
            Rectangle {
                id: colorPicker

                border.color: "black"
                border.width: 2
                color: root.value.data
                height: 32
                width: 32

                Component.onCompleted: {
                    if (!root.value.data) {
                        root.value.data = "red";
                    }
                }

                MouseArea {
                    anchors.fill: parent

                    onClicked: colorEditorPopup.open(colorPicker)
                }

                ColorEditorPopup {
                    id: colorEditorPopup

                    currentColor: root.value.data

                    onActivateColor: color => {
                        root.value.data = color;
                    }
                }
            }
        }
        readonly property Component imageSource: Component {
            StudioControls.ComboBox {
                id: imageSource

                actionIndicatorVisible: false
                model: fileModel.model
                textRole: "fileName"
                valueRole: "relativeFilePath"

                onCurrentValueChanged: () => {
                    root.value.data = `image://qmldesigner_nodegrapheditor/${imageSource.currentValue}`;
                }

                HelperWidgets.FileResourcesModel {
                    id: fileModel

                    filter: "*.png *.gif *.jpg *.bmp *.jpeg *.svg *.pbm *.pgm *.ppm *.xbm *.xpm *.hdr *.ktx *.webp"
                    modelNodeBackendProperty: modelNodeBackend
                }
            }
        }
        readonly property Component materialAlphaMode: Component {
            StudioControls.ComboBox {
                id: materialAlphaMode

                actionIndicatorVisible: false
                model: [
                    {
                        text: "Default",
                        value: 0
                    },
                    {
                        text: "Blend",
                        value: 2
                    },
                    {
                        text: "Opaque",
                        value: 3
                    },
                    {
                        text: "Mask",
                        value: 1
                    },
                ]
                textRole: "text"
                valueRole: "value"

                onCurrentValueChanged: () => {
                    root.value.data = materialAlphaMode.currentValue;
                }
            }
        }
        readonly property Component materialBlendMode: Component {
            StudioControls.ComboBox {
                id: materialBlendMode

                actionIndicatorVisible: false
                model: [
                    {
                        text: "SourceOver",
                        value: 0
                    },
                    {
                        text: "Screen",
                        value: 1
                    },
                    {
                        text: "Multiply",
                        value: 2
                    },
                ]
                textRole: "text"
                valueRole: "value"

                onCurrentValueChanged: () => {
                    root.value.data = materialBlendMode.currentValue;
                }
            }
        }
        readonly property Component materialChannel: Component {
            StudioControls.ComboBox {
                id: materialChannel

                actionIndicatorVisible: false
                model: [
                    {
                        text: "R",
                        value: 0
                    },
                    {
                        text: "G",
                        value: 1
                    },
                    {
                        text: "B",
                        value: 2
                    },
                    {
                        text: "A",
                        value: 3
                    },
                ]
                textRole: "text"
                valueRole: "value"

                onCurrentValueChanged: () => {
                    root.value.data = materialChannel.currentValue;
                }
            }
        }
        readonly property Component materialLighting: Component {
            StudioControls.ComboBox {
                id: materialLighting

                actionIndicatorVisible: false
                model: [
                    {
                        text: "NoLighting",
                        value: 0
                    },
                    {
                        text: "FragmentLighting",
                        value: 1
                    },
                ]
                textRole: "text"
                valueRole: "value"

                onCurrentValueChanged: () => {
                    root.value.data = materialLighting.currentValue;
                }
            }
        }
        readonly property Component realSpinBox: Component {
            StudioControls.RealSpinBox {
                id: realSpinBox

                actionIndicatorVisible: false
                realValue: root.value.data

                onRealValueModified: () => {
                    root.value.data = realSpinBox.realValue;
                }
            }
        }
        readonly property Component textureFiltering: Component {
            StudioControls.ComboBox {
                id: textureFiltering

                actionIndicatorVisible: false
                model: [
                    {
                        text: "None",
                        value: 0
                    },
                    {
                        text: "Nearest",
                        value: 1
                    },
                    {
                        text: "Linear",
                        value: 2
                    },
                ]
                textRole: "text"
                valueRole: "value"

                onCurrentValueChanged: () => {
                    root.value.data = textureFiltering.currentValue;
                }
            }
        }
        readonly property Component textureMappingMode: Component {
            StudioControls.ComboBox {
                id: textureMappingMode

                actionIndicatorVisible: false
                model: [
                    {
                        text: "UV",
                        value: 0
                    },
                    {
                        text: "Environment",
                        value: 1
                    },
                    {
                        text: "LightProbe",
                        value: 2
                    },
                ]
                textRole: "text"
                valueRole: "value"

                onCurrentValueChanged: () => {
                    root.value.data = textureMappingMode.currentValue;
                }
            }
        }
        readonly property Component textureTilingMode: Component {
            StudioControls.ComboBox {
                id: textureTilingMode

                actionIndicatorVisible: false
                model: [
                    {
                        text: "Repeat",
                        value: 3
                    },
                    {
                        text: "ClampToEdge",
                        value: 1
                    },
                    {
                        text: "MirroredRepeat",
                        value: 2
                    },
                ]
                textRole: "text"
                valueRole: "value"

                onCurrentValueChanged: () => {
                    root.value.data = textureTilingMode.currentValue;
                }
            }
        }
    }
    readonly property QtObject value: QtObject {
        property var data: undefined
        property var reset: undefined
        property string type: "undefined"
    }
}
