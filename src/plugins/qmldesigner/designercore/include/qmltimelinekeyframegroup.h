// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlchangeset.h"
#include "qmlmodelnodefacade.h"
#include <qmldesignercorelib_global.h>

namespace QmlDesigner {

class AbstractViewAbstractVieweGroup;
class QmlObjectNode;
class QmlTimeline;

class QMLDESIGNERCORE_EXPORT QmlTimelineKeyframeGroup final : public QmlModelNodeFacade
{
public:
    QmlTimelineKeyframeGroup();
    QmlTimelineKeyframeGroup(const ModelNode &modelNode);

    bool isValid() const;
    explicit operator bool() const { return isValid(); }
    static bool isValidQmlTimelineKeyframeGroup(const ModelNode &modelNode);
    void destroy();

    ModelNode target() const;
    void setTarget(const ModelNode &target);

    PropertyName propertyName() const;
    void setPropertyName(const PropertyName &propertyName);

    void setValue(const QVariant &value, qreal frame);
    QVariant value(qreal frame) const;

    NodeMetaInfo valueType() const;

    qreal currentFrame() const;

    bool hasKeyframe(qreal frame);

    qreal minActualKeyframe() const;
    qreal maxActualKeyframe() const;

    ModelNode keyframe(qreal position) const;

    QList<ModelNode> keyframes() const;

    QList<ModelNode> keyframePositions() const;

    static bool isValidKeyframe(const ModelNode &node);
    static bool checkKeyframesType(const ModelNode &node);
    static QmlTimelineKeyframeGroup keyframeGroupForKeyframe(const ModelNode &node);
    static QList<QmlTimelineKeyframeGroup> allInvalidTimelineKeyframeGroups(AbstractView *view);

    void moveAllKeyframes(qreal offset);
    void scaleAllKeyframes(qreal factor);
    int getSupposedTargetIndex(qreal newFrame) const;

    int indexOfKeyframe(const ModelNode &frame) const;
    void slideKeyframe(int sourceIndex, int targetIndex);

    bool isRecording() const;
    void toogleRecording(bool b) const;

    QmlTimeline timeline() const;

    static bool isDangling(const ModelNode &node);
    bool isDangling() const;
};

} // namespace QmlDesigner
