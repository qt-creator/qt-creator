// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace QmlProfiler {

enum Message {
    Event,
    RangeStart,
    RangeData,
    RangeLocation,
    RangeEnd,
    Complete,
    PixmapCacheEvent,
    SceneGraphFrame,
    MemoryAllocation,
    DebugMessage,
    Quick3DEvent,

    MaximumMessage
};

enum EventType {
    FramePaint, // unused
    Mouse,
    Key,
    AnimationFrame, // new Qt5 paint events
    EndTrace,
    StartTrace,

    MaximumEventType
};

enum Quick3DEventType {
    Quick3DRenderFrame,
    Quick3DSynchronizeFrame,
    Quick3DPrepareFrame,
    Quick3DMeshLoad,
    Quick3DCustomMeshLoad,
    Quick3DTextureLoad,
    Quick3DParticleUpdate,
    Quick3DGenerateShader,
    Quick3DLoadShader,
    MaximumQuick3DFrameType,
    NumQuick3DRenderThreadFrameTypes = Quick3DParticleUpdate,
    NumQuick3DGUIThreadFrameTypes = MaximumQuick3DFrameType - NumQuick3DRenderThreadFrameTypes,
};

enum RangeType {
    Painting, // old Qt4 paint events
    Compiling,
    Creating,
    Binding,
    HandlingSignal,
    Javascript,

    MaximumRangeType
};

enum BindingType {
    QmlBinding,
    V8Binding,
    OptimizedBinding,
    QPainterEvent,

    MaximumBindingType
};

enum PixmapEventType {
    PixmapSizeKnown,
    PixmapReferenceCountChanged,
    PixmapCacheCountChanged,
    PixmapLoadingStarted,
    PixmapLoadingFinished,
    PixmapLoadingError,

    MaximumPixmapEventType
};

enum InputEventType {
    InputKeyPress,
    InputKeyRelease,
    InputKeyUnknown,

    InputMousePress,
    InputMouseRelease,
    InputMouseMove,
    InputMouseDoubleClick,
    InputMouseWheel,
    InputMouseUnknown,

    MaximumInputEventType
};

enum SceneGraphFrameType {
    SceneGraphRendererFrame,        // Render Thread
    SceneGraphAdaptationLayerFrame, // Render Thread
    SceneGraphContextFrame,         // Render Thread
    SceneGraphRenderLoopFrame,      // Render Thread
    SceneGraphTexturePrepare,       // Render Thread
    SceneGraphTextureDeletion,      // Render Thread
    SceneGraphPolishAndSync,        // GUI Thread
    SceneGraphWindowsRenderShow,    // Unused
    SceneGraphWindowsAnimations,    // GUI Thread
    SceneGraphPolishFrame,          // GUI Thread

    MaximumSceneGraphFrameType
};

enum MemoryType {
    HeapPage,
    LargeItem,
    SmallItem,

    MaximumMemoryType
};

enum AnimationThread {
    GuiThread,
    RenderThread,

    MaximumAnimationThread
};

enum ProfileFeature {
    ProfileJavaScript,
    ProfileMemory,
    ProfilePixmapCache,
    ProfileSceneGraph,
    ProfileAnimations,
    ProfilePainting,
    ProfileCompiling,
    ProfileCreating,
    ProfileBinding,
    ProfileHandlingSignal,
    ProfileInputEvents,
    ProfileDebugMessages,
    ProfileQuick3D,

    MaximumProfileFeature
};

inline ProfileFeature featureFromRangeType(RangeType range)
{
    switch (range) {
        case Painting:
            return ProfilePainting;
        case Compiling:
            return ProfileCompiling;
        case Creating:
            return ProfileCreating;
        case Binding:
            return ProfileBinding;
        case HandlingSignal:
            return ProfileHandlingSignal;
        case Javascript:
            return ProfileJavaScript;
        default:
            return MaximumProfileFeature;
    }
}

namespace Constants {

// Shorthand for all QML and JS range features.
const quint64 QML_JS_RANGE_FEATURES = (1 << ProfileCompiling) |
                                      (1 << ProfileCreating) |
                                      (1 << ProfileBinding) |
                                      (1 << ProfileHandlingSignal) |
                                      (1 << ProfileJavaScript);
} // namespace Constants

} // namespace QmlProfiler
