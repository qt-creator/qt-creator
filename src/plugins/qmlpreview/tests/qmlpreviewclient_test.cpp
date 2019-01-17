/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qmlpreviewclient_test.h"
#include <qmlpreview/qmlpreviewclient.h>
#include <qmldebug/qpacketprotocol.h>
#include <QtTest>

namespace QmlPreview {
namespace Internal {

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

void QmlPreviewClientTest::testLanguate()
{
    TestableQmlPreviewClient client;
    QUrl url("file:///some/file.qml");
    QString locale("qt_QT");
    client.language(url, locale);
    QCOMPARE(client.messages.count(), 1);
    QmlDebug::QPacket packet(client.dataStreamVersion(), client.messages.takeFirst());
    qint8 command;
    QUrl resultUrl;
    QString resultLocale;
    packet >> command >> resultUrl >> resultLocale;
    QCOMPARE(static_cast<QmlPreviewClient::Command>(command), QmlPreviewClient::Language);
    QCOMPARE(resultUrl, url);
    QCOMPARE(resultLocale, locale);
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
        packet << static_cast<qint8>(QmlPreviewClient::Fps) << frames << 6 << 7 << 8
               << frames << 1 << 2 << 3;
        client.messageReceived(packet.data());
        QCOMPARE(numRequests, 1);
        QCOMPARE(numErrors, 1);
        QCOMPARE(numFrames, frames);
    }
}

} // namespace Internal
} // namespace QmlPreview

#include "qmlpreviewclient_test.moc"
