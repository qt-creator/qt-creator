// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
pragma Singleton

import QtQuick

QtObject {
    readonly property Component baseColor: Component {
        BaseColor {
        }
    }
    readonly property Component checkBox: Component {
        CheckBox {
        }
    }
    readonly property Component color: Component {
        Color {
        }
    }
    readonly property Component comboBox: Component {
        ComboBox {
        }
    }
    readonly property Component material: Component {
        Material {
        }
    }
    readonly property Component metalness: Component {
        Metalness {
        }
    }
    readonly property Component realSpinBox: Component {
        RealSpinBox {
        }
    }
    readonly property Component roughness: Component {
        Roughness {
        }
    }
    readonly property Component occlusion: Component {
        Occlusion {
        }
    }
    readonly property Component texture: Component {
        Texture {
        }
    }
}
