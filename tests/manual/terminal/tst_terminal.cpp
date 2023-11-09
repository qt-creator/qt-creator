// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/externalterminalprocessimpl.h>

#include <QRandomGenerator>
#include <QtTest>

//TESTED_COMPONENT=src/utils/changeset

class tst_Terminal : public QObject
{
    Q_OBJECT

private slots:
    void terminalApp()
    {
        if (!Utils::HostOsInfo::isMacHost())
            QSKIP("This test is only for macOS");

        int rnd = QRandomGenerator::global()->generate();
        QString testCode = R"(
set theFile to POSIX file "/tmp/testoutput.txt"
set theFile to theFile as string
set theOpenedFile to open for access file theFile with write permission
set eof of theOpenedFile to 0
write "%1" to theOpenedFile starting at eof
close access theOpenedFile
        )";
        QString terminalScript = Utils::ExternalTerminalProcessImpl::openTerminalScriptAttached();
        terminalScript = terminalScript.arg("sleep 1") + "\n" + testCode.arg(rnd);

        QProcess process;
        process.setProcessChannelMode(QProcess::ProcessChannelMode::MergedChannels);
        process.setProgram("osascript");
        process.setArguments({"-e", terminalScript});

        process.start();

        QTRY_VERIFY(process.state() == QProcess::NotRunning);
        const auto output = process.readAll();
        if (!output.isEmpty())
            qDebug() << "Output:" << output;
        QVERIFY(process.exitCode() == 0);

        QFile testOutputFile("/tmp/testoutput.txt");
        QVERIFY(testOutputFile.open(QIODevice::ReadOnly));
        QVERIFY(testOutputFile.readAll() == QByteArray::number(rnd));
    }
};

QTEST_GUILESS_MAIN(tst_Terminal)

#include "tst_terminal.moc"
