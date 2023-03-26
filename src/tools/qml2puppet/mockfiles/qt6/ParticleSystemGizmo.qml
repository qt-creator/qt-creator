// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 6.0
import QtQuick3D 6.0

IconGizmo {
    id: particleSystemGizmo

    property Node activeParticleSystem: null

    iconSource: "qrc:///qtquickplugin/mockfiles/images/editor_particlesystem.png"
    iconOpacity: selected || activeParticleSystem == targetNode ? 0.2 : 1
}
