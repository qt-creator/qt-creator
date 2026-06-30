// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gitlabquery.h"

#include "gitlabparameters.h"
#include "gitlabplugin.h"
#include "gitlabtr.h"

#include <coreplugin/icore.h>
#include <utils/commandline.h>
#include <utils/dialogtask.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <vcsbase/vcsoutputwindow.h>

#include <QtTaskTree/qconditional.h>
#include <QtTaskTree/qtasktree.h>

#include <QMessageBox>
#include <QUrl>

using namespace QtTaskTree;
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

static CommandLine gitLabCommand(const Query &query, const GitLabServer &server)
{
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
    return {gitLabParameters().curl, args};
}

Group gitLabQuery(const QuerySetupHandler &onSetup, const QueryDoneHandler &onDone)
{
    const Storage<GitLabQuery> storage;
    const Storage<bool> needsPrompt; // first attempt hit a (recoverable) certificate error

    const auto onProcessSetup = [storage](Process &process) {
        process.setCommand(gitLabCommand(storage->m_query,
                                         gitLabParameters().serverForId(storage->m_id)));
    };

    const auto onFirstDone = [storage, needsPrompt](const Process &process, DoneWith result) {
        if (result == DoneWith::Success) {
            storage->m_result = process.rawStdOut();
            return DoneResult::Success;
        }
        const GitLabServer server = gitLabParameters().serverForId(storage->m_id);
        // Only offer to ignore the certificate while it is still being validated, so a
        // persistent SSL error can't keep prompting and retrying.
        if (server.secure && server.validateCert
                && process.exitStatus() == QProcess::NormalExit
                && (process.exitCode() == 35 || process.exitCode() == 60)) { // ssl certificate issues
            *needsPrompt = true;
            return DoneResult::Success; // proceed to the prompt + retry branch
        }
        VcsBase::VcsOutputWindow::appendError(process.workingDirectory(), process.exitMessage());
        return DoneResult::Error;
    };

    const auto onPromptSetup = [storage](DialogWrapper<QMessageBox> &task) {
        const GitLabServer server = gitLabParameters().serverForId(storage->m_id);
        task.setParent(Core::ICore::dialogParent());
        QMessageBox *box = task.dialog();
        box->setIcon(QMessageBox::Question);
        box->setWindowTitle(Tr::tr("Certificate Error"));
        box->setText(Tr::tr("Server certificate for %1 cannot be authenticated.\n"
                            "Do you want to disable SSL verification for this server?\n"
                            "Note: This can expose you to man-in-the-middle attack.").arg(server.host));
        box->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        box->setDefaultButton(QMessageBox::No);
    };
    // Runs only on "Yes" (CallDoneFlag::OnSuccess): disabling certificate validation makes
    // the retry below pass -k. "No" stops the branch, and therefore the whole query.
    const auto onPromptDone = [storage] { acceptCertificate(storage->m_id); };

    const auto onRetryDone = [storage](const Process &process, DoneWith result) {
        if (result != DoneWith::Success) {
            VcsBase::VcsOutputWindow::appendError(process.workingDirectory(), process.exitMessage());
            return DoneResult::Error;
        }
        storage->m_result = process.rawStdOut();
        return DoneResult::Success;
    };

    return {
        storage,
        needsPrompt,
        onGroupSetup([storage, onSetup] { onSetup(*storage); }),
        ProcessTask(onProcessSetup, onFirstDone),
        If ([needsPrompt] { return *needsPrompt; }) >> Then {
            DialogTask<QMessageBox>(onPromptSetup, onPromptDone, CallDoneFlag::OnSuccess),
            ProcessTask(onProcessSetup, onRetryDone),
        },
        onGroupDone([storage, onDone] { onDone(*storage); }, CallDoneFlag::OnSuccess),
    };
}

} // namespace GitLab
