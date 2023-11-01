// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

using namespace Utils;

namespace GitLab {

const char API_PREFIX[]                 = "/api/v4";
const char QUERY_PROJECT[]              = "/projects/%1";
const char QUERY_PROJECTS[]             = "/projects?simple=true";
const char QUERY_USER[]                 = "/user";
const char QUERY_EVENTS[]               = "/projects/%1/events";

Query::Query(Type type, const QStringList &parameter)
    : m_type(type)
    , m_parameter(parameter)
{
}

void Query::setPageParameter(int page)
{
    m_pageParameter = page;
}

void Query::setAdditionalParameters(const QStringList &additional)
{
    m_additionalParameters = additional;
}

bool Query::hasPaginatedResults() const
{
    return m_type == Query::Projects || m_type == Query::Events;
}

QString Query::toString() const
{
    QString query = API_PREFIX;
    switch (m_type) {
    case Query::NoQuery:
        return {};
    case Query::Project:
        QTC_ASSERT(!m_parameter.isEmpty(), return {});
        query += QLatin1String(QUERY_PROJECT).arg(QLatin1String(
                                                      QUrl::toPercentEncoding(m_parameter.at(0))));
        break;
    case Query::Projects:
        query += QLatin1String(QUERY_PROJECTS);
        break;
    case Query::User:
        query += QUERY_USER;
        break;
    case Query::Events:
        QTC_ASSERT(!m_parameter.isEmpty(), return {});
        query += QLatin1String(QUERY_EVENTS).arg(QLatin1String(
                                                     QUrl::toPercentEncoding(m_parameter.at(0))));
        break;
    }
    if (m_pageParameter > 0) {
        query.append(m_type == Query::Projects ? '&' : '?');
        query.append("page=").append(QString::number(m_pageParameter));
    }
    if (!m_additionalParameters.isEmpty()) {
        query.append((m_type == Query::Projects || m_pageParameter > 0) ? '&' : '?');
        query.append(m_additionalParameters.join('&'));
    }
    return query;
}

QueryRunner::QueryRunner(const Query &query, const Id &id, QObject *parent)
    : QObject(parent)
{
    const GitLabParameters *p = GitLabPlugin::globalParameters();
    const auto server = p->serverForId(id);
    QStringList args = server.curlArguments();
    if (query.hasPaginatedResults())
        args << "-i";
    if (!server.token.isEmpty())
        args << "--header" << "PRIVATE-TOKEN: " + server.token;
    QString url = (server.secure ? "https://" : "http://") + server.host;
    if (server.port && (server.port != (server.secure ? GitLabServer::defaultPort : 80)))
        url.append(':' + QString::number(server.port));
    url += query.toString();
    args << url;
    m_process.setCommand({p->curl, args});
    connect(&m_process, &Process::done, this, [this, id] {
        if (m_process.result() != ProcessResult::FinishedWithSuccess) {
            const int exitCode = m_process.exitCode();
            if (m_process.exitStatus() == QProcess::NormalExit
                    && (exitCode == 35 || exitCode == 60) // common ssl certificate issues
                    && GitLabPlugin::handleCertificateIssue(id)) {
                // prepend -k for re-requesting the same query
                CommandLine cmdline = m_process.commandLine();
                cmdline.prependArgs({"-k"});
                m_process.setCommand(cmdline);
                start();
                return;
            }
            VcsBase::VcsOutputWindow::appendError(m_process.exitMessage());
        } else {
            emit resultRetrieved(m_process.readAllRawStandardOutput());
        }
        emit finished();
    });
}

void QueryRunner::start()
{
    QTC_ASSERT(!m_process.isRunning(), return);
    m_process.start();
}

} // namespace GitLab
