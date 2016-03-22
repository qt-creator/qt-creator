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
#include <QColor>
#include <timeline/timelinenotesmodel.h>

class tst_TimelineNotesModel : public QObject
{
    Q_OBJECT

private slots:
    void timelineModel();
    void addRemove();
    void properties();
    void selection();
    void modify();
};

class TestModel : public Timeline::TimelineModel {
public:
    TestModel(int modelId = 10) : TimelineModel(modelId)
    {
        insert(0, 10, 10);
    }

    int typeId(int) const
    {
        return 7;
    }
};

class TestNotesModel : public Timeline::TimelineNotesModel {
    friend class tst_TimelineNotesModel;
};

void tst_TimelineNotesModel::timelineModel()
{
    TestNotesModel notes;
    TestModel *model = new TestModel;
    TestModel *model2 = new TestModel(2);
    notes.addTimelineModel(model);
    notes.addTimelineModel(model2);
    QCOMPARE(notes.timelineModelByModelId(10), model);
    QCOMPARE(notes.timelineModels().count(), 2);
    QVERIFY(notes.timelineModels().contains(model));
    QVERIFY(notes.timelineModels().contains(model2));
    QVERIFY(!notes.isModified());
    notes.clear(); // clear() only clears the data, not the models
    QCOMPARE(notes.timelineModels().count(), 2);
    delete model; // notes model does monitor the destroyed() signal
    QCOMPARE(notes.timelineModels().count(), 1);
    delete model2;
}

void tst_TimelineNotesModel::addRemove()
{
    TestNotesModel notes;
    TestModel model;
    notes.addTimelineModel(&model);

    QSignalSpy spy(&notes, SIGNAL(changed(int,int,int)));
    int id = notes.add(10, 0, QLatin1String("xyz"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(notes.isModified(), true);
    QCOMPARE(notes.count(), 1);
    notes.resetModified();
    QCOMPARE(notes.isModified(), false);
    notes.remove(id);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(notes.isModified(), true);
    QCOMPARE(notes.count(), 0);
}

void tst_TimelineNotesModel::properties()
{

    TestNotesModel notes;
    int id = -1;
    {
        TestModel model;
        notes.addTimelineModel(&model);

        id = notes.add(10, 0, QLatin1String("xyz"));
        QVERIFY(id >= 0);
        QCOMPARE(notes.typeId(id), 7);
        QCOMPARE(notes.timelineIndex(id), 0);
        QCOMPARE(notes.timelineModel(id), 10);
        QCOMPARE(notes.text(id), QLatin1String("xyz"));
    }

    QCOMPARE(notes.typeId(id), -1); // cannot ask the model anymore
    QCOMPARE(notes.timelineIndex(id), 0);
    QCOMPARE(notes.timelineModel(id), 10);
    QCOMPARE(notes.text(id), QLatin1String("xyz"));
}

void tst_TimelineNotesModel::selection()
{
    TestNotesModel notes;
    TestModel model;
    notes.addTimelineModel(&model);
    int id1 = notes.add(10, 0, QLatin1String("blablub"));
    int id2 = notes.add(10, 0, QLatin1String("xyz"));
    QVariantList ids = notes.byTimelineModel(10);
    QCOMPARE(ids.length(), 2);
    QVERIFY(ids.contains(id1));
    QVERIFY(ids.contains(id2));

    ids = notes.byTypeId(7);
    QCOMPARE(ids.length(), 2);
    QVERIFY(ids.contains(id1));
    QVERIFY(ids.contains(id2));

    int got = notes.get(10, 0);
    QVERIFY(got == id1 || got == id2);
    QCOMPARE(notes.get(10, 20), -1);
    QCOMPARE(notes.get(20, 10), -1);
}

void tst_TimelineNotesModel::modify()
{
    TestNotesModel notes;
    TestModel model;
    notes.addTimelineModel(&model);
    QSignalSpy spy(&notes, SIGNAL(changed(int,int,int)));
    int id = notes.add(10, 0, QLatin1String("a"));
    QCOMPARE(spy.count(), 1);
    notes.resetModified();
    notes.update(id, QLatin1String("b"));
    QVERIFY(notes.isModified());
    QCOMPARE(spy.count(), 2);
    QCOMPARE(notes.text(id), QLatin1String("b"));
    notes.resetModified();
    notes.update(id, QLatin1String("b"));
    QVERIFY(!notes.isModified());
    QCOMPARE(spy.count(), 2);
    QCOMPARE(notes.text(id), QLatin1String("b"));

    notes.setText(id, QLatin1String("a"));
    QVERIFY(notes.isModified());
    QCOMPARE(spy.count(), 3);
    QCOMPARE(notes.text(id), QLatin1String("a"));
    notes.resetModified();

    notes.setText(10, 0, QLatin1String("x"));
    QVERIFY(notes.isModified());
    QCOMPARE(spy.count(), 4);
    QCOMPARE(notes.text(id), QLatin1String("x"));
    notes.resetModified();

    TestModel model2(9);
    notes.addTimelineModel(&model2);
    notes.setText(9, 0, QLatin1String("hh"));
    QVERIFY(notes.isModified());
    QCOMPARE(spy.count(), 5);
    QCOMPARE(notes.count(), 2);
    notes.resetModified();

    notes.setText(id, QString());
    QVERIFY(notes.isModified());
    QCOMPARE(spy.count(), 6);
    QCOMPARE(notes.count(), 1);
    notes.resetModified();

}

QTEST_MAIN(tst_TimelineNotesModel)

#include "tst_timelinenotesmodel.moc"
