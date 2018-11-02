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

#include <qmldesignercorelib_global.h>
#include "qmlmodelnodefacade.h"

namespace QmlDesigner {

class AbstractViewAbstractVieweGroup;
class QmlObjectNode;
class QmlModelStateGroup;
class QmlTimelineKeyframeGroup;

class QMLDESIGNERCORE_EXPORT QmlTimeline : public QmlModelNodeFacade
{

public:
    QmlTimeline();
    QmlTimeline(const ModelNode &modelNode);

    bool isValid() const override;
    static bool isValidQmlTimeline(const ModelNode &modelNode);
    void destroy();

    QmlTimelineKeyframeGroup keyframeGroup(const ModelNode &modelNode, const PropertyName &propertyName);
    bool hasTimeline(const ModelNode &modelNode, const PropertyName &propertyName);

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
    static bool hasActiveTimeline(AbstractView *view);

    bool isRecording() const;
    void toogleRecording(bool b) const;

    void resetGroupRecording() const;

private:
    void addKeyframeGroupIfNotExists(const ModelNode &node, const PropertyName &propertyName);
    bool hasKeyframeGroup(const ModelNode &node, const PropertyName &propertyName) const;
    QList<QmlTimelineKeyframeGroup> allKeyframeGroups() const;
};

} //QmlDesigner
