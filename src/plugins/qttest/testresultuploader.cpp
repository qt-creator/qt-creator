/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "testresultuploader.h"
#include "testoutputwindow.h"
#include "qsystem.h"
#include "testconfigurations.h"

#include <QFileInfo>
#include <QTextEdit>

TestResultUploader::TestResultUploader(QObject *parent) :
    QObject(parent)
{
    m_executer.setReadChannelMode(QProcess::MergedChannels);
    m_executer.setReadChannel(QProcess::StandardOutput);

    connect(&m_executer, SIGNAL(finished(int,QProcess::ExitStatus)),
        this, SLOT(execFinished(int,QProcess::ExitStatus)));
    connect(&m_executer, SIGNAL(readyReadStandardOutput()),
        this, SLOT(readStdOut()));

    m_uploadState = UploadIdle;
}

TestResultUploader::~TestResultUploader()
{
    disconnect(&m_executer, SIGNAL(finished(int,QProcess::ExitStatus)),
        this, SLOT(execFinished(int,QProcess::ExitStatus)));
    disconnect(&m_executer, SIGNAL(readyReadStandardOutput()),
        this, SLOT(readStdOut()));
}

void TestResultUploader::uploadResultsToDatabase(const QString &fname, TestConfig *cfg)
{
    if (m_uploadState != UploadIdle)
        return;

    m_testCfg = cfg;
    m_srcFname = fname;

    if (!fname.isEmpty()) {
        m_logFile.setFileName(QDir::homePath() + QDir::separator() + QLatin1String(".qttest")
            + QDir::separator() + QLatin1String("test_result_upload.log"));
        m_logFile.open(QFile::WriteOnly);

        if (m_testSettings.uploadServer().isEmpty()) {
            testOutputPane()->append(tr("-- ATTENTION: Uploading of test results to the database aborted. "
                "Please specify an upload server in the Upload Test Results Settings dialog."));
            return;
        }

        m_uploadServerName = m_testSettings.uploadServer();

        int pos = m_uploadServerName.indexOf(QLatin1Char(':'));
        if (pos)
            m_uploadServerName = m_uploadServerName.left(pos);

        QFileInfo inf(fname);
        if (!inf.exists())
            return;
        m_tgtFname = inf.fileName();

        QString cmd1 = QSystem::which(m_testCfg->PATH(), QLatin1String("scp"));
        QStringList args1;
        args1 << fname << QString::fromLatin1("%1:/tmp/%2").arg(m_uploadServerName, m_tgtFname);
        if (exec(cmd1, QStringList() << QLatin1String("-q") << QLatin1String("-v") << QLatin1String("-C") << args1, QString(), 60000))
            m_uploadState = UploadScp;
    }
}

bool TestResultUploader::exec(const QString &cmd, const QStringList &arguments, const QString &workDir, int timeout)
{
    Q_UNUSED(timeout);
    if (!m_testCfg)
        return false;

    // Kill the process if it's still running
    if (m_executer.state() != QProcess::NotRunning) {
        m_executer.kill();
        m_executer.waitForFinished();
    }

    Q_ASSERT(m_executer.state() == QProcess::NotRunning);

    if ((!workDir.isEmpty()) && (m_executer.workingDirectory() != workDir))
        m_executer.setWorkingDirectory(workDir);

    m_executer.setEnvironment(*m_testCfg->buildEnvironment());

    if (arguments.count() > 0) {
        QString realCmd;
        if (!cmd.contains(QDir::separator()))
            realCmd = QSystem::which(m_testCfg->PATH(), cmd);
        else if (QFile::exists(cmd))
            realCmd = cmd;

        if (realCmd.isEmpty()) {
            const QString path = QSystem::envKey(m_testCfg->buildEnvironment(), "PATH");
            testOutputPane()->append(tr("-- No '%1' instance found in PATH (%2).").arg(cmd, path));
            return false;
        }
        m_executer.start(realCmd,arguments);

    } else {
        m_executer.start(cmd);
    }

    if (m_logFile.isOpen()) {
        m_logFile.write("exec: ");
        m_logFile.write(cmd.toLatin1());
        m_logFile.write(arguments.join(QString(QLatin1Char(' '))).toLatin1());
        m_logFile.write("\n");
    }
    return m_executer.waitForStarted(30000);
}

void TestResultUploader::execFinished(int exitValue, QProcess::ExitStatus)
{
    if (exitValue != 0) {
        m_uploadState = UploadIdle;
        if (m_logFile.isOpen()) {
            m_logFile.write("Upload FAILED!");
            m_logFile.close();

            QString errFname = QDir::homePath() + QDir::separator() + QLatin1String(".qttest")
                + QDir::separator() + QLatin1String("last_test_result_upload_failure.log");
            QFile::remove(errFname);
            m_logFile.rename(m_logFile.fileName(), errFname);
            QFile errorFile(errFname);
            QString lastErrorMessage;
            if (errorFile.open(QIODevice::ReadOnly)){
                lastErrorMessage = tr("Upload FAILED: %1").arg(QString::fromLocal8Bit(errorFile.readAll()));
                errorFile.close();
            } else {
                lastErrorMessage = tr("Upload FAILED, look at the log file %1").arg(errFname);
            }
            testOutputPane()->append(lastErrorMessage);
        }

        return;
    }

    if (m_uploadState == UploadScp) {
        m_uploadState = UploadMv;

        QString cmd2 = QSystem::which(m_testCfg->PATH(), "ssh");
        QStringList args2;
        args2 << m_uploadServerName
            << QLatin1String("mv")
            << QLatin1String("-f")
            << QLatin1String("/tmp/") + m_tgtFname
            << QLatin1String("results/") + m_tgtFname;
        exec(cmd2, QStringList() << QLatin1String("-q") << QLatin1String("-v") << args2);
    } else if (m_uploadState == UploadMv) {
        m_uploadState = UploadIdle;
        testOutputPane()->append(tr("\nTest results have been uploaded into the results database.\nThank you for supporting %1").arg(m_testCfg->configName()));
        QFile::remove(m_srcFname);
        m_logFile.close();
        m_logFile.remove();
    }
}

void TestResultUploader::readStdOut()
{
    if (m_logFile.isOpen())
        m_logFile.write(m_executer.readAllStandardOutput());
}
