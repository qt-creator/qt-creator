// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlchangeset.h"
#include "qmlmodelnodefacade.h"

namespace QmlDesigner {

class AbstractViewAbstractVieweGroup;
class QmlObjectNode;
class QmlTimeline;

class QMLDESIGNER_EXPORT QmlTimelineKeyframeGroup final : public QmlModelNodeFacade
{
public:
    QmlTimelineKeyframeGroup() = default;

    QmlTimelineKeyframeGroup(const ModelNode &modelNode)
        : QmlModelNodeFacade(modelNode)
    {}

    bool isValid(SL sl = {}) const;
    explicit operator bool() const { return isValid(); }

    static bool isValidQmlTimelineKeyframeGroup(const ModelNode &modelNode, SL sl = {});
    void destroy(SL sl = {});

    ModelNode target(SL sl = {}) const;
    void setTarget(const ModelNode &target, SL sl = {});

    PropertyName propertyName(SL sl = {}) const;
    void setPropertyName(PropertyNameView propertyName, SL sl = {});

    void setValue(const QVariant &value, qreal frame, SL sl = {});
    QVariant value(qreal frame, SL sl = {}) const;

    NodeMetaInfo valueType(SL sl = {}) const;

    qreal currentFrame(SL sl = {}) const;

    bool hasKeyframe(qreal frame, SL sl = {});

    qreal minActualKeyframe(SL sl = {}) const;
    qreal maxActualKeyframe(SL sl = {}) const;

    ModelNode keyframe(qreal position, SL sl = {}) const;

    QList<ModelNode> keyframes(SL sl = {}) const;

    QList<ModelNode> keyframePositions(SL sl = {}) const;

    static bool isValidKeyframe(const ModelNode &node, SL sl = {});
    static bool checkKeyframesType(const ModelNode &node, SL sl = {});
    static QmlTimelineKeyframeGroup keyframeGroupForKeyframe(const ModelNode &node, SL sl = {});
    static QList<QmlTimelineKeyframeGroup> allInvalidTimelineKeyframeGroups(AbstractView *view,
                                                                            SL sl = {});

    void moveAllKeyframes(qreal offset, SL sl = {});
    void scaleAllKeyframes(qreal factor, SL sl = {});
    int getSupposedTargetIndex(qreal newFrame, SL sl = {}) const;

    int indexOfKeyframe(const ModelNode &frame, SL sl = {}) const;
    void slideKeyframe(int sourceIndex, int targetIndex, SL sl = {});

    bool isRecording(SL sl = {}) const;
    void toogleRecording(bool b, SL sl = {}) const;

    QmlTimeline timeline(SL sl = {}) const;

    static bool isDangling(const ModelNode &node, SL sl = {});
    bool isDangling(SL sl = {}) const;
};

} // namespace QmlDesigner
