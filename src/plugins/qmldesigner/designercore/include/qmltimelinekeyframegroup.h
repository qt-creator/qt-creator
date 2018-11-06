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
#include "qmlchangeset.h"

namespace QmlDesigner {

class AbstractViewAbstractVieweGroup;
class QmlObjectNode;
class QmlTimeline;

class QMLDESIGNERCORE_EXPORT QmlTimelineKeyframeGroup : public QmlModelNodeFacade
{

public:
    QmlTimelineKeyframeGroup();
    QmlTimelineKeyframeGroup(const ModelNode &modelNode);

    bool isValid() const override;
    static bool isValidQmlTimelineKeyframeGroup(const ModelNode &modelNode);
    void destroy();

    ModelNode target() const;
    void setTarget(const ModelNode &target);

    PropertyName propertyName() const;
    void setPropertyName(const PropertyName &propertyName);

    void setValue(const QVariant &value, qreal frame);
    QVariant value(qreal frame) const;

    TypeName valueType() const;

    qreal currentFrame() const;

    bool hasKeyframe(qreal frame);

    qreal minActualKeyframe() const;
    qreal maxActualKeyframe() const;

    const QList<ModelNode> keyframePositions() const;

    static bool isValidKeyframe(const ModelNode &node);
    static bool checkKeyframesType(const ModelNode &node);
    static QmlTimelineKeyframeGroup keyframeGroupForKeyframe(const ModelNode &node);

    void moveAllKeyframes(qreal offset);
    void scaleAllKeyframes(qreal factor);
    int getSupposedTargetIndex(qreal newFrame) const;

    int indexOfKeyframe(const ModelNode &frame) const;
    void slideKeyframe(int sourceIndex, int targetIndex);

    bool isRecording() const;
    void toogleRecording(bool b) const;

    QmlTimeline timeline() const;
};

} //QmlDesigner
