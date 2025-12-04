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
    QmlTimeline() = default;

    QmlTimeline(const ModelNode &modelNode)
        : QmlModelNodeFacade(modelNode)
    {}

    bool isValid(SL sl = {}) const;
    explicit operator bool() const { return isValid(); }

    static bool isValidQmlTimeline(const ModelNode &modelNode, SL sl = {});
    void destroy(SL sl = {});

    QmlTimelineKeyframeGroup keyframeGroup(const ModelNode &modelNode,
                                           PropertyNameView propertyName,
                                           SL sl = {});
    bool hasTimeline(const ModelNode &modelNode, PropertyNameView propertyName, SL sl = {});

    qreal startKeyframe(SL sl = {}) const;
    qreal endKeyframe(SL sl = {}) const;
    qreal currentKeyframe(SL sl = {}) const;
    qreal duration(SL sl = {}) const;

    bool isEnabled(SL sl = {}) const;

    qreal minActualKeyframe(const ModelNode &target, SL sl = {}) const;
    qreal maxActualKeyframe(const ModelNode &target, SL sl = {}) const;

    void moveAllKeyframes(const ModelNode &target, qreal offset, SL sl = {});
    void scaleAllKeyframes(const ModelNode &target, qreal factor, SL sl = {});

    QList<ModelNode> allTargets(SL sl = {}) const;
    QList<QmlTimelineKeyframeGroup> keyframeGroupsForTarget(const ModelNode &target, SL sl = {}) const;
    void destroyKeyframesForTarget(const ModelNode &target, SL sl = {});

    void removeKeyframesForTargetAndProperty(const ModelNode &target,
                                             PropertyNameView propertyName,
                                             SL sl = {});

    static bool hasActiveTimeline(AbstractView *view, SL sl = {});

    bool isRecording(SL sl = {}) const;
    void toogleRecording(bool b, SL sl = {}) const;

    void resetGroupRecording(SL sl = {}) const;
    bool hasKeyframeGroup(const ModelNode &node, PropertyNameView propertyName, SL sl = {}) const;
    bool hasKeyframeGroupForTarget(const ModelNode &node, SL sl = {}) const;

    void insertKeyframe(const ModelNode &target, PropertyNameView propertyName, SL sl = {});

private:
    void addKeyframeGroupIfNotExists(const ModelNode &node, PropertyNameView propertyName);
    QList<QmlTimelineKeyframeGroup> allKeyframeGroups() const;
};

} //QmlDesigner
