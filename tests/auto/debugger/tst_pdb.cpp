// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Runs the standalone checks for the pdb value dumper (pdbdumper.py, which
// exercises pdbbridge.py) through a Python interpreter and reports failures.
// The PySide6-specific checks inside the driver are skipped automatically when
// PySide6 is not installed.

#include <QProcess>
#include <QStandardPaths>
#include <QTest>

class tst_Pdb : public QObject
{
    Q_OBJECT

private slots:
    void dumper();
};

// Returns true if the given path is a working Python interpreter. On Windows,
// QStandardPaths::findExecutable() may return the Microsoft Store "App
// execution alias" stub (WindowsApps\python.exe), which is not a real
// interpreter: running it merely prints a message pointing at the Store
// ("Python was not found; ...") instead of a version. Probe the candidate and
// check that it actually reports a Python version before relying on it.
static bool isWorkingPython(const QString &python)
{
    QProcess probe;
    probe.setProcessChannelMode(QProcess::MergedChannels);
    probe.start(python, {"--version"});
    if (!probe.waitForStarted() || !probe.waitForFinished(10000))
        return false;
    if (probe.exitStatus() != QProcess::NormalExit || probe.exitCode() != 0)
        return false;
    // A real interpreter reports "Python <version>", e.g. "Python 3.12.1".
    // The Store stub instead prints "Python was not found; ...", so require a
    // digit right after the "Python " prefix to tell them apart.
    const QString version = QString::fromLocal8Bit(probe.readAll()).trimmed();
    return version.startsWith("Python ") && version.length() > 7
           && version.at(7).isDigit();
}

// Returns the first working Python interpreter, or an empty string if none is
// found. On Windows CI the PYTHON3_PATH environment variable usually points at
// the real Python installation (e.g. "C:\Python38_64"), so prefer it over the
// bare PATH lookup, which may resolve to the Microsoft Store stub.
static QString findWorkingPython()
{
    QStringList candidates;
    const QString pythonRoot = qEnvironmentVariable("PYTHON3_PATH");
    if (!pythonRoot.isEmpty()) {
        candidates << QStandardPaths::findExecutable("python3", {pythonRoot})
                   << QStandardPaths::findExecutable("python", {pythonRoot});
    }
    candidates << QStandardPaths::findExecutable("python3")
               << QStandardPaths::findExecutable("python");

    for (const QString &candidate : std::as_const(candidates)) {
        if (!candidate.isEmpty() && isWorkingPython(candidate))
            return candidate;
    }
    return {};
}

void tst_Pdb::dumper()
{
    const QString python = findWorkingPython();
    if (python.isEmpty())
        QSKIP("No working Python interpreter found in PYTHON3_PATH or PATH.");

    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("DUMPERDIR", QString::fromLocal8Bit(DUMPERDIR));
    process.setProcessEnvironment(env);

    process.start(python, {QString::fromLocal8Bit(PDBDUMPER_DRIVER)});
    QVERIFY2(process.waitForStarted(), qPrintable(process.errorString()));
    QVERIFY2(process.waitForFinished(60000), "The pdb dumper driver timed out.");

    const QByteArray output = process.readAll();
    QVERIFY2(process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0,
             output.constData());
}

QTEST_GUILESS_MAIN(tst_Pdb)

#include "tst_pdb.moc"
