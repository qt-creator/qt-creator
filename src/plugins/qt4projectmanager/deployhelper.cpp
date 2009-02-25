/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "deployhelper.h"
#include "qt4project.h"
#include "qt4projectmanagerconstants.h"

#include <QDebug>
#include <QDir>
#include <QEventLoop>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

DeployHelperRunStep::DeployHelperRunStep(Qt4Project *pro)
    : BuildStep(pro),  m_started(false), m_pro(pro)
{
    QDir workbenchDir = QCoreApplication::applicationDirPath();
    workbenchDir.cdUp();
    m_binary = QDir::convertSeparators(workbenchDir.absolutePath() + QLatin1String("/qtembeddedtools/qemudeployer"));
    m_id = "id";

};

bool DeployHelperRunStep::init(const QString &configuration)
{
    Q_UNUSED(configuration);
    m_qtdir = m_pro->qtDir(configuration);
    QFileInfo fi(m_pro->file()->fileName());
    m_appdir = fi.absolutePath();
    //find target
    m_exec = "";
    QStringList targets = QStringList(); //TODO fix m_pro->qmakeTarget();
    foreach (const QString &target, targets) {
        QFileInfo fi(m_appdir + QLatin1Char('/') + target);
        if (fi.exists())
            m_exec = target;
        break;
    }
    m_skin = m_pro->value("VNCSkin").toString();
    return true;
}

void DeployHelperRunStep::run(QFutureInterface<bool> & fi)
{
    if (m_id.isNull() || m_binary.isNull()) {
        fi.reportResult(false);
        return;
    }
    if (m_started)
        stop();

    QStringList args;
    args << "start" << "-id"<<m_id<<"-qtdir"<<m_qtdir<<"-appdir"<<m_appdir<<"-exec"<<m_exec;
    if (!m_skin.isEmpty())
        args<<"-skin"<<m_skin;

    for (int i=0; i<m_extraargs.count(); ++i)
        args.append(m_extraargs.at(i));

    QProcess proc;
    connect(&proc, SIGNAL(finished (int,QProcess::ExitStatus)),
            this, SLOT(processFinished()), Qt::DirectConnection);
    connect(&proc, SIGNAL(readyRead()), this, SLOT(readyRead()), Qt::DirectConnection);

    QStringList env = QProcess::systemEnvironment();
    env.replaceInStrings(QRegExp("^PATH=(.*)", Qt::CaseInsensitive), "PATH="+QApplication::applicationDirPath()+";\\1");
    proc.setEnvironment(env);
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(m_binary, args);
    proc.waitForStarted();
    m_started = true;

    m_eventLoop = new QEventLoop;
    m_eventLoop->exec();
    delete m_eventLoop;
    m_eventLoop = 0;
    fi.reportResult(true);
    return;
}

DeployHelperRunStep::~DeployHelperRunStep()
{
    cleanup();
}

QString DeployHelperRunStep::binary()
{
    return m_binary;
}

QString DeployHelperRunStep::id()
{
    return m_id;
}

bool DeployHelperRunStep::started()
{
    return m_started;
}

void DeployHelperRunStep::processFinished()
{
    m_eventLoop->exit(0);
}

void DeployHelperRunStep::stop()
{
    if (m_id.isNull() || m_binary.isNull() || !m_started)
        return;

    QStringList env = QProcess::systemEnvironment();
    env.replaceInStrings(QRegExp("^PATH=(.*)", Qt::CaseInsensitive), "PATH="+QApplication::applicationDirPath()+";\\1");

    QStringList args;
    args<<"stop"<<"-id"<<m_id;
    QProcess proc;
    proc.setEnvironment(env);
    proc.start(m_binary, args);
    proc.waitForFinished();
    m_started = false;
}

void DeployHelperRunStep::cleanup()
{
    if (m_id.isNull() || m_binary.isNull() || !m_started)
        return;

    QStringList env = QProcess::systemEnvironment();
    env.replaceInStrings(QRegExp("^PATH=(.*)", Qt::CaseInsensitive), "PATH="+QApplication::applicationDirPath()+";\\1");

    QStringList args;
    args<<"cleanup"<<"-id"<<m_id;
    QProcess proc;
    proc.setEnvironment(env);
    proc.start(m_binary, args);
    proc.waitForFinished();
    m_started = false;
}

void DeployHelperRunStep::readyRead()
{
    // TODO Unbreak the application output (this whole thing should be moved to a IRunConfigurationRunner)
    QProcess * proc = qobject_cast<QProcess *>(sender());
    while (proc->canReadLine()) {
        QString line = proc->readLine().trimmed();
        if (line.startsWith("L:") || line.startsWith("A:")) {
            //emit addToApplicationOutputWindow(line.mid(2));
        } else {
            //emit addToApplicationOutputWindow(line);
        }
    }
}

QString DeployHelperRunStep::name()
{
    return Constants::DEPLOYHELPERRUNSTEP;
}

QString DeployHelperRunStep::displayName()
{
    return "Linux emulator";
}

ProjectExplorer::BuildStepConfigWidget * DeployHelperRunStep::configWidget()
{
    return 0;
}
