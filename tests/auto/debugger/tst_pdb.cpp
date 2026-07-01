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

void tst_Pdb::dumper()
{
    QString python = QStandardPaths::findExecutable("python3");
    if (python.isEmpty())
        python = QStandardPaths::findExecutable("python");
    if (python.isEmpty())
        QSKIP("No Python interpreter found in PATH.");

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
