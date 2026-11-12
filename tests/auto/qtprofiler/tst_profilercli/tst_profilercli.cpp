// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QtTest>

#ifdef Q_OS_WIN
#include <windows.h>
#include <imagehlp.h>
#endif

using namespace Qt::StringLiterals;

// Every wait on the tool is this short, so that a tool that never answers fails
// each case on its own instead of running the whole test into its timeout.
constexpr int waitMs = 4000;

static QString cliPath()
{
    QString path = QCoreApplication::applicationDirPath() + '/' + TEST_RELATIVE_LIBEXEC_PATH
                   + "/qtprofiler-cli";
    if (QSysInfo::productType() == "windows")
        path += ".exe";
    return QDir::cleanPath(path);
}

static QByteArray peekChannel(QProcess &process, QProcess::ProcessChannel channel)
{
    const QProcess::ProcessChannel previous = process.readChannel();
    process.setReadChannel(channel);
    const QByteArray data = process.peek(process.bytesAvailable());
    process.setReadChannel(previous);
    return data;
}

#ifdef Q_OS_WIN
// The first DLL that `image` needs, directly or not, that the search path does
// not have, with the module that imports it; empty when all are there.
static QString missingDll(const QByteArray &image, const QByteArray &searchPath,
                          QSet<QByteArray> &seen)
{
    LOADED_IMAGE loaded;
    if (!MapAndLoad(image.constData(), searchPath.constData(), &loaded, TRUE, TRUE))
        return QString::fromLocal8Bit(image);
    QString missing;
    ULONG size = 0;
    auto descriptor = static_cast<PIMAGE_IMPORT_DESCRIPTOR>(
        ImageDirectoryEntryToData(loaded.MappedAddress, FALSE, IMAGE_DIRECTORY_ENTRY_IMPORT, &size));
    for (; descriptor && descriptor->Name && missing.isEmpty(); ++descriptor) {
        const QByteArray name(static_cast<const char *>(
            ImageRvaToVa(loaded.FileHeader, loaded.MappedAddress, descriptor->Name, nullptr)));
        const QByteArray key = name.toLower();
        if (key.startsWith("api-ms-") || key.startsWith("ext-ms-") || seen.contains(key))
            continue;
        seen.insert(key);
        missing = missingDll(name, searchPath, seen);
        if (!missing.isEmpty() && !missing.contains(u' '))
            missing += u" (needed by %1)"_s.arg(QString::fromLocal8Bit(loaded.ModuleName));
    }
    UnMapAndLoad(&loaded);
    return missing;
}
#endif

// What became of the tool, for a failure message. It leaves the output unread.
static QByteArray describe(QProcess &process)
{
    QString missing;
#ifdef Q_OS_WIN
    if (quint32(process.exitCode()) == 0xc0000135) { // STATUS_DLL_NOT_FOUND
        // The loader searches the tool's own directory first, then PATH.
        const QByteArray searchPath = QDir::toNativeSeparators(QFileInfo(cliPath()).path()).toLocal8Bit()
                                      + ';' + qgetenv("PATH");
        QSet<QByteArray> seen;
        missing = u"\nmissing: "_s
                  + missingDll(QDir::toNativeSeparators(cliPath()).toLocal8Bit(), searchPath, seen);
    }
#endif
    return (u"%1 %2: state %3, error %4 (%5), exit status %6, exit code 0x%7\n"
            "stdout: %8\nstderr: %9"_s
        .arg(process.program(), process.arguments().join(u' '))
        .arg(int(process.state()))
        .arg(int(process.error()))
        .arg(process.errorString())
        .arg(int(process.exitStatus()))
        .arg(quint32(process.exitCode()), 0, 16)
        .arg(QString::fromLocal8Bit(peekChannel(process, QProcess::StandardOutput)),
             QString::fromLocal8Bit(peekChannel(process, QProcess::StandardError)))
        + missing)
        .toLocal8Bit();
}

// Runs the tool to its end, with `arguments`.
static bool run(QProcess &process, const QStringList &arguments)
{
    process.start(cliPath(), arguments);
    return process.waitForFinished(waitMs);
}

// The tool started with --rpc, read one message at a time.
class Cli
{
public:
    Cli()
    {
        m_process.start(cliPath(), {u"--rpc"_s});
    }

    ~Cli()
    {
        if (m_process.state() != QProcess::NotRunning) {
            m_process.kill();
            m_process.waitForFinished(waitMs);
        }
    }

    bool started() { return m_process.waitForStarted(waitMs); }

    void send(const QByteArray &line) { m_process.write(line + '\n'); }

    void request(int id, const QString &method, const QJsonObject &params = {})
    {
        QJsonObject request{{u"jsonrpc"_s, u"2.0"_s}, {u"id"_s, id}, {u"method"_s, method}};
        if (!params.isEmpty())
            request.insert(u"params"_s, params);
        send(QJsonDocument(request).toJson(QJsonDocument::Compact));
    }

    // The next message on stdout, or an empty object when none came in time.
    QJsonObject next()
    {
        while (!m_process.canReadLine()) {
            if (!m_process.waitForReadyRead(waitMs))
                return {};
        }
        return QJsonDocument::fromJson(m_process.readLine()).object();
    }

    // The reply to request `id`, and the notifications that came before it. Without
    // one, it says what became of the tool, which the failing check cannot.
    QJsonObject reply(int id, QStringList *notificationsBefore = nullptr)
    {
        for (QJsonObject message = next(); !message.isEmpty(); message = next()) {
            if (message.value("id"_L1) == QJsonValue(id))
                return message;
            if (notificationsBefore)
                notificationsBefore->append(message.value("method"_L1).toString());
        }
        qWarning("No reply to request %d. %s", id, describe().constData());
        return {};
    }

    static int errorCode(const QJsonObject &reply)
    {
        return reply.value("error"_L1).toObject().value("code"_L1).toInt();
    }

    void closeInput() { m_process.closeWriteChannel(); }

    bool exitedCleanly()
    {
        return m_process.waitForFinished(waitMs) && m_process.exitStatus() == QProcess::NormalExit
               && m_process.exitCode() == 0;
    }

    QByteArray describe() { return ::describe(m_process); }

private:
    QProcess m_process;
};

class tst_ProfilerCli : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
#ifdef Q_OS_WIN
        // A tool that cannot be loaded would otherwise wait on the loader's
        // error box for good; the tool inherits this, and exits with the error.
        SetErrorMode(GetErrorMode() | SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif
        QVERIFY2(QFileInfo(cliPath()).isExecutable(), qPrintable(cliPath()));
    }

    void versionGoesToStdout()
    {
        QProcess process;
        QVERIFY2(run(process, {u"--version"_s}), describe(process).constData());
        QVERIFY2(process.exitCode() == 0, describe(process).constData());
        QVERIFY2(process.readAllStandardOutput().contains("QtProfilerCli"),
                 describe(process).constData());
    }

    // An installed tool finds the plugins it links on its own; only a test
    // runner puts the plugin directory on PATH.
    void startsWithoutThePluginDirectoryOnPath()
    {
        if (QSysInfo::productType() != "windows")
            QSKIP("Elsewhere the rpath says where the plugins are.");
        const QString pluginDirectory = QDir::cleanPath(QFileInfo(cliPath()).path() + '/'
                                                        + RELATIVE_PLUGIN_PATH);
        QStringList path = qEnvironmentVariable("PATH").split(u';', Qt::SkipEmptyParts);
        const qsizetype removed = path.removeIf([&pluginDirectory](const QString &entry) {
            return QDir::cleanPath(QDir::fromNativeSeparators(entry))
                       .compare(pluginDirectory, Qt::CaseInsensitive) == 0;
        });
        QVERIFY2(removed > 0, qPrintable(pluginDirectory + " is not on PATH"));
        QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
        environment.insert(u"PATH"_s, path.join(u';'));

        QProcess process;
        process.setProcessEnvironment(environment);
        QVERIFY2(run(process, {u"--version"_s}), describe(process).constData());
        QVERIFY2(process.exitCode() == 0, describe(process).constData());
        QVERIFY2(process.readAllStandardOutput().contains("QtProfilerCli"),
                 describe(process).constData());
    }

    void listBackends()
    {
        QProcess process;
        QVERIFY2(run(process, {u"--list-backends"_s}), describe(process).constData());
        QVERIFY2(process.exitCode() == 0, describe(process).constData());
        const QList<QByteArray> lines = process.readAllStandardOutput().trimmed().split('\n');
        QVERIFY(!lines.isEmpty());
        for (const QByteArray &line : lines)
            QCOMPARE(line.count('\t'), 1);
    }

    void initialize()
    {
        Cli cli;
        QVERIFY2(cli.started(), cli.describe().constData());
        cli.request(1, u"initialize"_s);
        const QJsonObject result = cli.reply(1).value("result"_L1).toObject();
        QCOMPARE(result.value("protocolVersion"_L1).toInt(), 1);
        QVERIFY(!result.value("backends"_L1).toArray().isEmpty());
    }

    void malformedRequests()
    {
        Cli cli;
        QVERIFY2(cli.started(), cli.describe().constData());
        cli.send("{not json");
        QCOMPARE(Cli::errorCode(cli.next()), -32700);
        cli.send(R"({"jsonrpc":"2.0","id":2})");
        QCOMPARE(Cli::errorCode(cli.reply(2)), -32600);
        cli.request(3, u"noSuchMethod"_s);
        QCOMPARE(Cli::errorCode(cli.reply(3)), -32601);
    }

    void idleState()
    {
        Cli cli;
        QVERIFY2(cli.started(), cli.describe().constData());
        cli.request(1, u"getStatus"_s);
        QCOMPARE(cli.reply(1).value("result"_L1).toObject().value("state"_L1).toString(),
                 u"idle"_s);
        cli.request(2, u"stopRecording"_s);
        QCOMPARE(Cli::errorCode(cli.reply(2)), -32001);
        cli.request(3, u"pauseRecording"_s);
        QCOMPARE(Cli::errorCode(cli.reply(3)), -32001);
    }

    void invalidStartParameters()
    {
        Cli cli;
        QVERIFY2(cli.started(), cli.describe().constData());
        cli.request(1, u"startRecording"_s, {{u"backend"_s, u"NoSuchBackend"_s}});
        QCOMPARE(Cli::errorCode(cli.reply(1)), -32602);
        cli.request(2, u"initialize"_s);
        const QString backend = cli.reply(2)
                                    .value("result"_L1).toObject()
                                    .value("backends"_L1).toArray()
                                    .first().toObject()
                                    .value("id"_L1).toString();
        cli.request(3, u"startRecording"_s,
                    {{u"backend"_s, backend},
                     {u"options"_s, QJsonObject{{u"NoSuchOption"_s, 1}}}});
        QCOMPARE(Cli::errorCode(cli.reply(3)), -32602);
    }

    void failedStartIsAnError_data()
    {
        QTest::addColumn<QString>("executable");
        QTest::addColumn<QString>("message");
        QTest::newRow("no executable") << QString() << u"Set an executable"_s;
        QTest::newRow("missing executable") << u"/nonexistent/app"_s << u"/nonexistent/app"_s;
    }

    // A start that fails at once is answered with the error, and is not also
    // reported as a recording that started and then failed.
    void failedStartIsAnError()
    {
        QFETCH(QString, executable);
        QFETCH(QString, message);
        Cli cli;
        QVERIFY2(cli.started(), cli.describe().constData());
        QJsonObject params{{u"backend"_s, u"QtTrace"_s}};
        if (!executable.isEmpty())
            params.insert(u"executable"_s, executable);
        cli.request(1, u"startRecording"_s, params);
        QStringList before;
        const QJsonObject reply = cli.reply(1, &before);
        QCOMPARE(before, QStringList());
        QCOMPARE(Cli::errorCode(reply), -32002);
        const QString text = reply.value("error"_L1).toObject().value("message"_L1).toString();
        QVERIFY2(text.contains(message), qPrintable(text));

        // Nothing about the failed start arrives later either: this is answered
        // after anything it would have sent.
        cli.request(2, u"getStatus"_s);
        before.clear();
        QCOMPARE(cli.reply(2, &before).value("result"_L1).toObject().value("state"_L1).toString(),
                 u"idle"_s);
        QCOMPARE(before, QStringList());
    }

    // The tool keeps itself alive through a hang-up, but what it launches gets
    // the default action.
    void launchedTargetGetsDefaultHangUp()
    {
        if (QSysInfo::productType() == "windows")
            QSKIP("There is no SIGHUP on Windows.");
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString script = directory.filePath(u"hangup.sh"_s);
        QFile file(script);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("#!/bin/sh\nkill -HUP $$\necho survived the hang-up\n");
        file.close();
        QVERIFY(file.setPermissions(file.permissions() | QFile::ExeOwner));

        // The target's output goes to the tool's stderr, and the tool reports
        // the end of the recording only after the target has ended.
        QProcess process;
        QVERIFY2(run(process, {u"--backend"_s, u"QtTrace"_s, u"--launch"_s, script}),
                 describe(process).constData());
        const QByteArray errors = process.readAllStandardError();
        QVERIFY2(!errors.contains("survived the hang-up"), errors.constData());
    }

    void shutdownExits()
    {
        Cli cli;
        QVERIFY2(cli.started(), cli.describe().constData());
        cli.request(1, u"shutdown"_s);
        QVERIFY(cli.reply(1).contains("result"_L1));
        QVERIFY2(cli.exitedCleanly(), cli.describe().constData());
    }

    void endOfInputExits()
    {
        Cli cli;
        QVERIFY2(cli.started(), cli.describe().constData());
        cli.closeInput();
        QVERIFY2(cli.exitedCleanly(), cli.describe().constData());
    }
};

QTEST_GUILESS_MAIN(tst_ProfilerCli)

#include "tst_profilercli.moc"
