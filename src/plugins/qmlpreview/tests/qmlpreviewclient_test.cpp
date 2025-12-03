// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlpreviewclient_test.h"
#include "qiopipe.h"

#include <qmlpreview/qmlpreviewclient.h>
#include <qmldebug/qmldebugconnection.h>
#include <qmldebug/qpacketprotocol.h>

#include <QTest>

using namespace QmlDebug;
namespace QmlPreview {

class QmlPreviewClientTest : public QObject
{
    Q_OBJECT

private slots:
    void testLoadFile();
    void testAnnounceFile();
    void testZoom();
    void testMessageReceived();
};

class QmlPreviewTestRig : public QObject
{
    Q_OBJECT
public:
    QmlPreviewTestRig()
    {
        m_pipe.open(QIODevice::ReadWrite);

        m_connection = new QmlDebugConnection(this);
        m_client = new QmlPreviewClient(m_connection);
        m_connection->assumeServerPlugins();

        m_connection->setDevice(m_pipe.takeEnd1());
        m_protocol = new QPacketProtocol(m_pipe.end2(), this);

        connect(m_protocol, &QPacketProtocol::readyRead, this, [this]() {
            while (m_protocol->packetsAvailable()) {
                QPacket pack(m_connection->currentDataStreamVersion(), m_protocol->read());
                NamedMessage message;
                pack >> message.name >> message.message;
                m_messages.append(std::move(message));
            }
        });
    }

    QmlPreviewClient *client() const { return m_client; }

    QByteArray nextMessage(const QString &name)
    {
        if (m_messages.first().name != name)
            return {};
        return m_messages.takeFirst().message;
    }

    qsizetype messageCount() const { return m_messages.size(); }

private:

    struct NamedMessage {
        QString name;
        QByteArray message;
    };

    QList<NamedMessage> m_messages;
    QIOPipe m_pipe;
    QmlDebugConnection *m_connection = nullptr;
    QmlPreviewClient *m_client = nullptr;
    QPacketProtocol *m_protocol = nullptr;
};

void QmlPreviewClientTest::testLoadFile()
{
    QmlPreviewTestRig rig;
    QUrl url("file:///some/file.qml");
    rig.client()->loadUrl(url);
    QCOMPARE(rig.messageCount(), 1);
    QPacket packet(rig.client()->dataStreamVersion(), rig.nextMessage(rig.client()->name()));
    qint8 command;
    QUrl sent;
    packet >> command >> sent;
    QCOMPARE(static_cast<QmlPreviewClient::Command>(command), QmlPreviewClient::Load);
    QCOMPARE(sent, url);
    QVERIFY(packet.atEnd());
}

void QmlPreviewClientTest::testAnnounceFile()
{
    QmlPreviewTestRig rig;
    QString file("/some/file.qml");
    QByteArray contents("tralala");
    rig.client()->announceFile(file, contents);
    QCOMPARE(rig.messageCount(), 1);
    QPacket packet(rig.client()->dataStreamVersion(), rig.nextMessage(rig.client()->name()));
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
    QmlPreviewTestRig rig;
    rig.client()->zoom(2.0f);
    QCOMPARE(rig.messageCount(), 1);
    QPacket packet(rig.client()->dataStreamVersion(), rig.nextMessage(rig.client()->name()));
    qint8 command;
    float zoomFactor;
    packet >> command >> zoomFactor;
    QCOMPARE(static_cast<QmlPreviewClient::Command>(command), QmlPreviewClient::Zoom);
    QCOMPARE(zoomFactor, 2.0f);
    QVERIFY(packet.atEnd());
}

void QmlPreviewClientTest::testMessageReceived()
{
    QmlPreviewTestRig rig;
    QString file("/some/file.qml");
    QString error("wrong things");
    int numRequests = 0;
    connect(rig.client(), &QmlPreviewClient::pathRequested, [&](const QString &requestedFile) {
        QCOMPARE(requestedFile, file);
        ++numRequests;
    });
    int numErrors = 0;
    connect(rig.client(), &QmlPreviewClient::errorReported, [&](const QString &reportedError) {
        QCOMPARE(reportedError, error);
        ++numErrors;
    });
    quint16 numFrames = 0;
    connect(rig.client(), &QmlPreviewClient::fpsReported,
            [&](const QmlPreviewClient::FpsInfo &frames) {
        numFrames += frames.numRenders;
    });
    {
        QPacket packet(rig.client()->dataStreamVersion());
        packet << static_cast<qint8>(QmlPreviewClient::Request) << file;
        rig.client()->messageReceived(packet.data());
        QCOMPARE(numRequests, 1);
        QCOMPARE(numErrors, 0);
        QCOMPARE(numFrames, quint16(0));
    }
    {
        QPacket packet(rig.client()->dataStreamVersion());
        packet << static_cast<qint8>(QmlPreviewClient::Error) << error;
        rig.client()->messageReceived(packet.data());
        QCOMPARE(numRequests, 1);
        QCOMPARE(numErrors, 1);
        QCOMPARE(numFrames, quint16(0));
    }
    {
        QPacket packet(rig.client()->dataStreamVersion());
        quint16 frames = 58;
        quint16 one = 1, two = 2, three = 3, six = 6, seven = 7, eight = 8;
        packet << static_cast<qint8>(QmlPreviewClient::Fps) << frames << six << seven << eight
               << frames << one << two << three;
        rig.client()->messageReceived(packet.data());
        QCOMPARE(numRequests, 1);
        QCOMPARE(numErrors, 1);
        QCOMPARE(numFrames, frames);
    }
}

QObject *createQmlPreviewClientTest()
{
    return new QmlPreviewClientTest;
}

} // namespace QmlPreview

#include "qmlpreviewclient_test.moc"
