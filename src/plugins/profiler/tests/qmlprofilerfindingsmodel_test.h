// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../qmlprofilerfindingsmodel.h"
#include "../qmlprofilermodelmanager.h"

#include <QObject>

namespace Profiler::Internal {

class QmlProfilerFindingsModelTest : public QObject
{
    Q_OBJECT

public:
    QmlProfilerFindingsModelTest();

private slots:
    void initTestCase();
    void testSlowCompileReported();
    void testFastCompileIgnored();
    void testPixmapLoadErrorsAggregated();
    void testSyncViewLoadAttributedToHandler();
    void testPeriodicHandlerReported();
    void testOversizedPixmapReported();
    void testPerFrameCostReported();
    void testBlockingCallReported();
    void testFrameJankAttributedToHandler();
    void testMemoryChurnAttributedToCaller();
    void testRepeatedCreationByView();
    void testRepeatedCreationByRepeaterOfFileDelegate();
    void testRepeatedCreationFromScript();
    void testRepeatedCreationWithoutContext();
    void testSyncViewLoadNamesWhatItBuilds();
    void testDetailsAreChildRows();
    void testBindingChurnReported();
    void testPixmapReloadReported();
    void testLocationWithoutLine();
    void testSeverityOrdering();
    void testJsonExport();
    void testClear();

private:
    QmlProfilerModelManager manager;
    QmlProfilerFindingsModel model;

    int slowCompileTypeId = -1;
    int fastCompileTypeId = -1;
    int pixmapErrorTypeId = -1;
    int pixmapErrorStartTypeId = -1;
    int pixmapSizeTypeId = -1;
    int handlerTypeId = -1;
    int createdTypeId = -1;
    int timerHandlerTypeId = -1;
    int bindingTypeId = -1;
    int animationTypeId = -1;
    int jankHandlerTypeId = -1;
    int blockingHandlerTypeId = -1;
    int delegateTypeId = -1;
    int churnBindingTypeId = -1;
    int allocatingJsTypeId = -1;
    int memoryTypeId = -1;
    int reloadStartTypeId = -1;
    int reloadFinishTypeId = -1;

    int viewTypeId = -1;
    int viewComponentTypeId = -1;
    int viewDelegateTypeId = -1;
    int viewTextTypeId = -1;
    int viewTextBindingTypeId = -1;
    int viewTextJsTypeId = -1;
    int viewVisibleBindingTypeId = -1;
    int viewVisibleJsTypeId = -1;
    int outsideBindingTypeId = -1;

    int repeaterTypeId = -1;
    int repeaterComponentTypeId = -1;
    int wrapperTypeId = -1;
    int fileRootTypeId = -1;
    int fileBindingTypeId = -1;

    int gameHandlerTypeId = -1;
    int gameHandlerJsTypeId = -1;
    int createBlockTypeId = -1;
    int blockTypeId = -1;
    int blockImageTypeId = -1;
    int blockStateTypeId = -1;
    int blockAnimationTypeId = -1;
    int blockChangesTypeId = -1;
    int blockSourceBindingTypeId = -1;
    int blockSourceJsTypeId = -1;
    int blockCompletedTypeId = -1;
    int blockCompletedJsTypeId = -1;
    int blockAfterBindingTypeId = -1;
    int blockAfterJsTypeId = -1;
};

} // namespace Profiler::Internal
