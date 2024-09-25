// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlmodelnodefacade.h"

namespace QmlDesigner {

class AbstractViewAbstractVieweGroup;
class QmlObjectNode;
class QmlModelStateGroup;
class QmlTimelineKeyframeGroup;

class QMLDESIGNER_EXPORT QmlTimeline final : public QmlModelNodeFacade
{

public:
    QmlTimeline();
    QmlTimeline(const ModelNode &modelNode);

    bool isValid() const;
    explicit operator bool() const { return isValid(); }
    static bool isValidQmlTimeline(const ModelNode &modelNode);
    void destroy();

    QmlTimelineKeyframeGroup keyframeGroup(const ModelNode &modelNode, PropertyNameView propertyName);
    bool hasTimeline(const ModelNode &modelNode, PropertyNameView propertyName);

    qreal startKeyframe() const;
    qreal endKeyframe() const;
    qreal currentKeyframe() const;
    qreal duration() const;

    bool isEnabled() const;

    qreal minActualKeyframe(const ModelNode &target) const;
    qreal maxActualKeyframe(const ModelNode &target) const;

    void moveAllKeyframes(const ModelNode &target, qreal offset);
    void scaleAllKeyframes(const ModelNode &target, qreal factor);

    QList<ModelNode> allTargets() const;
    QList<QmlTimelineKeyframeGroup> keyframeGroupsForTarget(const ModelNode &target) const;
    void destroyKeyframesForTarget(const ModelNode &target);

    void removeKeyframesForTargetAndProperty(const ModelNode &target, PropertyNameView propertyName);

    static bool hasActiveTimeline(AbstractView *view);

    bool isRecording() const;
    void toogleRecording(bool b) const;

    void resetGroupRecording() const;
    bool hasKeyframeGroup(const ModelNode &node, PropertyNameView propertyName) const;
    bool hasKeyframeGroupForTarget(const ModelNode &node) const;

    void insertKeyframe(const ModelNode &target, PropertyNameView propertyName);

private:
    void addKeyframeGroupIfNotExists(const ModelNode &node, PropertyNameView propertyName);
    QList<QmlTimelineKeyframeGroup> allKeyframeGroups() const;
};

} //QmlDesigner
