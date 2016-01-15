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

#include <timeline/timelinerenderpass.h>
#include <QtTest>

using namespace Timeline;

class DummyRenderPass : public TimelineRenderPass
{
public:
    static bool s_dtorRan;

    State *update(const TimelineAbstractRenderer *renderer, const TimelineRenderState *parentState,
                  State *state, int indexFrom, int indexTo, bool stateChanged, qreal spacing) const;
    ~DummyRenderPass();
};

bool DummyRenderPass::s_dtorRan = false;

TimelineRenderPass::State *DummyRenderPass::update(const TimelineAbstractRenderer *renderer,
                                                   const TimelineRenderState *parentState,
                                                   State *state, int indexFrom, int indexTo,
                                                   bool stateChanged, qreal spacing) const
{
    Q_UNUSED(renderer);
    Q_UNUSED(parentState);
    Q_UNUSED(indexFrom);
    Q_UNUSED(indexTo);
    Q_UNUSED(stateChanged);
    Q_UNUSED(spacing);
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

QTEST_MAIN(tst_TimelineRenderPass)

#include "tst_timelinerenderpass.moc"

