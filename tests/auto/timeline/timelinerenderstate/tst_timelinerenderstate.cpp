/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <timeline/timelinerenderstate.h>
#include <QtTest>
#include <QSGSimpleRectNode>

using namespace Timeline;

class DummyPassState : public TimelineRenderPass::State
{
private:
    QVector<QSGNode *> m_collapsedRows;
    QVector<QSGNode *> m_expandedRows;
    QSGNode *m_collapsedOverlay;
    QSGNode *m_expandedOverlay;

public:
    DummyPassState();
    ~DummyPassState();

    const QVector<QSGNode *> &expandedRows() const;
    const QVector<QSGNode *> &collapsedRows() const;
    QSGNode *expandedOverlay() const;
    QSGNode *collapsedOverlay() const;
};

QSGGeometryNode *createNode()
{
    QSGGeometryNode *node = new QSGSimpleRectNode;
    node->setFlag(QSGNode::OwnedByParent, false);
    return node;
}

DummyPassState::DummyPassState()
{
    m_collapsedRows << createNode() << createNode();
    m_expandedRows << createNode() << createNode();
    m_collapsedOverlay = createNode();
    m_expandedOverlay = createNode();
}

DummyPassState::~DummyPassState()
{
    delete m_collapsedOverlay;
    delete m_expandedOverlay;
    qDeleteAll(m_collapsedRows);
    qDeleteAll(m_expandedRows);
}

const QVector<QSGNode *> &DummyPassState::expandedRows() const
{
    return m_expandedRows;
}

const QVector<QSGNode *> &DummyPassState::collapsedRows() const
{
    return m_collapsedRows;
}

QSGNode *DummyPassState::expandedOverlay() const
{
    return m_expandedOverlay;
}

QSGNode *DummyPassState::collapsedOverlay() const
{
    return m_collapsedOverlay;
}

class tst_TimelineRenderState : public QObject
{
    Q_OBJECT

private slots:
    void startEndScale();
    void passStates();
    void emptyRoots();
    void assembleNodeTree();
};

void tst_TimelineRenderState::startEndScale()
{
    TimelineRenderState state(1, 2, 0.5, 3);
    QCOMPARE(state.start(), 1);
    QCOMPARE(state.end(), 2);
    QCOMPARE(state.scale(), 0.5);
}

void tst_TimelineRenderState::passStates()
{
    TimelineRenderState state(1, 2, 0.5, 3);
    const TimelineRenderState &constState = state;
    QCOMPARE(state.passState(0), static_cast<TimelineRenderPass::State *>(0));
    QCOMPARE(constState.passState(0), static_cast<const TimelineRenderPass::State *>(0));
    TimelineRenderPass::State *passState = new TimelineRenderPass::State;
    state.setPassState(0, passState);
    QCOMPARE(state.passState(0), passState);
    QCOMPARE(constState.passState(0), passState);
}

void tst_TimelineRenderState::emptyRoots()
{
    TimelineRenderState state(1, 2, 0.5, 3);
    QCOMPARE(state.expandedRowRoot()->childCount(), 0);
    QCOMPARE(state.collapsedRowRoot()->childCount(), 0);
    QCOMPARE(state.expandedOverlayRoot()->childCount(), 0);
    QCOMPARE(state.collapsedOverlayRoot()->childCount(), 0);

    const TimelineRenderState &constState = state;
    QCOMPARE(constState.expandedRowRoot()->childCount(), 0);
    QCOMPARE(constState.collapsedRowRoot()->childCount(), 0);
    QCOMPARE(constState.expandedOverlayRoot()->childCount(), 0);
    QCOMPARE(constState.collapsedOverlayRoot()->childCount(), 0);

    QVERIFY(state.isEmpty());
}

void tst_TimelineRenderState::assembleNodeTree()
{
    TimelineModel model(3, QString());
    TimelineRenderState state1(1, 2, 0.5, 3);
    state1.assembleNodeTree(&model, 30, 30);
    QSGTransformNode *node = state1.finalize(0, true, QMatrix4x4());
    QVERIFY(node != static_cast<QSGTransformNode *>(0));
    QVERIFY(node->childCount() == 2);

    TimelineRenderState state2(3, 4, 0.5, 3);
    state2.setPassState(0, new TimelineRenderPass::State);
    state2.assembleNodeTree(&model, 30, 30);
    QCOMPARE(state2.finalize(node, true, QMatrix4x4()), node);
    QVERIFY(node->childCount() == 2);

    TimelineRenderState state3(4, 5, 0.5, 3);
    DummyPassState *dummyState = new DummyPassState;
    state3.setPassState(0, new TimelineRenderPass::State);
    state3.setPassState(1, dummyState);
    state3.assembleNodeTree(&model, 30, 30);
    node = state3.finalize(node, true, QMatrix4x4());
    QCOMPARE(node->firstChild()->firstChild()->firstChild(), dummyState->expandedRows()[0]);
    QCOMPARE(node->lastChild()->firstChild(), dummyState->expandedOverlay());

    node = state3.finalize(node, false, QMatrix4x4());
    QCOMPARE(node->firstChild()->firstChild()->firstChild(), dummyState->collapsedRows()[0]);
    QCOMPARE(node->lastChild()->firstChild(), dummyState->collapsedOverlay());

    delete node;
}

QTEST_MAIN(tst_TimelineRenderState)

#include "tst_timelinerenderstate.moc"

