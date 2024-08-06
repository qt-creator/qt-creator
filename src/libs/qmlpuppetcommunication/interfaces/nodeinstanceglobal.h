// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QByteArray>
#include <QList>
#include <QVarLengthArray>

#include <vector>

namespace QmlDesigner {

using PropertyName = QByteArray;
using PropertyNameView = QByteArrayView;
using PropertyNameList = QList<PropertyName>;
using PropertyNameViews = QVarLengthArray<PropertyNameView, 64>;
using PropertyNames = std::vector<PropertyName>;
using TypeName = QByteArray;

enum class AuxiliaryDataType {
    None,
    Temporary,
    Document,
    NodeInstancePropertyOverwrite,
    NodeInstanceAuxiliary,
    Persistent
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
    ShowLookAt,
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
    GetNodeAtMainScenePos,
    SetBakeLightsView3D,
    SplitViewToggle,
    MaterialOverride,
    ShowWireframe,
    FlyModeToggle,
    EditCameraRotation,
    EditCameraMove,
    EditCameraStopAllMoves,
    SetLastSceneEnvData,
    Import3dUpdatePreviewImage,
    Import3dRotatePreviewModel,
    Import3dAddPreviewModel,
    Import3dSetCurrentPreviewModel
};

constexpr bool isNanotraceEnabled()
{
#ifdef NANOTRACE_DESIGNSTUDIO_ENABLED
    return true;
#else
    return false;
#endif
}
}
