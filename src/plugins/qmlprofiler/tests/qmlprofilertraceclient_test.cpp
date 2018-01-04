/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "qmlprofilertraceclient_test.h"
#include <qmldebug/qpacketprotocol.h>
#include <QtTest>

namespace QmlProfiler {
namespace Internal {

QmlProfilerTraceClientTest::QmlProfilerTraceClientTest(QObject *parent) : QObject(parent),
    traceClient(nullptr, &modelManager, std::numeric_limits<quint64>::max())
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

        traceClient.stateChanged(QmlDebug::QmlDebugClient::Enabled);
        QmlDebug::QPacket packet(QDataStream::Qt_4_7, trace);
        while (!packet.atEnd()) {
            QByteArray content;
            packet >> content;
            traceClient.messageReceived(content);
        }
        traceClient.stateChanged(QmlDebug::QmlDebugClient::NotConnected);

        modelManager.replayEvents(-1, -1, [&](const QmlEvent &event, const QmlEventType &type) {
            qint64 timestamp;
            qint32 message;
            qint32 rangeType;
            checkStream >> timestamp >> message >> rangeType;
            QCOMPARE(event.timestamp(), timestamp);
            QCOMPARE(type.message(), static_cast<Message>(message));
            QCOMPARE(type.rangeType(), static_cast<RangeType>(rangeType));
        });

        modelManager.clear();
        traceClient.clear();
    }

    QVERIFY(checkStream.atEnd());
}

} // namespace Internal
} // namespace QmlProfiler
