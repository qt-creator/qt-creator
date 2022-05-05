/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "queryrunner.h"

#include "gitlabparameters.h"
#include "gitlabplugin.h"

#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <utils/algorithm.h>
#include <utils/commandline.h>
#include <utils/qtcassert.h>
#include <vcsbase/vcsoutputwindow.h>

#include <QUrl>

namespace GitLab {

const char API_PREFIX[]                 = "/api/v4";
const char QUERY_PROJECT[]              = "/projects/%1";

Query::Query(Type type, const QStringList &parameter)
    : m_type(type)
    , m_parameter(parameter)
{
}

QString Query::toString() const
{
    QString query = API_PREFIX;
    switch (m_type) {
    case Query::NoQuery:
        return QString();
    case Query::Project:
        QTC_ASSERT(!m_parameter.isEmpty(), return {});
        query += QLatin1String(QUERY_PROJECT).arg(QLatin1String(
                                                      QUrl::toPercentEncoding(m_parameter.at(0))));
        break;
    }
    return query;
}

QueryRunner::QueryRunner(const Query &query, const Utils::Id &id, QObject *parent)
    : QObject(parent)
{
    const GitLabParameters *p = GitLabPlugin::globalParameters();
    const auto server = p->serverForId(id);
    QStringList args = server.curlArguments();
    if (!server.token.isEmpty())
        args << "--header" << "PRIVATE-TOKEN: " + server.token;
    QString url = "https://" + server.host;
    if (server.port != GitLabServer::defaultPort)
        url.append(':' + QString::number(server.port));
    url += query.toString();
    args << url;
    m_process.setCommand({p->curl, args});
    connect(&m_process, &Utils::QtcProcess::finished, this, &QueryRunner::processFinished);
    connect(&m_process, &Utils::QtcProcess::errorOccurred, this, &QueryRunner::processError);
}

QueryRunner::~QueryRunner()
{
    m_process.disconnect();
    terminate();
}

void QueryRunner::start()
{
    QTC_ASSERT(!m_running, return);
    m_running = true;
    m_process.start();
}

void QueryRunner::terminate()
{
    m_process.stopProcess();
}

void QueryRunner::errorTermination(const QString &msg)
{
    if (!m_running)
        return;
    VcsBase::VcsOutputWindow::appendError(msg);
    m_running = false;
    emit finished();
}

void QueryRunner::processError(QProcess::ProcessError /*error*/)
{
    const QString msg = tr("Error running %1: %2")
            .arg(m_process.commandLine().executable().toUserOutput(),
                 m_process.errorString());
    errorTermination(msg);
}
void QueryRunner::processFinished()
{
    const QString executable = m_process.commandLine().executable().toUserOutput();
    if (m_process.exitStatus() != QProcess::NormalExit) {
        errorTermination(tr("%1 crashed.").arg(executable));
        return;
    } else if (m_process.exitCode()) {
        errorTermination(tr("%1 returned %2.").arg(executable).arg(m_process.exitCode()));
        return;
    }
    m_running = false;
    emit resultRetrieved(m_process.readAllStandardOutput());
    emit finished();
}

} // namespace GitLab
