// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <tracing/timelinerenderpass.h>
#include <QtTest>

using namespace Timeline;

class DummyRenderPass : public TimelineRenderPass
{
public:
    static bool s_dtorRan;

    State *update(const TimelineAbstractRenderer *renderer, const TimelineRenderState *parentState,
                  State *state, int indexFrom, int indexTo, bool stateChanged, float spacing) const;
    ~DummyRenderPass();
};

bool DummyRenderPass::s_dtorRan = false;

TimelineRenderPass::State *DummyRenderPass::update(const TimelineAbstractRenderer *renderer,
                                                   const TimelineRenderState *parentState,
                                                   State *state, int indexFrom, int indexTo,
                                                   bool stateChanged, float spacing) const
{
    Q_UNUSED(renderer)
    Q_UNUSED(parentState)
    Q_UNUSED(indexFrom)
    Q_UNUSED(indexTo)
    Q_UNUSED(stateChanged)
    Q_UNUSED(spacing)
    return state;
}

DummyRenderPass::~DummyRenderPass()
{
    s_dtorRan = true;
}

class tst_TimelineRenderPass : public QObject
{
    Q_OBJECT

private slots:
    void virtualDtor();
    void emptyState();
};

void tst_TimelineRenderPass::virtualDtor()
{
    QVERIFY(!DummyRenderPass::s_dtorRan);
    TimelineRenderPass *pass = new DummyRenderPass;
    QVERIFY(!DummyRenderPass::s_dtorRan);
    delete pass;
    QVERIFY(DummyRenderPass::s_dtorRan);
}

void tst_TimelineRenderPass::emptyState()
{
    TimelineRenderPass::State state;
    QCOMPARE(state.collapsedOverlay(), static_cast<QSGNode *>(0));
    QCOMPARE(state.expandedOverlay(), static_cast<QSGNode *>(0));
    QVERIFY(state.collapsedRows().isEmpty());
    QVERIFY(state.expandedRows().isEmpty());
}

QTEST_GUILESS_MAIN(tst_TimelineRenderPass)

#include "tst_timelinerenderpass.moc"

