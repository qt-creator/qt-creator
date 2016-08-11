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

#include <QtTest>
#include <timeline/timelineoverviewrenderer_p.h>

using namespace Timeline;

class DummyRenderer : public TimelineOverviewRenderer {
    friend class tst_TimelineOverviewRenderer;
};

class DummyModel : public TimelineModel {
public:
    DummyModel() : TimelineModel(0) {}

    void loadData()
    {
        setCollapsedRowCount(3);
        setExpandedRowCount(3);
        for (int i = 0; i < 10; ++i)
            insert(i, i, i);
        emit contentChanged();
    }
};

class tst_TimelineOverviewRenderer : public QObject
{
    Q_OBJECT

private slots:
    void updatePaintNode();
};

void tst_TimelineOverviewRenderer::updatePaintNode()
{
    DummyRenderer renderer;
    QCOMPARE(renderer.updatePaintNode(0, 0), static_cast<QSGNode *>(0));
    DummyModel model;
    renderer.setModel(&model);
    QCOMPARE(renderer.updatePaintNode(0, 0), static_cast<QSGNode *>(0));
    model.loadData();
    QCOMPARE(renderer.updatePaintNode(0, 0), static_cast<QSGNode *>(0));
    TimelineZoomControl zoomer;
    renderer.setZoomer(&zoomer);
    QCOMPARE(renderer.updatePaintNode(0, 0), static_cast<QSGNode *>(0));
    zoomer.setTrace(0, 10);
    QSGNode *node = renderer.updatePaintNode(0, 0);
    QVERIFY(node != 0);
    QCOMPARE(renderer.updatePaintNode(node, 0), node);
    delete node;
}

QTEST_MAIN(tst_TimelineOverviewRenderer)

#include "tst_timelineoverviewrenderer.moc"
