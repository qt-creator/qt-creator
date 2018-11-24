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

#include "flamegraphmodel_test.h"

#include <qmlprofiler/qmlprofilerrangemodel.h>

#include <QtTest>

namespace QmlProfiler {
namespace Internal {

FlameGraphModelTest::FlameGraphModelTest(QObject *parent) :
    QObject(parent), model(&manager)
{
}

int FlameGraphModelTest::generateData(QmlProfilerModelManager *manager,
                                      Timeline::TimelineModelAggregator *aggregator)
{
    // Notes only work with timeline models
    auto rangeModel = new QmlProfilerRangeModel(manager, Javascript, aggregator);
    int rangeModelId = rangeModel->modelId();
    manager->notesModel()->addTimelineModel(rangeModel);

    manager->initialize();

    QStack<int> typeIndices;

    int i = 0;
    int typeIndex = -1;
    while (i < 10) {
        QmlEvent event;
        if (i < 5) {
            typeIndex = manager->appendEventType(
                        QmlEventType(MaximumMessage,
                                     static_cast<RangeType>(static_cast<int>(Javascript) - i), -1,
                                     QmlEventLocation("somefile.js", i, 20 - i),
                                     QString("funcfunc")));
        } else {
            typeIndex = typeIndices[i - 5];
        }
        event.setTypeIndex(typeIndex);
        event.setTimestamp(++i);
        event.setRangeStage(RangeStart);
        manager->appendEvent(std::move(event));
        typeIndices.push(typeIndex);
    }

    QmlEvent event;
    event.setTypeIndex(typeIndex);
    event.setRangeStage(RangeEnd);
    event.setTypeIndex(typeIndices.pop());
    event.setTimestamp(++i);
    manager->appendEvent(QmlEvent(event));

    event.setRangeStage(RangeStart);
    event.setTypeIndex(0); // Have it accepted by the JavaScript range model above
    typeIndices.push(0);
    event.setTimestamp(++i);
    manager->appendEvent(std::move(event));

    while (!typeIndices.isEmpty()) {
        QmlEvent event;
        event.setTimestamp(++i);
        event.setRangeStage(RangeEnd);
        event.setTypeIndex(typeIndices.pop());
        manager->appendEvent(std::move(event));
    }

    manager->finalize();

    static_cast<QmlProfilerNotesModel *>(manager->notesModel())
            ->setNotes(QVector<QmlNote>({
                                            // row 2 on purpose to test the range heuristic
                                            QmlNote(0, 2, 1, 21, "dings"),
                                            QmlNote(0, 3, 12, 1, "weg")
                                        }));
    manager->notesModel()->restore();

    return rangeModelId;
}

void FlameGraphModelTest::initTestCase()
{
    QCOMPARE(model.modelManager(), &manager);
    rangeModelId = generateData(&manager, &aggregator);
}

void FlameGraphModelTest::testIndex()
{
    QModelIndex index;
    QModelIndex parent = index;
    for (int i = 0; i < 5; ++i) {
        index = model.index(0, 0, index);
        QVERIFY(index.isValid());
        QCOMPARE(model.parent(index), parent);
        parent = index;
    }
    QVERIFY(!model.parent(QModelIndex()).isValid());
}

void FlameGraphModelTest::testCounts()
{
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.rowCount(model.index(0, 0)), 1);
    QCOMPARE(model.rowCount(model.index(1, 0)), 1);
    QCOMPARE(model.columnCount(), 1);
}

void FlameGraphModelTest::testData()
{
    const QVector<QString> typeRoles({
        FlameGraphModel::tr("JavaScript"),
        FlameGraphModel::tr("Signal"),
        FlameGraphModel::tr("Binding"),
        FlameGraphModel::tr("Create")
    });

    QModelIndex index = model.index(0, 0);
    QModelIndex index2 = model.index(1, 0);
    QCOMPARE(model.data(index, FlameGraphModel::TypeIdRole).toInt(), 0);
    QCOMPARE(model.data(index2, FlameGraphModel::TypeIdRole).toInt(), 4);
    QCOMPARE(model.data(index, FlameGraphModel::TypeRole).toString(),
             FlameGraphModel::tr("JavaScript"));
    QCOMPARE(model.data(index2, FlameGraphModel::TypeRole).toString(),
             FlameGraphModel::tr("Compile"));
    QCOMPARE(model.data(index, FlameGraphModel::DurationRole).toLongLong(), 21);
    QCOMPARE(model.data(index2, FlameGraphModel::DurationRole).toLongLong(), 13);
    QCOMPARE(model.data(index, FlameGraphModel::CallCountRole).toInt(), 1);
    QCOMPARE(model.data(index2, FlameGraphModel::CallCountRole).toInt(), 1);
    QCOMPARE(model.data(index, FlameGraphModel::DetailsRole).toString(),
             QLatin1String("funcfunc"));
    QCOMPARE(model.data(index2, FlameGraphModel::DetailsRole).toString(),
             QLatin1String("funcfunc"));
    QCOMPARE(model.data(index, FlameGraphModel::FilenameRole).toString(),
             QLatin1String("somefile.js"));
    QCOMPARE(model.data(index2, FlameGraphModel::FilenameRole).toString(),
             QLatin1String("somefile.js"));
    QCOMPARE(model.data(index, FlameGraphModel::LineRole).toInt(), 0);
    QCOMPARE(model.data(index2, FlameGraphModel::LineRole).toInt(), 4);
    QCOMPARE(model.data(index, FlameGraphModel::ColumnRole).toInt(), 20);
    QCOMPARE(model.data(index2, FlameGraphModel::ColumnRole).toInt(), 16);

    // The flame graph model does not know that one of the notes belongs to row 2, and the
    // other one to row 3. It will show both of them for all indexes with that type ID.
    QCOMPARE(model.data(index, FlameGraphModel::NoteRole).toString(), QString("dings\nweg"));

    QCOMPARE(model.data(index2, FlameGraphModel::NoteRole).toString(), QString());
    QCOMPARE(model.data(index, FlameGraphModel::TimePerCallRole).toLongLong(), 21);
    QCOMPARE(model.data(index2, FlameGraphModel::TimePerCallRole).toLongLong(), 13);
    QCOMPARE(model.data(index, FlameGraphModel::RangeTypeRole).toInt(),
             static_cast<int>(Javascript));
    QCOMPARE(model.data(index2, FlameGraphModel::RangeTypeRole).toInt(),
             static_cast<int>(Compiling));
    QCOMPARE(model.data(index, FlameGraphModel::LocationRole).toString(),
             QLatin1String("somefile.js:0"));
    QCOMPARE(model.data(index2, FlameGraphModel::LocationRole).toString(),
             QLatin1String("somefile.js:4"));
    QVERIFY(!model.data(index, -10).isValid());
    QVERIFY(!model.data(index2, -10).isValid());
    QVERIFY(!model.data(QModelIndex(), FlameGraphModel::LineRole).isValid());

    for (int i = 1; i < 9; ++i) {
        index = model.index(0, 0, index);
        QCOMPARE(model.data(index, FlameGraphModel::TypeRole).toString(),
                 typeRoles[i % typeRoles.length()]);
        QCOMPARE(model.data(index, FlameGraphModel::NoteRole).toString(),
                 (i % typeRoles.length() == 0) ? QString("dings\nweg") : QString());
    }
    QCOMPARE(model.data(index, FlameGraphModel::CallCountRole).toInt(), 1);

    index2 = model.index(0, 0, index2);
    QCOMPARE(model.data(index2, FlameGraphModel::TypeRole).toString(),
             FlameGraphModel::tr("Compile"));
    QCOMPARE(model.data(index2, FlameGraphModel::NoteRole).toString(), QString());
    QCOMPARE(model.data(index2, FlameGraphModel::CallCountRole).toInt(), 1);
}

void FlameGraphModelTest::testRoleNames()
{
    auto names = model.roleNames();
    QCOMPARE(names[FlameGraphModel::TypeIdRole],        QByteArray("typeId"));
    QCOMPARE(names[FlameGraphModel::TypeRole],          QByteArray("type"));
    QCOMPARE(names[FlameGraphModel::DurationRole],      QByteArray("duration"));
    QCOMPARE(names[FlameGraphModel::CallCountRole],     QByteArray("callCount"));
    QCOMPARE(names[FlameGraphModel::DetailsRole],       QByteArray("details"));
    QCOMPARE(names[FlameGraphModel::FilenameRole],      QByteArray("filename"));
    QCOMPARE(names[FlameGraphModel::LineRole],          QByteArray("line"));
    QCOMPARE(names[FlameGraphModel::ColumnRole],        QByteArray("column"));
    QCOMPARE(names[FlameGraphModel::NoteRole],          QByteArray("note"));
    QCOMPARE(names[FlameGraphModel::TimePerCallRole],   QByteArray("timePerCall"));
    QCOMPARE(names[FlameGraphModel::RangeTypeRole],     QByteArray("rangeType"));
}

void FlameGraphModelTest::testNotes()
{
    manager.notesModel()->add(rangeModelId, 1, QString("blubb"));
    QCOMPARE(model.data(model.index(0, 0), FlameGraphModel::NoteRole).toString(),
             QString("dings\nweg\nblubb"));
    manager.notesModel()->remove(0);
    QCOMPARE(model.data(model.index(0, 0), FlameGraphModel::NoteRole).toString(),
             QString("weg\nblubb"));
    manager.notesModel()->remove(0);
    QCOMPARE(model.data(model.index(0, 0), FlameGraphModel::NoteRole).toString(),
             QString("blubb"));
    manager.notesModel()->remove(0);
    QCOMPARE(model.data(model.index(0, 0), FlameGraphModel::NoteRole).toString(),
             QString());
}

void FlameGraphModelTest::cleanupTestCase()
{
    manager.clearAll();
    QCOMPARE(model.rowCount(), 0);
}

} // namespace Internal
} // namespace QmlProfiler
