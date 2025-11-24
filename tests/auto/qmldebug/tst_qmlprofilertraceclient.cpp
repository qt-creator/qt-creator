// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tst_qmlprofilertraceclient.h"

#include <qmldebug/qpacketprotocol.h>
#include <utils/temporarydirectory.h>

#include <QTest>
#include <QFutureInterface>

using namespace QmlDebug;
using namespace Utils;
using namespace Timeline;

ModelManager::ModelManager()
    : events("traceclienttest-data")
{
    QVERIFY(events.open());
}

int ModelManager::appendEventType(QmlEventType &&type)
{
    const int index = types.size();
    types.append(std::move(type));
    return index;
}

void ModelManager::appendEvent(QmlEvent &&event)
{
    events.append(std::move(event));
}

void ModelManager::replayQmlEvents(
    QmlEventLoader loader, Finalizer finalizer, ErrorHandler errorHandler) const
{
    switch (events.replay([&](const QmlEvent &event) {
        const QmlEventType &type = types[event.typeIndex()];
        loader(std::move(event), type);
        return true;
    })) {
    case TraceStashFile<QmlEvent>::ReplaySuccess:
        finalizer();
        return;
    case TraceStashFile<QmlEvent>::ReplayOpenFailed:
        errorHandler("Could not open trace file.");
        return;
    case TraceStashFile<QmlEvent>::ReplayLoadFailed:
        errorHandler("Event rejected by loader.");
        return;
    case TraceStashFile<QmlEvent>::ReplayReadPastEnd:
        errorHandler("Read past end in trace file.");
        return;
    }
}

void ModelManager::clearAll()
{
    events.clear();
    types.clear();
    QVERIFY(events.open());
}

void QmlProfilerTraceClientTest::initMain()
{
    TemporaryDirectory::setMasterTemporaryDirectory(
        QDir::tempPath() + "/QmlProfilerTraceClientTest" + "-XXXXXX");
}

QmlProfilerTraceClientTest::QmlProfilerTraceClientTest(QObject *parent)
    : QObject(parent)
    , traceClient(
        nullptr,
        std::bind(&ModelManager::appendEventType, &modelManager, std::placeholders::_1),
        std::bind(&ModelManager::appendEvent, &modelManager, std::placeholders::_1),
        std::numeric_limits<quint64>::max())
{
}

static void bufferCompressedFile(const QString &name, QBuffer &buffer)
{
    QFile file(name);
    QVERIFY(file.open(QIODevice::ReadOnly));
    buffer.setData(qUncompress(file.readAll()));
    file.close();
    QVERIFY(buffer.open(QIODevice::ReadOnly));
}

void QmlProfilerTraceClientTest::testMessageReceived()
{
    QBuffer inBuffer;
    bufferCompressedFile(":/qmlprofiler/tests/traces.dat", inBuffer);
    QDataStream inStream(&inBuffer);
    inStream.setVersion(QDataStream::Qt_5_6);

    QBuffer checkBuffer;
    bufferCompressedFile(":/qmlprofiler/tests/check.dat", checkBuffer);
    QDataStream checkStream(&checkBuffer);
    checkStream.setVersion(QDataStream::Qt_5_6);

    while (!inStream.atEnd()) {
        QByteArray trace;
        inStream >> trace;

        traceClient.stateChanged(QmlDebugClient::Enabled);
        QPacket packet(QDataStream::Qt_4_7, trace);
        while (!packet.atEnd()) {
            QByteArray content;
            packet >> content;
            traceClient.messageReceived(content);
        }
        traceClient.stateChanged(QmlDebugClient::NotConnected);

        modelManager.replayQmlEvents([&](const QmlEvent &event, const QmlEventType &type) {
            qint64 timestamp;
            quint8 message;
            quint8 rangeType;
            checkStream >> timestamp >> message >> rangeType;
            QVERIFY(message != MaximumMessage);
            QVERIFY(rangeType != MaximumRangeType);
            QVERIFY(type.message() != MaximumMessage);
            QVERIFY(type.rangeType() != MaximumRangeType);
            QCOMPARE(event.timestamp(), timestamp);
            QCOMPARE(type.message(), static_cast<Message>(message));
            QCOMPARE(type.rangeType(), static_cast<RangeType>(rangeType));
        }, [this]() {
            modelManager.clearAll();
            traceClient.clear();
        }, [this](const QString &message) {
            QVERIFY(!message.isEmpty());
            if (message == QStringLiteral("Read past end in trace file.")) {
                // Ignore read-past-end errors: Our test traces are somewhat dirty and don't end on
                //                              packet boundaries
                modelManager.clearAll();
                traceClient.clear();
            } else {
                QFAIL(message.toUtf8().constData());
            }
        });
    }

    QVERIFY(checkStream.atEnd());
}

QTEST_GUILESS_MAIN(QmlProfilerTraceClientTest)
