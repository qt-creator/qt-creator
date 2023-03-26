// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfprofilertracefile_test.h"
#include <perfprofiler/perfprofilertracefile.h>
#include <perfprofiler/perftimelinemodelmanager.h>
#include <perfprofiler/perftimelinemodel.h>
#include <utils/temporaryfile.h>

#include <QtTest>

namespace PerfProfiler {
namespace Internal {

PerfProfilerTraceFileTest::PerfProfilerTraceFileTest(QObject *parent) : QObject(parent)
{
}

static void checkModelContainsTraceData(PerfTimelineModel *model)
{
    bool sampleFound = false;
    for (int i = 0; i < model->count(); ++i) {
        if (model->isSample(i)) {
            QCOMPARE(model->details(i).value("__probe_ip").toString(),
                     QString("0x000055d1db028560"));
            sampleFound = true;
        }
    }
    QVERIFY(sampleFound);
}

static void checkModels(PerfProfilerTraceManager *traceManager,
                        PerfTimelineModelManager *modelManager)
{
    traceManager->finalize();

    const QVariantList perfModels = modelManager->models();

    QCOMPARE(traceManager->locations().size(), static_cast<size_t>(2));
    QCOMPARE(traceManager->symbols().count(), 1);
    QCOMPARE(traceManager->attributes().size(),
             static_cast<size_t>(1 - PerfEvent::LastSpecialTypeId));
    QCOMPARE(traceManager->tracePoints().count(), 1);

    QCOMPARE(perfModels.size(), 1);

    checkModelContainsTraceData(qvariant_cast<PerfTimelineModel *>(perfModels[0]));
}

struct MessageHandler {
    MessageHandler(QtMessageHandler handler)
    {
        defaultHandler = qInstallMessageHandler(handler);
    }

    ~MessageHandler()
    {
        qInstallMessageHandler(defaultHandler);
    }

    static QtMessageHandler defaultHandler;
};

QtMessageHandler MessageHandler::defaultHandler = nullptr;

static void handleMessage(QtMsgType type, const QMessageLogContext &context, const QString &string)
{
    QVERIFY(type != QtWarningMsg || !string.contains("Read only part of message"));
    MessageHandler::defaultHandler(type, context, string);
}

void PerfProfilerTraceFileTest::testSaveLoadTraceData()
{
    MessageHandler messageHandler(&handleMessage);
    PerfProfilerTraceManager traceManager;
    PerfTimelineModelManager modelManager(&traceManager);
    {
        PerfProfilerTraceFile traceFile;
        traceFile.setTraceManager(&traceManager);
        QFile file(":/perfprofiler/tests/probe.file.ptr");
        QVERIFY(file.open(QIODevice::ReadOnly));
        traceManager.initialize();
        traceFile.load(&file);
        QVERIFY(!traceFile.isCanceled());
    }
    checkModels(&traceManager, &modelManager);

    Utils::TemporaryFile file("perftesttrace");
    QVERIFY(file.open());

    {
        PerfProfilerTraceFile traceFile;
        traceFile.setTraceManager(&traceManager);
        traceFile.save(&file);
        file.flush();
        QVERIFY(!traceFile.isCanceled());
    }

    traceManager.clearAll();
    QCOMPARE(traceManager.locations().size(), static_cast<size_t>(0));
    QCOMPARE(traceManager.symbols().count(), 0);
    QCOMPARE(traceManager.attributes().size(), static_cast<size_t>(5));
    QCOMPARE(traceManager.tracePoints().count(), 0);
    QCOMPARE(modelManager.models().size(), 0);

    {
        PerfProfilerTraceFile traceFile;
        traceFile.setTraceManager(&traceManager);
        QFile generatedFile(file.fileName());
        QVERIFY(generatedFile.open(QIODevice::ReadOnly));
        traceManager.initialize();
        traceFile.load(&generatedFile);
        QVERIFY(!traceFile.isCanceled());
    }

    checkModels(&traceManager, &modelManager);

}

} // namespace Internal
} // namespace PerfProfiler
