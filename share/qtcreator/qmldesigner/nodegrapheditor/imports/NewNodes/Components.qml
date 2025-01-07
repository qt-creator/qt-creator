// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
pragma Singleton

import QtQuick

QtObject {
    readonly property QtObject color: QtObject {
        id: color

        readonly property Component component: Component {
            ColorNode {
            }
        }
        readonly property var constructor: (graph, self, data) => {
            const node = graph.insertGroup(color.component);

            node.item.draggable = false;
            node.label = data.text;
            data.bindings(node.item.value);

            return node;
        }
    }
    readonly property QtObject input: QtObject {
        id: input

        readonly property Component component: Component {
            InputNode {
            }
        }
        readonly property var constructor: (graph, self, data) => {
            const node = graph.insertNode(input.component);

            node.item.draggable = false;
            node.item.text = data.text;
            node.item.sourceComponent = node.item.components[data.sourceComponent];
            node.item.value.data = self.value[data.value];
            node.item.value.reset = data.resetValue;
            self.value[data.value] = Qt.binding(() => {
                return node.item.value.data;
            });

            return node;
        }
    }
    readonly property QtObject metalness: QtObject {
        id: metalness

        readonly property Component component: Component {
            MetalnessNode {
            }
        }
        readonly property var constructor: (graph, self, data) => {
            const node = graph.insertGroup(metalness.component);

            node.item.draggable = false;
            node.label = data.text;
            data.bindings(node.item.value);

            return node;
        }
    }
    readonly property QtObject normal: QtObject {
        id: normal

        readonly property Component component: Component {
            NormalNode {
            }
        }
        readonly property var constructor: (graph, self, data) => {
            const node = graph.insertGroup(normal.component);

            node.item.draggable = false;
            node.label = data.text;
            data.bindings(node.item.value);

            return node;
        }
    }
    readonly property QtObject occlusion: QtObject {
        id: occlusion

        readonly property Component component: Component {
            OcclusionNode {
            }
        }
        readonly property var constructor: (graph, self, data) => {
            const node = graph.insertGroup(occlusion.component);

            node.item.draggable = false;
            node.label = data.text;
            data.bindings(node.item.value);

            return node;
        }
    }
    readonly property QtObject opacity: QtObject {
        id: opacity

        readonly property Component component: Component {
            OpacityNode {
            }
        }
        readonly property var constructor: (graph, self, data) => {
            const node = graph.insertGroup(opacity.component);

            node.item.draggable = false;
            node.label = data.text;
            data.bindings(node.item.value);

            return node;
        }
    }
    readonly property QtObject output: QtObject {
        id: output

        readonly property Component component: Component {
            OutputNode {
            }
        }
        readonly property var constructor: (graph, self, data) => {
            const node = graph.insertNode(output.component);

            node.item.draggable = false;
            node.item.text = data.text;
            node.item.value.data = self.value;
            // self.value = Qt.binding(() => {
            //     return node.item.value;
            // });

            return node;
        }
    }
    readonly property QtObject roughness: QtObject {
        id: roughness

        readonly property Component component: Component {
            RoughnessNode {
            }
        }
        readonly property var constructor: (graph, self, data) => {
            const node = graph.insertGroup(roughness.component);

            node.item.draggable = false;
            node.label = data.text;
            data.bindings(node.item.value);

            return node;
        }
    }
}
