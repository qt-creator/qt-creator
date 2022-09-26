/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QtGlobal>

namespace QmlProfiler {

enum Message {
    UndefinedMessage = 0xff,
    Event = 0,
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
    UndefinedEventType = 0xff,
    FramePaint = 0, // unused
    Mouse,
    Key,
    AnimationFrame, // new Qt5 paint events
    EndTrace,
    StartTrace,

    MaximumEventType
};

enum Quick3DEventType {
    UndefinedQuick3DEventType = 0xff,
    Quick3DRenderFrame = 0,
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
    UndefinedRangeType = 0xff,
    Painting = 0, // old Qt4 paint events
    Compiling,
    Creating,
    Binding,
    HandlingSignal,
    Javascript,

    MaximumRangeType
};

enum BindingType {
    UndefinedBindingType = 0xff,
    QmlBinding = 0,
    V8Binding,
    OptimizedBinding,
    QPainterEvent,

    MaximumBindingType
};

enum PixmapEventType {
    UndefinedPixmapEventType = 0xff,
    PixmapSizeKnown = 0,
    PixmapReferenceCountChanged,
    PixmapCacheCountChanged,
    PixmapLoadingStarted,
    PixmapLoadingFinished,
    PixmapLoadingError,

    MaximumPixmapEventType
};

enum InputEventType {
    UndefinedInputEventType = 0xff,
    InputKeyPress = 0,
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
    UndefinedSceheGraphFrameType = 0xff,
    SceneGraphRendererFrame = 0,    // Render Thread
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
    UndefinedMemoryType = 0xff,
    HeapPage = 0,
    LargeItem,
    SmallItem,

    MaximumMemoryType
};

enum AnimationThread {
    UndefinedAnimationThread = 0xff,
    GuiThread = 0,
    RenderThread,

    MaximumAnimationThread
};

enum ProfileFeature {
    UndefinedProfileFeature = 0xff,
    ProfileJavaScript = 0,
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
            return UndefinedProfileFeature;
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
