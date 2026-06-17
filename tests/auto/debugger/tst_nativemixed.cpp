// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Autotest for native combined (C++ + QML in one engine) debugging. It
// builds the qmlmix fixture, then drives a real gdb or lldb session through
// the Python bridge (nativemixed_driver.py) and checks the outcomes of
// QML/C++ cross-stepping and the spliced mixed stack.
//
// The checks that need the qtdeclarative qt_v4AboutToCallNativeMethodHook
// (QML->C++ step-in, C++->QML step-out, the C++-frame stack splice) only
// work with Qt >= 6.12, so they are required only there and ignored on
// older Qt. The fixture also needs the qmldbg_native plugin; the test skips
// if the debugger or the fixture build is unavailable.

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QProcess>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTest>

// Off by default; enable the verbose cmake and debugger output with
// QT_LOGGING_RULES="qtc.debugger.nativemixed.debug=true".
Q_LOGGING_CATEGORY(lcNativeMixed, "qtc.debugger.nativemixed", QtWarningMsg)

enum Engine { GdbEngine, LldbEngine };

// The hook lands in Qt 6.12; bump this if it slips to a later release.
static const int HookQtVersion = 0x060c00;

class tst_NativeMixed : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void nativeMixed();
    void cleanupTestCase();

private:
    bool buildFixture();

    Engine m_engine = GdbEngine;
    QString m_debuggerBinary;
    QString m_qmakeBinary;
    QString m_qtPrefix;
    int m_qtVersion = 0;
    const QString m_dumperDir = QString::fromLocal8Bit(DUMPERDIR);
    const QString m_driver = QString::fromLocal8Bit(NATIVEMIXED_DRIVER);
    const QString m_fixtureSrc = QString::fromLocal8Bit(QMLMIX_SOURCE_DIR);
    QTemporaryDir m_buildDir;
    QString m_exe;
    bool m_fixtureOk = false;
};

static int parseQtVersion(const QString &s)
{
    const QStringList parts = s.trimmed().split('.');
    if (parts.size() < 2)
        return 0;
    const int major = parts.at(0).toInt();
    const int minor = parts.at(1).toInt();
    const int patch = parts.size() > 2 ? parts.at(2).toInt() : 0;
    return (major << 16) | (minor << 8) | patch;
}

static QString runAndCapture(const QString &exe, const QStringList &args,
                             const QString &workingDir = {})
{
    QProcess p;
    if (!workingDir.isEmpty())
        p.setWorkingDirectory(workingDir);
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.start(exe, args);
    if (!p.waitForStarted() || !p.waitForFinished(180000))
        return {};
    return QString::fromLocal8Bit(p.readAll());
}

void tst_NativeMixed::initTestCase()
{
    m_debuggerBinary = QString::fromLocal8Bit(qgetenv("QTC_DEBUGGER_PATH_FOR_TEST"));
    if (m_debuggerBinary.isEmpty())
        m_debuggerBinary = "gdb";
    const QString base = QFileInfo(m_debuggerBinary).baseName().toLower();
    m_engine = base.contains("lldb") ? LldbEngine : GdbEngine;

    m_qmakeBinary = QString::fromLocal8Bit(qgetenv("QTC_QMAKE_PATH_FOR_TEST"));
    if (m_qmakeBinary.isEmpty())
        m_qmakeBinary = QString::fromLocal8Bit(DEFAULT_QMAKE_BINARY);

    m_qtVersion = parseQtVersion(
        runAndCapture(m_qmakeBinary, {"-query", "QT_VERSION"}));
    m_qtPrefix = runAndCapture(m_qmakeBinary, {"-query", "QT_INSTALL_PREFIX"}).trimmed();
    QVERIFY2(m_qtVersion != 0, "Could not determine the Qt version under test.");

    m_fixtureOk = buildFixture();
}

bool tst_NativeMixed::buildFixture()
{
    if (!m_buildDir.isValid())
        return false;
    const QString build = m_buildDir.path();

    QStringList configure = {"-S", m_fixtureSrc, "-B", build,
                             "-DCMAKE_BUILD_TYPE=Debug"};
    if (!m_qtPrefix.isEmpty())
        configure << ("-DCMAKE_PREFIX_PATH=" + m_qtPrefix);
    const QString cfg = runAndCapture("cmake", configure);
    const QString out = runAndCapture("cmake", {"--build", build});
    qCDebug(lcNativeMixed).noquote() << "cmake configure:\n" << cfg << "\ncmake build:\n" << out;

    // The fixture target is qmlmixtest; find the produced executable.
    QDirIterator it(build, QDir::Files | QDir::Executable, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QFileInfo fi(it.next());
        if (fi.fileName() == "qmlmixtest" || fi.fileName() == "qmlmixtest.exe") {
            m_exe = fi.absoluteFilePath();
            return true;
        }
    }
    return false;
}

void tst_NativeMixed::nativeMixed()
{
    if (!m_fixtureOk)
        QSKIP("Native combined fixture could not be built (Qt with QML "
              "debugging and the qmldbg_native plugin is required).");

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("QTC_NM_EXE", m_exe);
    env.insert("QTC_NM_QMLDIR", m_fixtureSrc);
    env.insert("QTC_NM_ENGINE", m_engine == GdbEngine ? "gdb" : "lldb");

    QProcess dbg;
    dbg.setProcessEnvironment(env);
    dbg.setWorkingDirectory(m_buildDir.path());
    dbg.setProcessChannelMode(QProcess::MergedChannels);

    if (m_engine == GdbEngine) {
        const QStringList args = {
            "-nx", "-q", "-batch",
            "-ex", QString("python import sys; sys.path.insert(1, \"%1\")").arg(m_dumperDir),
            "-ex", "python from gdbbridge import *",
            "-ex", QString("python exec(open(\"%1\").read())").arg(m_driver)};
        dbg.start(m_debuggerBinary, args);
    } else {
        QFile commands(m_buildDir.path() + "/lldb-commands.txt");
        QVERIFY(commands.open(QIODevice::WriteOnly));
        commands.write(QString("script import sys; sys.path.insert(1, '%1')\n")
                           .arg(m_dumperDir).toUtf8());
        commands.write(QString("script exec(open('%1').read())\n").arg(m_driver).toUtf8());
        commands.close();
        dbg.start(m_debuggerBinary, {"-b", "-s", commands.fileName()});
    }

    QVERIFY(dbg.waitForStarted());
    QVERIFY2(dbg.waitForFinished(240000), "Debugger session timed out.");
    const QString output = QString::fromLocal8Bit(dbg.readAll());
    qCDebug(lcNativeMixed).noquote() << output;

    // On a session-level failure the captured output is the only clue, so
    // surface it regardless of the logging category.
    if (!output.contains("nmdone={"))
        qWarning().noquote() << "Debugger session output:\n" << output;
    QVERIFY2(output.contains("nmdone={"), "Driver did not finish the session.");

    const bool requireHook = m_qtVersion >= HookQtVersion;
    QRegularExpression re("nmcheck=\\{name=\"([^\"]*)\",hook=\"([01])\",ok=\"([01])\"\\}");
    auto matches = re.globalMatch(output);
    int hookChecks = 0;
    bool any = false;
    while (matches.hasNext()) {
        const QRegularExpressionMatch m = matches.next();
        any = true;
        const QString name = m.captured(1);
        const bool hook = m.captured(2) == "1";
        const bool ok = m.captured(3) == "1";
        if (hook) {
            ++hookChecks;
            if (!requireHook)
                continue; // Needs the Qt 6.12 hook; ignore on older Qt.
        }
        QVERIFY2(ok, qPrintable(QString("check failed: %1").arg(name)));
    }
    QVERIFY2(any, "Driver emitted no checks.");
    if (requireHook)
        QVERIFY2(hookChecks > 0, "Expected hook-dependent checks on Qt >= 6.12.");
}

void tst_NativeMixed::cleanupTestCase()
{
    if (qEnvironmentVariableIsSet("QTC_KEEP_TEMP"))
        m_buildDir.setAutoRemove(false);
}

QTEST_MAIN(tst_NativeMixed)

#include "tst_nativemixed.moc"
