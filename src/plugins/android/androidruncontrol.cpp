/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 BogDan Vatra <bog_dan_ro@yahoo.com>
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "androidruncontrol.h"

#include "androiddeploystep.h"
#include "androidglobal.h"
#include "androidrunconfiguration.h"
#include "androidrunner.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <QIcon>

namespace Android {
namespace Internal {

using ProjectExplorer::RunConfiguration;
using namespace ProjectExplorer;

AndroidRunControl::AndroidRunControl(AndroidRunConfiguration *rc)
    : RunControl(rc, ProjectExplorer::NormalRunMode)
    , m_runner(new AndroidRunner(this, rc, false))
    , m_running(false)
{
}

AndroidRunControl::~AndroidRunControl()
{
    stop();
}

void AndroidRunControl::start()
{
    m_running = true;
    emit started();
    disconnect(m_runner, 0, this, 0);

    connect(m_runner, SIGNAL(remoteErrorOutput(QByteArray)),
        SLOT(handleRemoteErrorOutput(QByteArray)));
    connect(m_runner, SIGNAL(remoteOutput(QByteArray)),
        SLOT(handleRemoteOutput(QByteArray)));
    connect(m_runner, SIGNAL(remoteProcessFinished(const QString &)),
        SLOT(handleRemoteProcessFinished(const QString &)));
    appendMessage(tr("Starting remote process ..."), Utils::NormalMessageFormat);
    m_runner->start();
}

ProjectExplorer::RunControl::StopResult AndroidRunControl::stop()
{
    m_runner->stop();
    return StoppedSynchronously;
}

void AndroidRunControl::handleRemoteProcessFinished(const QString &error)
{
    appendMessage(error, Utils::ErrorMessageFormat);
    disconnect(m_runner, 0, this, 0);
    m_running = false;
    emit finished();
}

void AndroidRunControl::handleRemoteOutput(const QByteArray &output)
{
    appendMessage(QString::fromUtf8(output), Utils::StdOutFormatSameLine);
}

void AndroidRunControl::handleRemoteErrorOutput(const QByteArray &output)
{
    appendMessage(QString::fromUtf8(output), Utils::StdErrFormatSameLine);
}

bool AndroidRunControl::isRunning() const
{
    return m_running;
}

QString AndroidRunControl::displayName() const
{
    return m_runner->displayName();
}

QIcon AndroidRunControl::icon() const
{
    return QIcon(QLatin1String(ProjectExplorer::Constants::ICON_DEBUG_SMALL));
}

} // namespace Internal
} // namespace Qt4ProjectManager
