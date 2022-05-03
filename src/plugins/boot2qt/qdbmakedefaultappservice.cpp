/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qdbmakedefaultappservice.h"

#include "qdbconstants.h"
#include "qdbrunconfiguration.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/target.h>
#include <utils/qtcprocess.h>

using namespace Utils;

namespace Qdb {
namespace Internal {

class QdbMakeDefaultAppServicePrivate
{
public:
    bool m_makeDefault = true;
    QtcProcess m_process;
};

QdbMakeDefaultAppService::QdbMakeDefaultAppService(QObject *parent)
    : AbstractRemoteLinuxDeployService(parent),
      d(new QdbMakeDefaultAppServicePrivate)
{
    connect(&d->m_process, &QtcProcess::done, this, [this] {
        if (d->m_process.error() != QProcess::UnknownError)
            emit errorMessage(tr("Remote process failed: %1").arg(d->m_process.errorString()));
        else if (d->m_makeDefault)
            emit progressMessage(tr("Application set as the default one."));
        else
            emit progressMessage(tr("Reset the default application."));

        stopDeployment();
    });
    connect(&d->m_process, &QtcProcess::readyReadStandardError, this, [this] {
        emit stdErrData(QString::fromUtf8(d->m_process.readAllStandardError()));
    });
}

QdbMakeDefaultAppService::~QdbMakeDefaultAppService() = default;

void QdbMakeDefaultAppService::setMakeDefault(bool makeDefault)
{
    d->m_makeDefault = makeDefault;
}

void QdbMakeDefaultAppService::doDeploy()
{
    QString remoteExe;

    if (ProjectExplorer::RunConfiguration *rc = target()->activeRunConfiguration()) {
        if (auto exeAspect = rc->aspect<ProjectExplorer::ExecutableAspect>())
            remoteExe = exeAspect->executable().toString();
    }

    const QString args = d->m_makeDefault && !remoteExe.isEmpty()
            ? QStringLiteral("--make-default ") + remoteExe
            : QStringLiteral("--remove-default");
    d->m_process.setCommand(
            {deviceConfiguration()->mapToGlobalPath(Constants::AppcontrollerFilepath), {args}});
    d->m_process.start();
}

void QdbMakeDefaultAppService::stopDeployment()
{
    d->m_process.close();
    handleDeploymentDone();
}

} // namespace Internal
} // namespace Qdb
