// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <modelnode.h>
#include <qmltimeline.h>

namespace QmlDesigner {

class ExternalDependenciesInterface;

class TimelineActions
{
public:
    static void deleteAllKeyframesForTarget(const ModelNode &targetNode,
                                            const QmlTimeline &timeline);
    static void insertAllKeyframesForTarget(const ModelNode &targetNode,
                                            const QmlTimeline &timeline);
    static void copyAllKeyframesForTarget(const ModelNode &targetNode, const QmlTimeline &timeline);
    static void pasteKeyframesToTarget(const ModelNode &targetNode, const QmlTimeline &timeline);

    static void copyKeyframes(const QList<ModelNode> &keyframes,
                              ExternalDependenciesInterface &externalDependencies);
    static void pasteKeyframes(AbstractView *timelineView, const QmlTimeline &TimelineActions);

    static bool clipboardContainsKeyframes();

private:
    TimelineActions();
};

} // namespace QmlDesigner
