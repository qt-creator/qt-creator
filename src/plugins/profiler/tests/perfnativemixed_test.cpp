// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfnativemixed_test.h"

#include <profiler/perfprofilerconstants.h>
#include <profiler/perfprofilertracefile.h>
#include <profiler/perfprofilertracemanager.h>
#include <profiler/perftimelinemodel.h>
#include <profiler/perftimelinemodelmanager.h>

#include <QBuffer>
#include <QByteArray>
#include <QDataStream>
#include <QtEndian>
#include <QTest>

#include <functional>

namespace Profiler::Internal {

// This test proves the "native mixed" claim on the consumer side only: a single
// sampled call stack that interleaves C++ frames with QML/JS frames renders as
// one nested PerfTimelineModel stack, with the JS frames distinguishable from
// the C++ ones. It uses no protocol/struct change -- JS frames are ordinary
// perf locations whose Location points at the .qml source and whose Symbol
// carries the "[QML]" binary-name marker.
//
// It deliberately feeds a synthetic trace and does NOT exercise perf capture:
// whether perf/V4 can actually emit such merged stacks (jitdump for JIT'd JS,
// engine-side reconstruction for interpreted JS) is the separate producer-side
// work, in perfparser/qtdeclarative. This test pins down the data contract that
// work must satisfy. See native-mixed-profiler-design.md.

class PerfNativeMixedTest : public QObject
{
    Q_OBJECT
private slots:
    void testMergedStacks();
};

namespace {

// String table ids used in the synthetic trace.
enum StringId {
    StrMain, StrApp, StrMainCpp, StrEngine, StrQtQml,
    StrOnClicked, StrQmlMarker, StrMainQml, StrCompute, StrThread
};

// Location/symbol ids (shared key space, as perf uses it).
enum FrameId { LocMain, LocEngine, LocOnClicked, LocCompute };

const char *qmlMarker() { return "[QML]"; }

void appendFramed(QByteArray &out, const QByteArray &message)
{
    const quint32 size = qToLittleEndian<quint32>(quint32(message.size()));
    out.append(reinterpret_cast<const char *>(&size), sizeof(size));
    out.append(message);
}

QByteArray makeMessage(int version, const std::function<void(QDataStream &)> &fill)
{
    QByteArray message;
    QDataStream stream(&message, QIODevice::WriteOnly);
    stream.setVersion(version);
    fill(stream);
    return message;
}

QByteArray buildSyntheticTrace()
{
    const int version = QDataStream::Qt_DefaultCompiledVersion;

    QByteArray trace;
    trace.append(Constants::PerfStreamMagic, sizeof(Constants::PerfStreamMagic));
    const qint32 streamVersion = qToLittleEndian<qint32>(version);
    trace.append(reinterpret_cast<const char *>(&streamVersion), sizeof(streamVersion));

    const auto addString = [&](qint32 id, const QByteArray &value) {
        appendFramed(trace, makeMessage(version, [&](QDataStream &s) {
            s << quint8(PerfEventType::StringDefinition) << id << value;
        }));
    };
    addString(StrMain, "main");
    addString(StrApp, "app");
    addString(StrMainCpp, "/app/main.cpp");
    addString(StrEngine, "QV4::Function::call");
    addString(StrQtQml, "libQt6Qml.so");
    addString(StrOnClicked, "onClicked");
    addString(StrQmlMarker, qmlMarker());
    addString(StrMainQml, "Main.qml");
    addString(StrCompute, "compute");
    addString(StrThread, "worker");

    // One software attribute (id 0); the samples reference it.
    appendFramed(trace, makeMessage(version, [&](QDataStream &s) {
        PerfEventType::Attribute attribute;
        attribute.type = PerfEventType::TypeSoftware;
        attribute.config = 0;
        attribute.name = -1;
        attribute.usesFrequency = false;
        attribute.frequencyOrPeriod = 0;
        s << quint8(PerfEventType::AttributesDefinition) << qint32(0) << attribute;
    }));

    const auto addLocation = [&](qint32 id, quint64 address, qint32 file, qint32 line) {
        appendFramed(trace, makeMessage(version, [&](QDataStream &s) {
            PerfEventType::Location location;
            location.address = address;
            location.file = file;
            location.pid = 100;
            location.line = line;
            location.column = 1;
            location.parentLocationId = -1;
            s << quint8(PerfEventType::LocationDefinition) << id << location;
        }));
    };
    const auto addSymbol = [&](qint32 id, qint32 name, qint32 binary, qint32 path) {
        appendFramed(trace, makeMessage(version, [&](QDataStream &s) {
            PerfProfilerTraceManager::Symbol symbol;
            symbol.name = name;
            symbol.binary = binary;
            symbol.path = path;
            symbol.isKernel = false;
            s << quint8(PerfEventType::SymbolDefinition) << id << symbol;
        }));
    };

    // C++ frames.
    addLocation(LocMain, 0x1000, StrMainCpp, 10);
    addSymbol(LocMain, StrMain, StrApp, StrMainCpp);
    addLocation(LocEngine, 0x2000, -1, -1);
    addSymbol(LocEngine, StrEngine, StrQtQml, -1);
    // QML/JS frames: source is Main.qml, binary marker "[QML]".
    addLocation(LocOnClicked, 0x3000, StrMainQml, 42);
    addSymbol(LocOnClicked, StrOnClicked, StrQmlMarker, StrMainQml);
    addLocation(LocCompute, 0x3100, StrMainQml, 58);
    addSymbol(LocCompute, StrCompute, StrQmlMarker, StrMainQml);

    // A thread (pid != 0 marks it enabled).
    appendFramed(trace, makeMessage(version, [&](QDataStream &s) {
        PerfProfilerTraceManager::Thread thread;
        thread.pid = 100;
        thread.tid = 200;
        thread.start = 1000;
        thread.cpu = 0;
        thread.name = StrThread;
        s << quint8(PerfEventType::Command) << thread;
    }));

    // Samples. Frames are leaf-first; the stack is
    //   main -> QV4::Function::call -> onClicked -> compute
    // i.e. JS nested below C++.
    const auto addSample = [&](quint64 timestamp, const QList<qint32> &frames) {
        appendFramed(trace, makeMessage(version, [&](QDataStream &s) {
            s << quint8(PerfEventType::Sample)
              << quint32(100) << quint32(200) << timestamp << quint32(0)
              << frames << quint8(0)
              << QList<QPair<qint32, quint64>>{{qint32(0), quint64(1)}};
        }));
    };
    addSample(1000, {LocCompute, LocOnClicked, LocEngine, LocMain});
    addSample(1100, {LocCompute, LocOnClicked, LocEngine, LocMain});
    addSample(1200, {LocOnClicked, LocEngine, LocMain});
    addSample(1300, {LocEngine, LocMain});
    addSample(1400, {LocMain});

    return trace;
}

} // namespace

void PerfNativeMixedTest::testMergedStacks()
{
    PerfProfilerTraceManager &manager = traceManager();
    PerfTimelineModelManager modelManager;

    manager.clearAll();
    manager.initialize();

    QByteArray trace = buildSyntheticTrace();
    QBuffer buffer(&trace);
    QVERIFY(buffer.open(QIODevice::ReadOnly));

    {
        PerfProfilerTraceFile traceFile;
        traceFile.setTraceManager(&manager);
        traceFile.load(&buffer);
        QVERIFY(!traceFile.isCanceled());
    }

    manager.finalize();

    const QList<Timeline::TimelineModel *> models = modelManager.models();
    QCOMPARE(models.size(), 1);

    auto *model = static_cast<PerfTimelineModel *>(models.first());
    QVERIFY(model->count() > 0);

    bool sawCpp = false;
    bool sawQml = false;
    int deepestRow = -1;
    QByteArray deepestBinary;
    for (int i = 0; i < model->count(); ++i) {
        const int locationId = model->selectionId(i);
        if (locationId < 0)
            continue;
        const PerfProfilerTraceManager::Symbol &symbol = manager.symbol(locationId);
        if (symbol.binary < 0)
            continue;
        const QByteArray binary = manager.string(symbol.binary);
        if (binary == qmlMarker())
            sawQml = true;
        else if (binary == "app" || binary == "libQt6Qml.so")
            sawCpp = true;

        const int row = model->expandedRow(i);
        if (row > deepestRow) {
            deepestRow = row;
            deepestBinary = binary;
        }
    }

    // The merged stack contains both C++ and QML/JS frames ...
    QVERIFY(sawCpp);
    QVERIFY(sawQml);
    // ... and the deepest frame is a QML/JS one (JS nested below C++).
    QCOMPARE(deepestBinary, QByteArray(qmlMarker()));
}

QObject *createPerfNativeMixedTest()
{
    return new PerfNativeMixedTest;
}

} // namespace Profiler::Internal

#include "perfnativemixed_test.moc"
