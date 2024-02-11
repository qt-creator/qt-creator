// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "axivionplugin.h"

#include "axivionoutputpane.h"
#include "axivionprojectsettings.h"
#include "axivionsettings.h"
#include "axiviontr.h"
#include "credentialquery.h"
#include "dashboard/dto.h"
#include "dashboard/error.h"

#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <solutions/tasking/networkquery.h>
#include <solutions/tasking/tasktreerunner.h>

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/textmark.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/environment.h>
#include <utils/networkaccessmanager.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QInputDialog>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>

#include <memory>

constexpr char s_axivionTextMarkId[] = "AxivionTextMark";
constexpr char s_axivionKeychainService[] = "keychain.axivion.qtcreator";

using namespace Core;
using namespace ProjectExplorer;
using namespace Tasking;
using namespace TextEditor;
using namespace Utils;

namespace Axivion::Internal {

QIcon iconForIssue(const QString &prefix)
{
    static QHash<QString, QIcon> prefixToIcon;
    auto it = prefixToIcon.find(prefix);

    if (it == prefixToIcon.end()) {
        Icon icon({{FilePath::fromString(":/axivion/images/button-" + prefix.toLower() + ".png"),
                    Theme::PaletteButtonText}},
                  Icon::Tint);
        it = prefixToIcon.insert(prefix, icon.icon());
    }
    return it.value();
}

QString anyToSimpleString(const Dto::Any &any)
{
    if (any.isString())
        return any.getString();
    if (any.isBool())
        return QString("%1").arg(any.getBool());
    if (any.isDouble())
        return QString::number(any.getDouble());
    if (any.isNull())
        return QString(); // or NULL??
    if (any.isList()) {
        const std::vector<Dto::Any> anyList = any.getList();
        QStringList list;
        for (const Dto::Any &inner : anyList)
            list << anyToSimpleString(inner);
        return list.join(',');
    }
    if (any.isMap()) { // TODO
        const std::map<QString, Dto::Any> anyMap = any.getMap();
        auto value = anyMap.find("displayName");
        if (value != anyMap.end())
            return anyToSimpleString(value->second);
        value = anyMap.find("name");
        if (value != anyMap.end())
            return anyToSimpleString(value->second);
    }
    return {};
}

static QString apiTokenDescription()
{
    const QString ua = "Axivion" + QCoreApplication::applicationName() + "Plugin/"
                       + QCoreApplication::applicationVersion();
    QString user = Utils::qtcEnvironmentVariable("USERNAME");
    if (user.isEmpty())
        user = Utils::qtcEnvironmentVariable("USER");
    return "Automatically created by " + ua + " on " + user + "@" + QSysInfo::machineHostName();
}

static QString credentialKey()
{
    const auto escape = [](const QString &string) {
        QString escaped = string;
        return escaped.replace('\\', "\\\\").replace('@', "\\@");
    };
    return escape(settings().server.dashboard) + '@' + escape(settings().server.username);
}

static DashboardInfo toDashboardInfo(const QUrl &source, const Dto::DashboardInfoDto &infoDto)
{
    const QVersionNumber versionNumber = infoDto.dashboardVersionNumber
        ? QVersionNumber::fromString(*infoDto.dashboardVersionNumber) : QVersionNumber();

    QStringList projects;
    QHash<QString, QUrl> projectUrls;

    if (infoDto.projects) {
        for (const Dto::ProjectReferenceDto &project : *infoDto.projects) {
            projects.push_back(project.name);
            projectUrls.insert(project.name, project.url);
        }
    }
    return {source, versionNumber, projects, projectUrls, infoDto.checkCredentialsUrl};
}

QString IssueListSearch::toQuery() const
{
    if (kind.isEmpty())
        return {};
    QString result;
    result.append(QString("?kind=%1&offset=%2").arg(kind).arg(offset));
    if (limit)
        result.append(QString("&limit=%1").arg(limit));
    // TODO other params
    if (!versionStart.isEmpty()) {
        result.append(QString("&start=%1").arg(
            QString::fromUtf8(QUrl::toPercentEncoding(versionStart))));
    }
    if (!versionEnd.isEmpty()) {
        result.append(QString("&end=%1").arg(
            QString::fromUtf8(QUrl::toPercentEncoding(versionEnd))));
    }
    if (!owner.isEmpty()) {
        result.append(QString("&user=%1").arg(
            QString::fromUtf8((QUrl::toPercentEncoding(owner)))));
    }
    if (!filter_path.isEmpty()) {
        result.append(QString("&filter_path=%1").arg(
            QString::fromUtf8(QUrl::toPercentEncoding(filter_path))));
    }
    if (!state.isEmpty())
        result.append(QString("&state=%1").arg(state));
    if (computeTotalRowCount)
        result.append("&computeTotalRowCount=true");
    return result;
}

class AxivionPluginPrivate : public QObject
{
public:
    AxivionPluginPrivate();
    void handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);
    void onStartupProjectChanged(Project *project);
    void fetchProjectInfo(const QString &projectName);
    void handleOpenedDocs();
    void onDocumentOpened(IDocument *doc);
    void onDocumentClosed(IDocument * doc);
    void clearAllMarks();
    void handleIssuesForFile(const Dto::FileViewDto &fileView);
    void fetchIssueInfo(const QString &id);

    std::optional<QByteArray> m_apiToken; // TODO: Should be cleared on settings modification
    NetworkAccessManager m_networkAccessManager;
    AxivionOutputPane m_axivionOutputPane;
    std::optional<DashboardInfo> m_dashboardInfo;
    std::optional<Dto::ProjectInfoDto> m_currentProjectInfo;
    Project *m_project = nullptr;
    bool m_runningQuery = false;
    TaskTreeRunner m_taskTreeRunner;
    std::unordered_map<IDocument *, std::unique_ptr<TaskTree>> m_docMarksTrees;
    TaskTreeRunner m_issueInfoRunner;
};

static AxivionPluginPrivate *dd = nullptr;

class AxivionTextMark : public TextMark
{
public:
    AxivionTextMark(const FilePath &filePath, const Dto::LineMarkerDto &issue)
        : TextMark(filePath, issue.startLine, {Tr::tr("Axivion"), s_axivionTextMarkId})
    {
        const QString markText = issue.description;
        const QString id = issue.kind + QString::number(issue.id.value_or(-1));
        setToolTip(id + markText);
        setIcon(iconForIssue(issue.kind));
        setPriority(TextMark::NormalPriority);
        setLineAnnotation(markText);
        setActionsProvider([id] {
            auto action = new QAction;
            action->setIcon(Icons::INFO.icon());
            action->setToolTip(Tr::tr("Show rule details"));
            QObject::connect(action, &QAction::triggered, dd, [id] { dd->fetchIssueInfo(id); });
            return QList{action};
        });
    }
};

void fetchProjectInfo(const QString &projectName)
{
    QTC_ASSERT(dd, return);
    dd->fetchProjectInfo(projectName);
}

std::optional<Dto::ProjectInfoDto> projectInfo()
{
    QTC_ASSERT(dd, return {});
    return dd->m_currentProjectInfo;
}

// FIXME: extend to give some details?
// FIXME: move when curl is no more in use?
bool handleCertificateIssue()
{
    QTC_ASSERT(dd, return false);
    const QString serverHost = QUrl(settings().server.dashboard).host();
    if (QMessageBox::question(ICore::dialogParent(), Tr::tr("Certificate Error"),
                              Tr::tr("Server certificate for %1 cannot be authenticated.\n"
                                     "Do you want to disable SSL verification for this server?\n"
                                     "Note: This can expose you to man-in-the-middle attack.")
                              .arg(serverHost))
            != QMessageBox::Yes) {
        return false;
    }
    settings().server.validateCert = false;
    settings().apply();

    return true;
}

AxivionPluginPrivate::AxivionPluginPrivate()
{
#if QT_CONFIG(ssl)
    connect(&m_networkAccessManager, &QNetworkAccessManager::sslErrors,
            this, &AxivionPluginPrivate::handleSslErrors);
#endif // ssl
}

void AxivionPluginPrivate::handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
#if QT_CONFIG(ssl)
    const QList<QSslError::SslError> accepted{
        QSslError::CertificateNotYetValid, QSslError::CertificateExpired,
        QSslError::InvalidCaCertificate, QSslError::CertificateUntrusted,
        QSslError::HostNameMismatch
    };
    if (Utils::allOf(errors,
                     [&accepted](const QSslError &e) { return accepted.contains(e.error()); })) {
        if (!settings().server.validateCert || handleCertificateIssue())
            reply->ignoreSslErrors(errors);
    }
#else // ssl
    Q_UNUSED(reply)
    Q_UNUSED(errors)
#endif // ssl
}

void AxivionPluginPrivate::onStartupProjectChanged(Project *project)
{
    if (project == m_project)
        return;

    if (m_project)
        disconnect(m_project, &Project::fileListChanged, this, &AxivionPluginPrivate::handleOpenedDocs);

    m_project = project;
    clearAllMarks();
    m_currentProjectInfo = {};
    m_axivionOutputPane.updateDashboard();

    if (!m_project)
        return;

    connect(m_project, &Project::fileListChanged, this, &AxivionPluginPrivate::handleOpenedDocs);
    const AxivionProjectSettings *projSettings = AxivionProjectSettings::projectSettings(m_project);
    fetchProjectInfo(projSettings->dashboardProjectName());
}

static QUrl urlForProject(const QString &projectName)
{
    QString dashboard = settings().server.dashboard;
    if (!dashboard.endsWith(QLatin1Char('/')))
        dashboard += QLatin1Char('/');
    return QUrl(dashboard).resolved(QStringLiteral("api/projects/")).resolved(projectName);
}

static constexpr int httpStatusCodeOk = 200;
constexpr char s_htmlContentType[] = "text/html";
constexpr char s_jsonContentType[] = "application/json";

static Group fetchHtmlRecipe(const QUrl &url, const std::function<void(const QByteArray &)> &handler)
{
    struct StorageData
    {
        QByteArray credentials;
    };

    const Storage<StorageData> storage;

    // TODO: Refactor so that it's a common code with fetchDataRecipe().
    const auto onCredentialSetup = [storage] {
        storage->credentials = QByteArrayLiteral("AxToken ") + settings().server.token.toUtf8();
    };

    const auto onQuerySetup = [storage, url](NetworkQuery &query) {
        QNetworkRequest request(url);
        request.setRawHeader("Accept", s_htmlContentType);
        request.setRawHeader("Authorization", storage->credentials);
        const QByteArray ua = "Axivion" + QCoreApplication::applicationName().toUtf8() +
                              "Plugin/" + QCoreApplication::applicationVersion().toUtf8();
        request.setRawHeader("X-Axivion-User-Agent", ua);
        query.setRequest(request);
        query.setNetworkAccessManager(&dd->m_networkAccessManager);
    };

    const auto onQueryDone = [url, handler](const NetworkQuery &query, DoneWith doneWith) {
        QNetworkReply *reply = query.reply();
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QString contentType = reply->header(QNetworkRequest::ContentTypeHeader)
                                        .toString()
                                        .split(';')
                                        .constFirst()
                                        .trimmed()
                                        .toLower();
        if (doneWith == DoneWith::Success && statusCode == httpStatusCodeOk
            && contentType == s_htmlContentType) {
            handler(reply->readAll());
            return DoneResult::Success;
        }
        return DoneResult::Error;
    };

    const Group recipe {
        storage,
        Sync(onCredentialSetup),
        NetworkQueryTask(onQuerySetup, onQueryDone),
    };

    return recipe;
}

template <typename DtoType>
struct GetDtoStorage
{
    QByteArray credential;
    QUrl url;
    std::optional<DtoType> dtoData;
};

template <typename DtoType>
static Group getDtoRecipe(const Storage<GetDtoStorage<DtoType>> &dtoStorage)
{
    const Storage<QByteArray> storage;

    const auto onNetworkQuerySetup = [dtoStorage](NetworkQuery &query) {
        QNetworkRequest request(dtoStorage->url);
        request.setRawHeader("Accept", s_jsonContentType);
        request.setRawHeader("Authorization", dtoStorage->credential);
        const QByteArray ua = "Axivion" + QCoreApplication::applicationName().toUtf8() +
                              "Plugin/" + QCoreApplication::applicationVersion().toUtf8();
        request.setRawHeader("X-Axivion-User-Agent", ua);
        query.setRequest(request);
        query.setNetworkAccessManager(&dd->m_networkAccessManager);
    };

    const auto onNetworkQueryDone = [storage](const NetworkQuery &query, DoneWith doneWith) {
        QNetworkReply *reply = query.reply();
        const QNetworkReply::NetworkError error = reply->error();
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QString contentType = reply->header(QNetworkRequest::ContentTypeHeader)
                                        .toString()
                                        .split(';')
                                        .constFirst()
                                        .trimmed()
                                        .toLower();
        if (doneWith == DoneWith::Success && statusCode == httpStatusCodeOk
            && contentType == s_jsonContentType) {
            *storage = reply->readAll();
            return DoneResult::Success;
        }

        const auto getError = [&]() -> Error {
            if (contentType == s_jsonContentType) {
                try {
                    return DashboardError(reply->url(), statusCode,
                                          reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString(),
                                          Dto::ErrorDto::deserialize(reply->readAll()));
                } catch (const Dto::invalid_dto_exception &) {
                    // ignore
                }
            }
            if (statusCode != 0) {
                return HttpError(reply->url(), statusCode,
                                 reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString(),
                                 QString::fromUtf8(reply->readAll())); // encoding?
            }
            return NetworkError(reply->url(), error, reply->errorString());
        };

        MessageManager::writeFlashing(QStringLiteral("Axivion: %1").arg(getError().message()));
        return DoneResult::Error;
    };

    const auto onDeserializeSetup = [storage](Async<DtoType> &task) {
        const auto deserialize = [](QPromise<DtoType> &promise, const QByteArray &input) {
            try {
                promise.addResult(DtoType::deserialize(input));
            } catch (const Dto::invalid_dto_exception &) {
                promise.future().cancel();
            }
        };
        task.setFutureSynchronizer(ExtensionSystem::PluginManager::futureSynchronizer());
        task.setConcurrentCallData(deserialize, *storage);
    };

    const auto onDeserializeDone = [dtoStorage](const Async<DtoType> &task, DoneWith doneWith) {
        if (doneWith == DoneWith::Success && task.isResultAvailable()) {
            dtoStorage->dtoData = task.result();
        } else {
            MessageManager::writeFlashing(QString("Axivion: %1")
                .arg(Tr::tr("Deserialization of an unexpected Dto structure.")));
        }
    };

    const Group recipe {
        storage,
        NetworkQueryTask(onNetworkQuerySetup, onNetworkQueryDone),
        AsyncTask<DtoType>(onDeserializeSetup, onDeserializeDone)
    };
    return recipe;
};

template <typename DtoType>
struct PostDtoStorage
{
    QByteArray credential;
    QUrl url;
    QByteArray csrfToken;
    QByteArray writeData;
    std::optional<DtoType> dtoData;
};

template <typename DtoType>
static Group postDtoRecipe(const Storage<PostDtoStorage<DtoType>> &dtoStorage)
{
    const Storage<QByteArray> storage;

    const auto onNetworkQuerySetup = [dtoStorage](NetworkQuery &query) {
        QNetworkRequest request(dtoStorage->url);
        request.setRawHeader("Accept", s_jsonContentType);
        request.setRawHeader("Authorization", dtoStorage->credential);
        const QByteArray ua = "Axivion" + QCoreApplication::applicationName().toUtf8() +
                              "Plugin/" + QCoreApplication::applicationVersion().toUtf8();
        request.setRawHeader("X-Axivion-User-Agent", ua);
        request.setRawHeader("Content-Type", "application/json");
        request.setRawHeader("AX-CSRF-Token", dtoStorage->csrfToken);
        query.setRequest(request);
        query.setWriteData(dtoStorage->writeData);
        query.setOperation(NetworkOperation::Post);
        query.setNetworkAccessManager(&dd->m_networkAccessManager);
    };

    const auto onNetworkQueryDone = [storage](const NetworkQuery &query, DoneWith doneWith) {
        QNetworkReply *reply = query.reply();
        const QNetworkReply::NetworkError error = reply->error();
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QString contentType = reply->header(QNetworkRequest::ContentTypeHeader)
                                        .toString()
                                        .split(';')
                                        .constFirst()
                                        .trimmed()
                                        .toLower();
        if (doneWith == DoneWith::Success && statusCode == httpStatusCodeOk
            && contentType == s_jsonContentType) {
            *storage = reply->readAll();
            return DoneResult::Success;
        }

        const auto getError = [&]() -> Error {
            if (contentType == s_jsonContentType) {
                try {
                    return DashboardError(reply->url(), statusCode,
                                          reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString(),
                                          Dto::ErrorDto::deserialize(reply->readAll()));
                } catch (const Dto::invalid_dto_exception &) {
                    // ignore
                }
            }
            if (statusCode != 0) {
                return HttpError(reply->url(), statusCode,
                                 reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString(),
                                 QString::fromUtf8(reply->readAll())); // encoding?
            }
            return NetworkError(reply->url(), error, reply->errorString());
        };

        MessageManager::writeFlashing(QStringLiteral("Axivion: %1").arg(getError().message()));
        return DoneResult::Error;
    };

    const auto onDeserializeSetup = [storage](Async<DtoType> &task) {
        const auto deserialize = [](QPromise<DtoType> &promise, const QByteArray &input) {
            promise.addResult(DtoType::deserialize(input));
        };
        task.setFutureSynchronizer(ExtensionSystem::PluginManager::futureSynchronizer());
        task.setConcurrentCallData(deserialize, *storage);
    };

    const auto onDeserializeDone = [dtoStorage](const Async<DtoType> &task, DoneWith doneWith) {
        if (doneWith == DoneWith::Success)
            dtoStorage->dtoData = task.future().result();
    };

    const Group recipe {
        storage,
        NetworkQueryTask(onNetworkQuerySetup, onNetworkQueryDone),
        AsyncTask<DtoType>(onDeserializeSetup, onDeserializeDone)
    };
    return recipe;
};

template<typename DtoType>
static Group fetchDataRecipe(const QUrl &url, const std::function<void(const DtoType &)> &handler)
{
    const Storage<QString> passwordStorage;
    const Storage<GetDtoStorage<Dto::DashboardInfoDto>> dashboardStorage;
    const Storage<PostDtoStorage<Dto::ApiTokenInfoDto>> apiTokenStorage;
    const Storage<GetDtoStorage<DtoType>> dtoStorage;

    const auto onGetCredentialSetup = [](CredentialQuery &credential) {
        credential.setOperation(CredentialOperation::Get);
        credential.setService(s_axivionKeychainService);
        credential.setKey(credentialKey());
    };
    const auto onGetCredentialDone = [](const CredentialQuery &credential, DoneWith result) {
        if (result == DoneWith::Success)
            dd->m_apiToken = credential.data();
        // TODO: Show the message about keystore error and info that we can't authorize without it.
    };
    const auto onDashboardGroupSetup = [passwordStorage, dashboardStorage] {
        if (dd->m_apiToken)
            return SetupResult::StopWithSuccess;

        bool ok = false;
        const QString text(Tr::tr("Enter the password for:\nDashboard: %1\nUser: %2")
                               .arg(settings().server.dashboard, settings().server.username));
        *passwordStorage = QInputDialog::getText(ICore::mainWindow(),
            Tr::tr("Axivion Server Password"), text, QLineEdit::Password, {}, &ok);
        if (!ok)
            return SetupResult::StopWithError;

        const QString credential = settings().server.username + ':' + *passwordStorage;
        dashboardStorage->credential = "Basic " + credential.toUtf8().toBase64();
        dashboardStorage->url = QUrl(settings().server.dashboard);
        return SetupResult::Continue;
    };

    const auto onApiTokenGroupSetup = [passwordStorage, dashboardStorage, apiTokenStorage] {
        if (!dashboardStorage->dtoData)
            return SetupResult::StopWithSuccess;

        dd->m_dashboardInfo = toDashboardInfo(settings().server.dashboard,
                                              *dashboardStorage->dtoData);

        const Dto::DashboardInfoDto &dashboardDto = *dashboardStorage->dtoData;
        if (!dashboardDto.userApiTokenUrl)
            return SetupResult::StopWithError;

        apiTokenStorage->credential = dashboardStorage->credential;
        apiTokenStorage->url
            = QUrl(settings().server.dashboard).resolved(*dashboardDto.userApiTokenUrl);
        apiTokenStorage->csrfToken = dashboardDto.csrfToken.toUtf8();
        const Dto::ApiTokenCreationRequestDto requestDto{*passwordStorage, "IdePlugin",
                                                         apiTokenDescription(), 0};
        apiTokenStorage->writeData = requestDto.serialize();
        return SetupResult::Continue;
    };

    const auto onSetCredentialSetup = [apiTokenStorage](CredentialQuery &credential) {
        if (!apiTokenStorage->dtoData || !apiTokenStorage->dtoData->token)
            return SetupResult::StopWithSuccess;

        dd->m_apiToken = apiTokenStorage->dtoData->token->toUtf8();
        credential.setOperation(CredentialOperation::Set);
        credential.setService(s_axivionKeychainService);
        credential.setKey(credentialKey());
        credential.setData(*dd->m_apiToken);
        return SetupResult::Continue;
    };

    const auto onDtoSetup = [dtoStorage, url] {
        if (!dd->m_apiToken)
            return SetupResult::StopWithError;

        dtoStorage->credential = "AxToken " + *dd->m_apiToken;
        dtoStorage->url = url;
        return SetupResult::Continue;
    };
    const auto onDtoDone = [dtoStorage, handler] {
        if (dtoStorage->dtoData)
            handler(*dtoStorage->dtoData);
    };

    const Group recipe {
        Group {
            LoopUntil([](int) { return !dd->m_apiToken; }),
            CredentialQueryTask(onGetCredentialSetup, onGetCredentialDone),
            Group {
                passwordStorage,
                dashboardStorage,
                onGroupSetup(onDashboardGroupSetup),
                Group { // GET DashboardInfoDto
                    finishAllAndSuccess,
                    getDtoRecipe(dashboardStorage),
                },
                Group { // POST ApiTokenCreationRequestDto, GET ApiTokenInfoDto.
                    apiTokenStorage,
                    onGroupSetup(onApiTokenGroupSetup),
                    postDtoRecipe(apiTokenStorage),
                    CredentialQueryTask(onSetCredentialSetup)
                }
            }
        },
        Group {
            dtoStorage,
            onGroupSetup(onDtoSetup),
            getDtoRecipe(dtoStorage),
            onGroupDone(onDtoDone)
        }
    };
    return recipe;
}

Group dashboardInfoRecipe(const DashboardInfoHandler &handler)
{
    const auto onSetup = [handler] {
        if (dd->m_dashboardInfo) {
            if (handler)
                handler(*dd->m_dashboardInfo);
            return SetupResult::StopWithSuccess;
        }
        return SetupResult::Continue;
    };
    const auto onDone = [handler] {
        if (handler)
            handler(make_unexpected(QString("Error"))); // TODO: Collect error message in the storage.
    };

    const QUrl url(settings().server.dashboard);

    const auto resultHandler = [handler, url](const Dto::DashboardInfoDto &data) {
        dd->m_dashboardInfo = toDashboardInfo(url, data);
        if (handler)
            handler(*dd->m_dashboardInfo);
    };

    const Group root {
        onGroupSetup(onSetup), // Stops if cache exists.
        fetchDataRecipe<Dto::DashboardInfoDto>(url, resultHandler),
        onGroupDone(onDone, CallDoneIf::Error)
    };
    return root;
}

Group issueTableRecipe(const IssueListSearch &search, const IssueTableHandler &handler)
{
    QTC_ASSERT(dd->m_currentProjectInfo, return {}); // TODO: Call handler with unexpected?

    const QString query = search.toQuery();
    if (query.isEmpty())
        return {}; // TODO: Call handler with unexpected?

    const QUrl url = urlForProject(dd->m_currentProjectInfo.value().name + '/')
                         .resolved(QString("issues" + query));

    return fetchDataRecipe<Dto::IssueTableDto>(url, handler);
}

Group lineMarkerRecipe(const FilePath &filePath, const LineMarkerHandler &handler)
{
    QTC_ASSERT(dd->m_currentProjectInfo, return {}); // TODO: Call handler with unexpected?
    QTC_ASSERT(!filePath.isEmpty(), return {}); // TODO: Call handler with unexpected?

    const QString fileName = QString::fromUtf8(QUrl::toPercentEncoding(filePath.path()));
    const QUrl url = urlForProject(dd->m_currentProjectInfo.value().name + '/')
                         .resolved(QString("files?filename=" + fileName));
    return fetchDataRecipe<Dto::FileViewDto>(url, handler);
}

Group issueHtmlRecipe(const QString &issueId, const HtmlHandler &handler)
{
    QTC_ASSERT(dd->m_currentProjectInfo, return {}); // TODO: Call handler with unexpected?

    QString dashboard = settings().server.dashboard;
    if (!dashboard.endsWith(QLatin1Char('/')))
        dashboard += QLatin1Char('/');

    const QUrl url = urlForProject(dd->m_currentProjectInfo.value().name + '/')
                         .resolved(QString("issues/"))
                         .resolved(QString(issueId + '/'))
                         .resolved(QString("properties"));

    return fetchHtmlRecipe(url, handler);
}

void AxivionPluginPrivate::fetchProjectInfo(const QString &projectName)
{
    if (!m_project)
        return;

    clearAllMarks();
    if (projectName.isEmpty()) {
        m_currentProjectInfo = {};
        m_axivionOutputPane.updateDashboard();
        return;
    }

    const auto onTaskTreeSetup = [this, projectName](TaskTree &taskTree) {
        if (!m_dashboardInfo)
            return SetupResult::StopWithError;

        const auto it = m_dashboardInfo->projectUrls.constFind(projectName);
        if (it == m_dashboardInfo->projectUrls.constEnd())
            return SetupResult::StopWithError;

        const auto handler = [this](const Dto::ProjectInfoDto &data) {
            m_currentProjectInfo = data;
            m_axivionOutputPane.updateDashboard();
            handleOpenedDocs();
        };

        const QUrl url(settings().server.dashboard);
        taskTree.setRecipe(fetchDataRecipe<Dto::ProjectInfoDto>(url.resolved(*it), handler));
        return SetupResult::Continue;
    };

    const Group root {
        dashboardInfoRecipe(),
        TaskTreeTask(onTaskTreeSetup)
    };
    m_taskTreeRunner.start(root);
}

Group tableInfoRecipe(const QString &prefix, const TableInfoHandler &handler)
{
    const QUrl url = urlForProject(dd->m_currentProjectInfo.value().name + '/')
                         .resolved(QString("issues_meta?kind=" + prefix));
    return fetchDataRecipe<Dto::TableInfoDto>(url, handler);
}

void AxivionPluginPrivate::fetchIssueInfo(const QString &id)
{
    if (!m_currentProjectInfo)
        return;

    const auto ruleHandler = [](const QByteArray &htmlText) {
        QByteArray fixedHtml = htmlText;
        const int idx = htmlText.indexOf("<div class=\"ax-issuedetails-table-container\">");
        if (idx >= 0)
            fixedHtml = "<html><body>" + htmlText.mid(idx);
        dd->m_axivionOutputPane.updateAndShowRule(QString::fromUtf8(fixedHtml));
    };

    m_issueInfoRunner.start(issueHtmlRecipe(id, ruleHandler));
}

void AxivionPluginPrivate::handleOpenedDocs()
{
    const QList<IDocument *> openDocuments = DocumentModel::openedDocuments();
    for (IDocument *doc : openDocuments)
        onDocumentOpened(doc);
}

void AxivionPluginPrivate::clearAllMarks()
{
    const QList<IDocument *> openDocuments = DocumentModel::openedDocuments();
    for (IDocument *doc : openDocuments)
        onDocumentClosed(doc);
}

void AxivionPluginPrivate::onDocumentOpened(IDocument *doc)
{
    if (!doc || !m_currentProjectInfo || !m_project || !m_project->isKnownFile(doc->filePath()))
        return;

    const FilePath filePath = doc->filePath().relativeChildPath(m_project->projectDirectory());
    QTC_ASSERT(!filePath.isEmpty(), return);

    const auto handler = [this](const Dto::FileViewDto &data) {
        if (data.lineMarkers.empty())
            return;
        handleIssuesForFile(data);
    };
    TaskTree *taskTree = new TaskTree;
    taskTree->setRecipe(lineMarkerRecipe(filePath, handler));
    m_docMarksTrees.insert_or_assign(doc, std::unique_ptr<TaskTree>(taskTree));
    connect(taskTree, &TaskTree::done, this, [this, doc] {
        const auto it = m_docMarksTrees.find(doc);
        QTC_ASSERT(it != m_docMarksTrees.end(), return);
        it->second.release()->deleteLater();
        m_docMarksTrees.erase(it);
    });
    taskTree->start();
}

void AxivionPluginPrivate::onDocumentClosed(IDocument *doc)
{
    const auto document = qobject_cast<TextDocument *>(doc);
    if (!document)
        return;

    const auto it = m_docMarksTrees.find(doc);
    if (it != m_docMarksTrees.end())
        m_docMarksTrees.erase(it);

    const TextMarks &marks = document->marks();
    for (TextMark *mark : marks) {
        if (mark->category().id == s_axivionTextMarkId)
            delete mark;
    }
}

void AxivionPluginPrivate::handleIssuesForFile(const Dto::FileViewDto &fileView)
{
    if (fileView.lineMarkers.empty())
        return;

    Project *project = ProjectManager::startupProject();
    if (!project)
        return;

    const FilePath filePath = project->projectDirectory().pathAppended(fileView.fileName);

    for (const Dto::LineMarkerDto &marker : std::as_const(fileView.lineMarkers)) {
        // FIXME the line location can be wrong (even the whole issue could be wrong)
        // depending on whether this line has been changed since the last axivion run and the
        // current state of the file - some magic has to happen here
        new AxivionTextMark(filePath, marker);
    }
}

class AxivionPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Axivion.json")

    ~AxivionPlugin() final
    {
        AxivionProjectSettings::destroyProjectSettings();
        delete dd;
        dd = nullptr;
    }

    void initialize() final
    {
        dd = new AxivionPluginPrivate;

        AxivionProjectSettings::setupProjectPanel();

        connect(ProjectManager::instance(), &ProjectManager::startupProjectChanged,
                dd, &AxivionPluginPrivate::onStartupProjectChanged);
        connect(EditorManager::instance(), &EditorManager::documentOpened,
                dd, &AxivionPluginPrivate::onDocumentOpened);
        connect(EditorManager::instance(), &EditorManager::documentClosed,
                dd, &AxivionPluginPrivate::onDocumentClosed);
    }
};

} // Axivion::Internal

#include "axivionplugin.moc"
