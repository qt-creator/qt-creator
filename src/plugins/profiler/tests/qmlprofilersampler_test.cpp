// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilersampler_test.h"

#include "../qmlprofilermodelmanager.h"
#include "../qmlprofilersampler.h"

#include <qmldebug/qmlprofilereventtypes.h>
#include <qmldebug/qpacketprotocol.h>

#include <utils/result.h>
#include <utils/url.h>

#include <QtTaskTree/QSingleTaskTreeRunner>

#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>

using namespace QmlDebug;

namespace Profiler::Internal {

// Speaks just enough of the QML profiler protocol: it answers the hello, a
// start with a trace start and an animation frame, and a stop with the end of
// the trace. Its clock advances 1000 with every event, from 5000000, so the
// times the capture rebases are known. While held, it keeps its answers until
// released, as a busy server does.
class FakeProfilerServer : public QObject
{
public:
    FakeProfilerServer()
    {
        m_server.listen(QHostAddress(url.host()), url.port());
        connect(&m_server, &QTcpServer::newConnection, this, [this] {
            m_socket = m_server.nextPendingConnection();
            m_protocol = new QPacketProtocol(m_socket, m_socket);
            connect(m_protocol, &QPacketProtocol::readyRead, this, &FakeProfilerServer::read);
        });
    }

    void setHeld(bool held)
    {
        m_held = held;
        if (!held) {
            for (const QByteArray &answer : std::exchange(m_heldAnswers, {}))
                send(answer);
        }
    }

    const QUrl url = Utils::urlFromLocalHostAndFreePort();
    // Recording states asked for, in order. The first is the client's own
    // default, sent as it connects, before the capture has its say.
    QList<bool> requests;

private:
    void read()
    {
        while (m_protocol->packetsAvailable()) {
            QPacket packet(m_version, m_protocol->read());
            QString name;
            packet >> name;
            if (!m_greeted) {
                m_greeted = true;
                QPacket hello(QDataStream::Qt_4_7);
                hello << QString::fromLatin1("QDeclarativeDebugClient") << 0 << 1
                      << QStringList({"CanvasFrameRate", "EngineControl"})
                      << QList<float>({1.0f, 1.0f}) << m_version;
                m_protocol->send(hello.data());
                continue;
            }
            if (name != "CanvasFrameRate")
                continue;
            QByteArray message;
            packet >> message;
            QPacket request(m_version, message);
            bool recording = false;
            request >> recording;
            requests.append(recording);
            answer(recording);
        }
    }

    void answer(bool recording)
    {
        if (recording == m_recording)
            return;
        m_recording = recording;
        if (recording) {
            queue(event(Event, StartTrace));
            QPacket frame(m_version);
            frame << nextTime() << qint32(Event) << qint32(AnimationFrame) << qint32(60)
                  << qint32(1) << qint32(0);
            queue(frame.data());
        } else {
            const qint64 time = nextTime();
            queue(event(Event, EndTrace, time));
            queue(event(Complete, -1, time));
        }
    }

    QByteArray event(Message message, int subtype, std::optional<qint64> time = {})
    {
        QPacket packet(m_version);
        packet << (time ? *time : nextTime()) << qint32(message);
        if (subtype >= 0)
            packet << qint32(subtype);
        return packet.data();
    }

    void queue(const QByteArray &data)
    {
        if (m_held)
            m_heldAnswers.append(data);
        else
            send(data);
    }

    void send(const QByteArray &data)
    {
        QPacket packet(m_version);
        packet << QString::fromLatin1("CanvasFrameRate") << data;
        m_protocol->send(packet.data());
    }

    qint64 nextTime() { return m_time += 1000; }

    const int m_version = QDataStream::Qt_DefaultCompiledVersion;
    QTcpServer m_server;
    QTcpSocket *m_socket = nullptr;
    QPacketProtocol *m_protocol = nullptr;
    bool m_greeted = false;
    bool m_recording = false;
    bool m_held = false;
    QList<QByteArray> m_heldAnswers;
    qint64 m_time = 4'999'000;
};

// One capture of the QML profiler backend against the fake server.
class Capture
{
public:
    Capture(FakeProfilerServer &server, bool startPaused)
        : session(std::make_shared<RecordingSession>())
    {
        session->serverUrl = server.url;
        session->requestedFeatures = 1ULL << ProfileAnimations;
        if (startPaused)
            session->setPaused(true);
        runner.start(QtTaskTree::Group{sampler.captureRecipe(session)}, {},
                     [this](QtTaskTree::DoneWith) { done = true; });
    }

    QmlProfilerSampler sampler;
    std::shared_ptr<RecordingSession> session;
    QtTaskTree::QSingleTaskTreeRunner runner;
    bool done = false;
};

static void loadTrace(const Utils::FilePath &path, QmlProfilerModelManager &loaded)
{
    QSignalSpy loadSpy(&loaded, &QmlProfilerModelManager::loadFinished);
    loaded.load(path.toFSPathString());
    QTRY_COMPARE(loadSpy.count(), 1);
}

using Ranges = QList<std::pair<qint64, qint64>>;

void QmlProfilerSamplerTest::testPauseResumeStop()
{
    FakeProfilerServer server;
    Capture capture(server, false);
    QTRY_VERIFY(capture.session->isStarted());

    capture.session->setPaused(true);
    capture.session->setPaused(false);
    // The server answers in order, so its second start proves the pause is in.
    QTRY_COMPARE(server.requests, (QList<bool>{false, true, false, true}));
    capture.session->requestStop();
    QTRY_VERIFY(capture.done);
    QVERIFY(capture.session->result);
    QVERIFY_RESULT(*capture.session->result);

    QmlProfilerModelManager loaded;
    loadTrace(**capture.session->result, loaded);
    if (QTest::currentTestFailed())
        return;
    // Server times 5000000 to 5005000, on a timeline starting at the first trace.
    QCOMPARE(loaded.traceStart(), 0);
    QCOMPARE(loaded.traceEnd(), 5000);
    QCOMPARE(loaded.numEvents(), 2);
    QCOMPARE(loaded.pausedRanges(), (Ranges{{2000, 3000}}));
}

void QmlProfilerSamplerTest::testStartPausedThenStop()
{
    FakeProfilerServer server;
    Capture capture(server, true);
    // The server has heard that it is not to record yet.
    QTRY_COMPARE(server.requests, QList<bool>{false});

    capture.session->requestStop();
    QTRY_VERIFY(capture.done);
    QVERIFY(capture.session->result);
    QVERIFY(!*capture.session->result);
    QCOMPARE(capture.session->result->error(), stoppedBeforeResumeMessage());
}

// The recording must end with the trace the server starts for the resume,
// not at the end of the one the pause ends.
void QmlProfilerSamplerTest::testPauseResumeStopBeforeTheServerAnswers()
{
    FakeProfilerServer server;
    Capture capture(server, false);
    QTRY_VERIFY(capture.session->isStarted());

    server.setHeld(true);
    capture.session->setPaused(true);
    capture.session->setPaused(false);
    capture.session->requestStop();
    QTRY_COMPARE(server.requests, (QList<bool>{false, true, false, true, false}));
    server.setHeld(false);

    QTRY_VERIFY(capture.done);
    QVERIFY(capture.session->result);
    QVERIFY_RESULT(*capture.session->result);

    QmlProfilerModelManager loaded;
    loadTrace(**capture.session->result, loaded);
    if (QTest::currentTestFailed())
        return;
    QCOMPARE(loaded.numEvents(), 2);
    QCOMPARE(loaded.pausedRanges(), (Ranges{{2000, 3000}}));
}

} // namespace Profiler::Internal
