// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlpreviewclient_test.h"
#include <qmlpreview/qmlpreviewclient.h>
#include <qmldebug/qpacketprotocol.h>
#include <QtTest>

namespace QmlPreview {

class TestableQmlPreviewClient : public QmlPreviewClient
{
    Q_OBJECT
public:
    QByteArrayList messages;

    explicit TestableQmlPreviewClient() : QmlPreviewClient(nullptr) {}
    void sendMessage(const QByteArray &message) override
    {
        messages.append(message);
    }
};

void QmlPreviewClientTest::testLoadFile()
{
    TestableQmlPreviewClient client;
    QUrl url("file:///some/file.qml");
    client.loadUrl(url);
    QCOMPARE(client.messages.count(), 1);
    QmlDebug::QPacket packet(client.dataStreamVersion(), client.messages.takeFirst());
    qint8 command;
    QUrl sent;
    packet >> command >> sent;
    QCOMPARE(static_cast<QmlPreviewClient::Command>(command), QmlPreviewClient::Load);
    QCOMPARE(sent, url);
    QVERIFY(packet.atEnd());
}

void QmlPreviewClientTest::testAnnounceFile()
{
    TestableQmlPreviewClient client;
    QString file("/some/file.qml");
    QByteArray contents("tralala");
    client.announceFile(file, contents);
    QCOMPARE(client.messages.count(), 1);
    QmlDebug::QPacket packet(client.dataStreamVersion(), client.messages.takeFirst());
    qint8 command;
    QString sentFile;
    QByteArray sentContents;
    packet >> command >> sentFile >> sentContents;
    QCOMPARE(static_cast<QmlPreviewClient::Command>(command), QmlPreviewClient::File);
    QCOMPARE(sentFile, file);
    QCOMPARE(sentContents, contents);
    QVERIFY(packet.atEnd());
}

void QmlPreviewClientTest::testZoom()
{
    TestableQmlPreviewClient client;
    client.zoom(2.0f);
    QCOMPARE(client.messages.count(), 1);
    QmlDebug::QPacket packet(client.dataStreamVersion(), client.messages.takeFirst());
    qint8 command;
    float zoomFactor;
    packet >> command >> zoomFactor;
    QCOMPARE(static_cast<QmlPreviewClient::Command>(command), QmlPreviewClient::Zoom);
    QCOMPARE(zoomFactor, 2.0f);
    QVERIFY(packet.atEnd());
}

void QmlPreviewClientTest::testMessageReceived()
{
    TestableQmlPreviewClient client;
    QString file("/some/file.qml");
    QString error("wrong things");
    int numRequests = 0;
    connect(&client, &QmlPreviewClient::pathRequested, [&](const QString &requestedFile) {
        QCOMPARE(requestedFile, file);
        ++numRequests;
    });
    int numErrors = 0;
    connect(&client, &QmlPreviewClient::errorReported, [&](const QString &reportedError) {
        QCOMPARE(reportedError, error);
        ++numErrors;
    });
    quint16 numFrames = 0;
    connect(&client, &QmlPreviewClient::fpsReported, [&](const QmlPreviewClient::FpsInfo &frames) {
        numFrames += frames.numRenders;
    });
    {
        QmlDebug::QPacket packet(client.dataStreamVersion());
        packet << static_cast<qint8>(QmlPreviewClient::Request) << file;
        client.messageReceived(packet.data());
        QCOMPARE(numRequests, 1);
        QCOMPARE(numErrors, 0);
        QCOMPARE(numFrames, quint16(0));
    }
    {
        QmlDebug::QPacket packet(client.dataStreamVersion());
        packet << static_cast<qint8>(QmlPreviewClient::Error) << error;
        client.messageReceived(packet.data());
        QCOMPARE(numRequests, 1);
        QCOMPARE(numErrors, 1);
        QCOMPARE(numFrames, quint16(0));
    }
    {
        QmlDebug::QPacket packet(client.dataStreamVersion());
        quint16 frames = 58;
        quint16 one = 1, two = 2, three = 3, six = 6, seven = 7, eight = 8;
        packet << static_cast<qint8>(QmlPreviewClient::Fps) << frames << six << seven << eight
               << frames << one << two << three;
        client.messageReceived(packet.data());
        QCOMPARE(numRequests, 1);
        QCOMPARE(numErrors, 1);
        QCOMPARE(numFrames, frames);
    }
}

} // namespace QmlPreview

#include "qmlpreviewclient_test.moc"
