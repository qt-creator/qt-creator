// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <tracing/timelinemodelaggregator.h>
#include <utils/qtcsettings.h>

using namespace Timeline;
using namespace Utils;

class tst_TimelineModelAggregator : public QObject
{
    Q_OBJECT

private slots:
    void height();
    void addRemoveModel();
    void prevNext();
    void pausedRanges();
    void hideModels();
    void showOnlyOneModel();
    void hiddenModelsKeepTheirPlace();
    void orderSurvivesNewModels();
    void hiddenStateSurvivesNewModels();
    void modelsWithoutPersistentIdTakeTheirPlaceFromTheOwner();
    void absentModelsKeepTheirPlace();
    void layoutSurvivesRestart();
    void forgetsAbsentModelsBeyondLimit();
};

static TimelineModel *namedModel(TimelineModelAggregator *aggregator, const QString &id)
{
    auto model = new TimelineModel(aggregator);
    model->setPersistentId(id);
    return model;
}

static QStringList namesOf(const QList<TimelineModel *> &models)
{
    QStringList names;
    for (const TimelineModel *model : models)
        names.append(model->persistentId());
    return names;
}

// What a profiler does when it loads another trace: it drops its models and
// creates new ones, with new model ids, in the order it always gives them.
static QList<TimelineModel *> reload(TimelineModelAggregator *aggregator, const QStringList &names)
{
    const QList<TimelineModel *> old = aggregator->models();
    aggregator->clear();
    qDeleteAll(old);
    QList<TimelineModel *> models;
    for (const QString &name : names)
        models.append(name.isEmpty() ? new TimelineModel(aggregator) : namedModel(aggregator, name));
    aggregator->setModels(models);
    return models;
}

class HeightTestModel : public TimelineModel {
public:
    HeightTestModel(TimelineModelAggregator *parent) : TimelineModel(parent)
    {
        insert(0, 1, 1);
    }
};

void tst_TimelineModelAggregator::height()
{
    TimelineModelAggregator aggregator;
    QCOMPARE(aggregator.height(), 0);

    QSignalSpy heightSpy(&aggregator, &TimelineModelAggregator::heightChanged);
    TimelineModel *model = new TimelineModel(&aggregator);
    aggregator.addModel(model);
    QCOMPARE(aggregator.height(), 0);
    QCOMPARE(heightSpy.count(), 0);
    aggregator.addModel(new HeightTestModel(&aggregator));
    QVERIFY(aggregator.height() > 0);
    QCOMPARE(heightSpy.count(), 1);
    aggregator.clear();
    QCOMPARE(aggregator.height(), 0);
    QCOMPARE(heightSpy.count(), 2);
}

void tst_TimelineModelAggregator::addRemoveModel()
{
    TimelineNotesModel notes;
    TimelineModelAggregator aggregator;
    aggregator.setNotes(&notes);
    QSignalSpy spy(&aggregator, &TimelineModelAggregator::modelsChanged);

    QCOMPARE(aggregator.notes(), &notes);

    TimelineModel *model1 = new TimelineModel(&aggregator);
    TimelineModel *model2 = new TimelineModel(&aggregator);
    aggregator.addModel(model1);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(aggregator.modelCount(), 1);
    QCOMPARE(aggregator.model(0), model1);
    QCOMPARE(aggregator.models().count(), 1);

    aggregator.addModel(model2);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(aggregator.modelCount(), 2);
    QCOMPARE(aggregator.model(1), model2);
    QCOMPARE(aggregator.models().count(), 2);

    QCOMPARE(aggregator.modelIndexById(model1->modelId()), 0);
    QCOMPARE(aggregator.modelIndexById(model2->modelId()), 1);
    QCOMPARE(aggregator.modelIndexById(aggregator.generateModelId()), -1);

    QCOMPARE(aggregator.modelOffset(0), 0);
    QCOMPARE(aggregator.modelOffset(1), 0);

    aggregator.clear();
    QCOMPARE(spy.count(), 3);
    QCOMPARE(aggregator.modelCount(), 0);
}

class PrevNextTestModel : public TimelineModel
{
public:
    PrevNextTestModel(TimelineModelAggregator *parent) : TimelineModel(parent)
    {
        for (int i = 0; i < 20; ++i)
            insert(i + modelId(), i * modelId(), modelId());
    }
};

void tst_TimelineModelAggregator::prevNext()
{
    TimelineModelAggregator aggregator;
    aggregator.generateModelId(); // start modelIds at 1
    aggregator.addModel(new PrevNextTestModel(&aggregator));
    aggregator.addModel(new PrevNextTestModel(&aggregator));
    aggregator.addModel(new PrevNextTestModel(&aggregator));

    // Add an empty model to trigger the special code paths that skip it
    aggregator.addModel(new TimelineModel(&aggregator));
    {
        int indexes[] = {-1, -1, -1};
        int lastModel = -1;
        int lastIndex = -1;
        for (int i = 0; i < 60; ++i) {
            ModelItem item = aggregator.nextItem(lastModel, lastIndex, -1);
            QVERIFY(item.modelIndex < 3);
            QVERIFY(item.modelIndex >= 0);
            QCOMPARE(item.itemIndex, ++indexes[item.modelIndex]);
            lastModel = item.modelIndex;
            lastIndex = item.itemIndex;
        }

        ModelItem item = aggregator.nextItem(lastModel, lastIndex, -1);
        QCOMPARE(item.modelIndex, 0);
        QCOMPARE(item.itemIndex, 0);
    }

    {
        int indexes[] = {20, 20, 20};
        int lastModel = -1;
        int lastIndex = -1;
        for (int i = 0; i < 60; ++i) {
            ModelItem item = aggregator.prevItem(lastModel, lastIndex, -1);
            QVERIFY(item.modelIndex < 3);
            QVERIFY(item.modelIndex >= 0);
            QCOMPARE(item.itemIndex, --indexes[item.modelIndex]);
            lastModel = item.modelIndex;
            lastIndex = item.itemIndex;
        }

        ModelItem item = aggregator.prevItem(lastModel, lastIndex, -1);
        QCOMPARE(item.modelIndex, 2);
        QCOMPARE(item.itemIndex, 19);
    }
}

void tst_TimelineModelAggregator::pausedRanges()
{
    TimelineModelAggregator aggregator;
    QSignalSpy spy(&aggregator, &TimelineModelAggregator::pausedRangesChanged);
    QVERIFY(aggregator.pausedRanges().isEmpty());

    const TimelineModelAggregator::PausedRanges ranges{{10, 20}, {30, 40}};
    aggregator.setPausedRanges(ranges);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(aggregator.pausedRanges(), ranges);

    aggregator.setPausedRanges(ranges);
    QCOMPARE(spy.count(), 1);

    aggregator.clear();
    QVERIFY(aggregator.pausedRanges().isEmpty());
    QCOMPARE(spy.count(), 2);

    aggregator.clear();
    QCOMPARE(spy.count(), 2);
}

void tst_TimelineModelAggregator::hideModels()
{
    TimelineModelAggregator aggregator;
    TimelineModel *memory = new TimelineModel(&aggregator);
    TimelineModel *creating = new TimelineModel(&aggregator);
    aggregator.setModels({memory, creating});
    QSignalSpy spy(&aggregator, &TimelineModelAggregator::hiddenModelsChanged);

    QVERIFY(!aggregator.isModelHidden(memory->modelId()));
    aggregator.setModelHidden(memory->modelId(), true);
    QCOMPARE(spy.count(), 1);
    QVERIFY(aggregator.isModelHidden(memory->modelId()));
    QVERIFY(!aggregator.isModelHidden(creating->modelId()));

    // Hiding it again changes nothing, and says nothing.
    aggregator.setModelHidden(memory->modelId(), true);
    QCOMPARE(spy.count(), 1);

    // The user hides it from the view, the model itself is not hidden.
    QVERIFY(!memory->hidden());
    QCOMPARE(aggregator.models(), QList<TimelineModel *>({memory, creating}));

    aggregator.setModelHidden(memory->modelId(), false);
    QCOMPARE(spy.count(), 2);
    QVERIFY(!aggregator.isModelHidden(memory->modelId()));

    aggregator.setModelHidden(memory->modelId(), true);
    aggregator.setModelHidden(creating->modelId(), true);
    QCOMPARE(spy.count(), 4);
    aggregator.showAllModels();
    QCOMPARE(spy.count(), 5);
    QVERIFY(!aggregator.isModelHidden(memory->modelId()));
    QVERIFY(!aggregator.isModelHidden(creating->modelId()));
    aggregator.showAllModels();
    QCOMPARE(spy.count(), 5);
}

void tst_TimelineModelAggregator::showOnlyOneModel()
{
    TimelineModelAggregator aggregator;
    TimelineModel *pixmaps = new TimelineModel(&aggregator);
    TimelineModel *memory = new TimelineModel(&aggregator);
    TimelineModel *creating = new TimelineModel(&aggregator);
    aggregator.setModels({pixmaps, memory, creating});
    aggregator.setModelHidden(memory->modelId(), true);
    QSignalSpy spy(&aggregator, &TimelineModelAggregator::hiddenModelsChanged);

    // Showing only a hidden model shows it.
    aggregator.showOnlyModel(memory->modelId());
    QCOMPARE(spy.count(), 1);
    QVERIFY(aggregator.isModelHidden(pixmaps->modelId()));
    QVERIFY(!aggregator.isModelHidden(memory->modelId()));
    QVERIFY(aggregator.isModelHidden(creating->modelId()));

    aggregator.showOnlyModel(memory->modelId());
    QCOMPARE(spy.count(), 1);

    // Then the second one to look at next to it.
    aggregator.setModelHidden(creating->modelId(), false);
    QVERIFY(aggregator.isModelHidden(pixmaps->modelId()));
    QVERIFY(!aggregator.isModelHidden(memory->modelId()));
    QVERIFY(!aggregator.isModelHidden(creating->modelId()));
}

void tst_TimelineModelAggregator::hiddenModelsKeepTheirPlace()
{
    TimelineModelAggregator aggregator;
    TimelineModel *pixmaps = new TimelineModel(&aggregator);
    TimelineModel *memory = new TimelineModel(&aggregator);
    TimelineModel *creating = new TimelineModel(&aggregator);
    aggregator.setModels({pixmaps, memory, creating});
    aggregator.setModelHidden(memory->modelId(), true);

    aggregator.moveModel(2, 0);
    QCOMPARE(aggregator.models(), QList<TimelineModel *>({creating, pixmaps, memory}));
    QVERIFY(aggregator.isModelHidden(memory->modelId()));

    // A profiler takes its models out while it loads a trace, and puts them back.
    const QList<TimelineModel *> models = aggregator.models();
    aggregator.setModels({});
    aggregator.setModels(models);
    QCOMPARE(aggregator.models(), QList<TimelineModel *>({creating, pixmaps, memory}));
    QVERIFY(aggregator.isModelHidden(memory->modelId()));
    QVERIFY(!aggregator.isModelHidden(creating->modelId()));
}

void tst_TimelineModelAggregator::orderSurvivesNewModels()
{
    TimelineModelAggregator aggregator;
    reload(&aggregator, {"Pixmap Cache", "Memory Usage", "Creating"});
    aggregator.moveModel(2, 1);
    QCOMPARE(namesOf(aggregator.models()),
             QStringList({"Pixmap Cache", "Creating", "Memory Usage"}));

    const QList<TimelineModel *> models
        = reload(&aggregator, {"Pixmap Cache", "Memory Usage", "Creating"});
    QCOMPARE(namesOf(aggregator.models()),
             QStringList({"Pixmap Cache", "Creating", "Memory Usage"}));
    QCOMPARE(aggregator.modelIndexById(models.at(2)->modelId()), 1);
}

void tst_TimelineModelAggregator::hiddenStateSurvivesNewModels()
{
    TimelineModelAggregator aggregator;
    QList<TimelineModel *> models = reload(&aggregator, {"Memory Usage", "Creating", "Binding"});
    aggregator.showOnlyModel(models.at(0)->modelId());
    aggregator.setModelHidden(models.at(1)->modelId(), false);

    QSignalSpy spy(&aggregator, &TimelineModelAggregator::hiddenModelsChanged);
    models = reload(&aggregator, {"Memory Usage", "Creating", "Binding"});
    QCOMPARE(spy.count(), 1);
    QVERIFY(!aggregator.isModelHidden(models.at(0)->modelId()));
    QVERIFY(!aggregator.isModelHidden(models.at(1)->modelId()));
    QVERIFY(aggregator.isModelHidden(models.at(2)->modelId()));

    aggregator.showAllModels();
    models = reload(&aggregator, {"Memory Usage", "Creating", "Binding"});
    for (const TimelineModel *model : std::as_const(models))
        QVERIFY(!aggregator.isModelHidden(model->modelId()));
}

void tst_TimelineModelAggregator::modelsWithoutPersistentIdTakeTheirPlaceFromTheOwner()
{
    TimelineModelAggregator aggregator;
    QList<TimelineModel *> models = reload(&aggregator, {"A", "", "B", "C"});
    aggregator.moveModel(3, 0);
    QCOMPARE(namesOf(aggregator.models()), QStringList({"C", "A", "", "B"}));
    aggregator.setModelHidden(models.at(1)->modelId(), true);

    // The named ones take the places they hold among each other, around the
    // unnamed one, which is where its owner puts it, and shown again.
    models = reload(&aggregator, {"A", "", "B", "C"});
    QCOMPARE(namesOf(aggregator.models()), QStringList({"C", "", "A", "B"}));
    QVERIFY(!aggregator.isModelHidden(models.at(1)->modelId()));
}

void tst_TimelineModelAggregator::absentModelsKeepTheirPlace()
{
    TimelineModelAggregator aggregator;
    QList<TimelineModel *> models = reload(&aggregator, {"A", "B", "C"});
    aggregator.moveModel(2, 0);
    QCOMPARE(namesOf(aggregator.models()), QStringList({"C", "A", "B"}));

    // The owner leaves C out for now, as the CTF viewer does for threads the
    // user does not look at, and the user swaps the other two meanwhile.
    aggregator.setModels({models.at(0), models.at(1)});
    QCOMPARE(namesOf(aggregator.models()), QStringList({"A", "B"}));
    aggregator.moveModel(1, 0);

    aggregator.setModels(models);
    QCOMPARE(namesOf(aggregator.models()), QStringList({"C", "B", "A"}));

    // A model that the user has never moved goes after the one above it once
    // the user moves another.
    reload(&aggregator, {"A", "D", "B", "C"});
    QCOMPARE(namesOf(aggregator.models()), QStringList({"C", "D", "B", "A"}));
    aggregator.moveModel(0, 3);
    QCOMPARE(namesOf(aggregator.models()), QStringList({"D", "B", "A", "C"}));
    reload(&aggregator, {"A", "B", "C", "D"});
    QCOMPARE(namesOf(aggregator.models()), QStringList({"D", "B", "A", "C"}));
}

void tst_TimelineModelAggregator::layoutSurvivesRestart()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString fileName = dir.filePath("settings.ini");
    const QStringList names{"Memory Usage", "Creating", "Binding", "Painting"};

    {
        QtcSettings settings(fileName, QSettings::IniFormat);
        TimelineModelAggregator aggregator;
        aggregator.persistLayout(&settings, "Timeline");
        const QList<TimelineModel *> models = reload(&aggregator, names);
        aggregator.moveModel(3, 0);
        aggregator.setModelHidden(models.at(2)->modelId(), true);
    }

    {
        QtcSettings settings(fileName, QSettings::IniFormat);
        TimelineModelAggregator aggregator;
        // The QML profiler has its models before it asks for their layout.
        const QList<TimelineModel *> models = reload(&aggregator, names);
        QSignalSpy modelsSpy(&aggregator, &TimelineModelAggregator::modelsChanged);
        QSignalSpy hiddenSpy(&aggregator, &TimelineModelAggregator::hiddenModelsChanged);
        aggregator.persistLayout(&settings, "Timeline");
        QCOMPARE(modelsSpy.count(), 1);
        QCOMPARE(hiddenSpy.count(), 1);
        QCOMPARE(namesOf(aggregator.models()),
                 QStringList({"Painting", "Memory Usage", "Creating", "Binding"}));
        QVERIFY(aggregator.isModelHidden(models.at(2)->modelId()));
        QVERIFY(!aggregator.isModelHidden(models.at(1)->modelId()));

        aggregator.showAllModels();
        QVERIFY(!settings.contains("Timeline/Hidden"));
    }

    {
        QtcSettings settings(fileName, QSettings::IniFormat);
        TimelineModelAggregator aggregator;
        // Perf and CTF ask for it before they have any.
        aggregator.persistLayout(&settings, "Timeline");
        const QList<TimelineModel *> models = reload(&aggregator, names);
        QCOMPARE(namesOf(aggregator.models()),
                 QStringList({"Painting", "Memory Usage", "Creating", "Binding"}));
        QVERIFY(!aggregator.isModelHidden(models.at(2)->modelId()));
    }
}

void tst_TimelineModelAggregator::forgetsAbsentModelsBeyondLimit()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QtcSettings settings(dir.filePath("settings.ini"), QSettings::IniFormat);

    for (const int absent : {10, 300}) {
        QStringList order;
        for (int i = 0; i < absent; ++i)
            order.append(QString("Thread %1").arg(i));
        settings.setValue("Timeline/Order", order);

        TimelineModelAggregator aggregator;
        aggregator.persistLayout(&settings, "Timeline");
        reload(&aggregator, {"A", "B"});
        aggregator.moveModel(1, 0);

        const QStringList saved = settings.value("Timeline/Order").toStringList();
        if (absent == 10)
            QCOMPARE(saved, QStringList({"B", "A"}) + order);
        else
            QCOMPARE(saved, QStringList({"B", "A"}));
    }
}

QTEST_GUILESS_MAIN(tst_TimelineModelAggregator)

#include "tst_timelinemodelaggregator.moc"
