// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QByteArray>
#include <QList>

#include <vector>

namespace QmlDesigner {

using PropertyName = QByteArray;
using PropertyNameList = QList<PropertyName>;
using PropertyNames = std::vector<PropertyName>;
using TypeName = QByteArray;

enum class AuxiliaryDataType {
    None,
    Temporary,
    Document,
    NodeInstancePropertyOverwrite,
    NodeInstanceAuxiliary
};

enum class View3DActionType {
    Empty,
    MoveTool,
    ScaleTool,
    RotateTool,
    FitToView,
    AlignCamerasToView,
    AlignViewToCamera,
    SelectionModeToggle,
    CameraToggle,
    OrientationToggle,
    EditLightToggle,
    ShowGrid,
    ShowSelectionBox,
    ShowIconGizmo,
    ShowCameraFrustum,
    ShowParticleEmitter,
    Edit3DParticleModeToggle,
    ParticlesPlay,
    ParticlesRestart,
    ParticlesSeek,
    SyncEnvBackground,
    GetNodeAtPos,
    SetBakeLightsView3D
};

constexpr bool isNanotraceEnabled()
{
#ifdef NANOTRACE_ENABLED
    return true;
#else
    return false;
#endif
}
}
