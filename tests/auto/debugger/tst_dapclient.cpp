// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dap/dapclient.h"

#include <QTest>

using namespace Debugger::Internal;

// Feeds the client whatever a test writes, in the chunks it writes it: what
// arrives in one read() is what the peer happened to flush, not a message.
class ChunkProvider : public IDataProvider
{
public:
    void feed(const QByteArray &chunk)
    {
        m_pending.append(chunk);
        emit readyReadStandardOutput();
    }

    void finish() { emit done(); }

    void start() override {}
    bool isRunning() const override { return true; }
    void writeRaw(const QByteArray &) override {}
    void kill() override {}
    void interrupt() override {}
    QByteArray readAllStandardOutput() override { return std::exchange(m_pending, {}); }
    QString readAllStandardError() override { return {}; }
    int exitCode() const override { return 0; }
    QString executable() const override { return "fake"; }
    QProcess::ExitStatus exitStatus() const override { return QProcess::NormalExit; }
    QProcess::ProcessError error() const override { return QProcess::UnknownError; }
    Utils::ProcessResult result() const override { return Utils::ProcessResult::FinishedWithSuccess; }
    QString exitMessage() const override { return {}; }

private:
    QByteArray m_pending;
};

class TestClient : public DapClient
{
public:
    using DapClient::DapClient;

private:
    const QLoggingCategory &logCategory() override
    {
        static const QLoggingCategory category("qtc.dbg.tst_dapclient", QtWarningMsg);
        return category;
    }
};

static QByteArray framed(const QByteArray &body)
{
    return "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body;
}

class tst_DapClient : public QObject
{
    Q_OBJECT

private slots:
    void reportsUnframedOutputOnce();
    void reportsUnframedOutputSplitAcrossChunks();
    void reportsWhatIsLeftWhenTheSessionEnds();
};

void tst_DapClient::reportsUnframedOutputOnce()
{
    ChunkProvider provider;
    TestClient client(&provider);
    QStringList unframed;
    connect(&client, &DapClient::unframedOutput, this,
            [&unframed](const QString &text) { unframed.append(text); });
    QList<DapEventType> events;
    connect(&client, &DapClient::eventReady, this,
            [&events](DapEventType type, const QJsonObject &) { events.append(type); });

    provider.feed("warning: .gdbinit failed\n"
                  + framed(R"({"type":"event","event":"initialized"})"));

    QCOMPARE(unframed, QStringList{"warning: .gdbinit failed"});
    QCOMPARE(events.size(), 1);
}

void tst_DapClient::reportsUnframedOutputSplitAcrossChunks()
{
    ChunkProvider provider;
    TestClient client(&provider);
    QStringList unframed;
    connect(&client, &DapClient::unframedOutput, this,
            [&unframed](const QString &text) { unframed.append(text); });
    QList<DapEventType> events;
    connect(&client, &DapClient::eventReady, this,
            [&events](DapEventType type, const QJsonObject &) { events.append(type); });

    // The message arrives in pieces, as a pipe hands it over. The output in
    // front of it must be reported once, not with every piece.
    const QByteArray message = framed(R"({"type":"event","event":"initialized"})");
    provider.feed("warning: .gdbinit failed\n" + message.left(12));
    provider.feed(message.mid(12, 20));
    provider.feed(message.mid(32));

    QCOMPARE(unframed, QStringList{"warning: .gdbinit failed"});
    QCOMPARE(events.size(), 1);
}

void tst_DapClient::reportsWhatIsLeftWhenTheSessionEnds()
{
    ChunkProvider provider;
    TestClient client(&provider);
    QStringList unframed;
    connect(&client, &DapClient::unframedOutput, this,
            [&unframed](const QString &text) { unframed.append(text); });

    // A session that dies before it ever frames anything: the reason is in
    // those bytes, and nothing else will report them.
    provider.feed("gdb: unrecognized option '--nonsense'\n");
    QCOMPARE(unframed, QStringList{});

    provider.finish();
    QCOMPARE(unframed, QStringList{"gdb: unrecognized option '--nonsense'"});
}

QTEST_GUILESS_MAIN(tst_DapClient)

#include "tst_dapclient.moc"
