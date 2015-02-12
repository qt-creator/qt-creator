/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLPROFILEREVENTTYPES_H
#define QMLPROFILEREVENTTYPES_H

#include <QtGlobal>

namespace QmlDebug {

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
const int QML_MIN_LEVEL = 1; // Set to 0 to remove the empty line between models in the timeline

// Shorthand for all QML and JS range features.
const quint64 QML_JS_RANGE_FEATURES = (1 << ProfileCompiling) |
                                      (1 << ProfileCreating) |
                                      (1 << ProfileBinding) |
                                      (1 << ProfileHandlingSignal) |
                                      (1 << ProfileJavaScript);
}

} // namespace QmlDebug

#endif //QMLPROFILEREVENTTYPES_H
