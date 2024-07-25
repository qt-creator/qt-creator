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
#include <coreplugin/inavigationwidgetfactory.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/navigationwidget.h>

#include <extensionsystem/iplugin.h>

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
#include <utils/checkablemessagebox.h>
#include <utils/environment.h>
#include <utils/fileinprojectfinder.h>
#include <utils/networkaccessmanager.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QDesktopServices>
#include <QInputDialog>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTextBrowser>
#include <QTimer>
#include <QUrlQuery>

#include <memory>

constexpr char s_axivionTextMarkId[] = "AxivionTextMark";
constexpr char s_axivionKeychainService[] = "keychain.axivion.qtcreator";

using namespace Core;
using namespace ProjectExplorer;
using namespace Tasking;
using namespace TextEditor;
using namespace Utils;

namespace Axivion::Internal {

QIcon iconForIssue(const std::optional<Dto::IssueKind> &issueKind)
{
    if (!issueKind)
        return {};

    static QHash<Dto::IssueKind, QIcon> prefixToIcon;

    auto it = prefixToIcon.constFind(*issueKind);
    if (it != prefixToIcon.constEnd())
        return *it;

    const QLatin1String prefix = Dto::IssueKindMeta::enumToStr(*issueKind);
    const Icon icon({{FilePath::fromString(":/axivion/images/button-" + prefix + ".png"),
                      Theme::PaletteButtonText}}, Icon::Tint);
    return prefixToIcon.insert(*issueKind, icon.icon()).value();
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
        value = anyMap.find("tag");
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

static QString escapeKey(const QString &string)
{
    QString escaped = string;
    return escaped.replace('\\', "\\\\").replace('@', "\\@");
}

static QString credentialKey(const AxivionServer &server)
{
    return escapeKey(server.username) + '@' + escapeKey(server.dashboard);
}

template <typename DtoType>
struct GetDtoStorage
{
    QUrl url;
    std::optional<QByteArray> credential;
    std::optional<DtoType> dtoData;
};

template <typename DtoType>
struct PostDtoStorage
{
    QUrl url;
    std::optional<QByteArray> credential;
    QByteArray csrfToken;
    QByteArray writeData;
    std::optional<DtoType> dtoData;
};

static DashboardInfo toDashboardInfo(const GetDtoStorage<Dto::DashboardInfoDto> &dashboardStorage)
{
    const Dto::DashboardInfoDto &infoDto = *dashboardStorage.dtoData;
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
    return {dashboardStorage.url, versionNumber, projects, projectUrls, infoDto.checkCredentialsUrl};
}

QUrlQuery IssueListSearch::toUrlQuery(QueryMode mode) const
{
    QUrlQuery query;
    QTC_ASSERT(!kind.isEmpty(), return query);
    query.addQueryItem("kind", kind);
    if (!versionStart.isEmpty())
        query.addQueryItem("start", versionStart);
    if (!versionEnd.isEmpty())
        query.addQueryItem("end", versionEnd);
    if (mode == QueryMode::SimpleQuery)
        return query;

    if (!owner.isEmpty())
        query.addQueryItem("user", owner);
    if (!filter_path.isEmpty())
        query.addQueryItem("filter_any path", filter_path);
    if (!state.isEmpty())
        query.addQueryItem("state", state);
    if (mode == QueryMode::FilterQuery)
        return query;

    QTC_CHECK(mode == QueryMode::FullQuery);
    query.addQueryItem("offset", QString::number(offset));
    if (limit)
        query.addQueryItem("limit", QString::number(limit));
    if (computeTotalRowCount)
        query.addQueryItem("computeTotalRowCount", "true");
    if (!sort.isEmpty())
        query.addQueryItem("sort", sort);
    return query;
}

enum class ServerAccess { Unknown, NoAuthorization, WithAuthorization };

class AxivionPluginPrivate : public QObject
{
    Q_OBJECT
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
    void setIssueDetails(const QString &issueDetailsHtml);
    void handleAnchorClicked(const QUrl &url);

signals:
    void issueDetailsChanged(const QString &issueDetailsHtml);

public:
    // active id used for any network communication, defaults to settings' default
    // set to projects settings' dashboard id on open project
    Id m_dashboardServerId;
    // TODO: Should be set to Unknown on server address change in settings.
    ServerAccess m_serverAccess = ServerAccess::Unknown;
    // TODO: Should be cleared on username change in settings.
    std::optional<QByteArray> m_apiToken;
    NetworkAccessManager m_networkAccessManager;
    std::optional<DashboardInfo> m_dashboardInfo;
    std::optional<Dto::ProjectInfoDto> m_currentProjectInfo;
    std::optional<QString> m_analysisVersion;
    Project *m_project = nullptr;
    bool m_runningQuery = false;
    TaskTreeRunner m_taskTreeRunner;
    std::unordered_map<IDocument *, std::unique_ptr<TaskTree>> m_docMarksTrees;
    TaskTreeRunner m_issueInfoRunner;
    FileInProjectFinder m_fileFinder; // FIXME maybe obsolete when path mapping is implemented
    QMetaObject::Connection m_fileFinderConnection;
};

static AxivionPluginPrivate *dd = nullptr;

class AxivionTextMark : public TextMark
{
public:
    AxivionTextMark(const FilePath &filePath, const Dto::LineMarkerDto &issue,
                    std::optional<Theme::Color> color)
        : TextMark(filePath, issue.startLine, {"Axivion", s_axivionTextMarkId})
    {
        const QString markText = issue.description;
        const QString id = issue.kind + QString::number(issue.id.value_or(-1));
        setToolTip(id + '\n' + markText);
        setIcon(iconForIssue(issue.getOptionalKindEnum()));
        if (color)
            setColor(*color);
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
bool handleCertificateIssue(const Utils::Id &serverId)
{
    QTC_ASSERT(dd, return false);
    const QString serverHost = QUrl(settings().serverForId(serverId).dashboard).host();
    if (QMessageBox::question(ICore::dialogParent(), Tr::tr("Certificate Error"),
                              Tr::tr("Server certificate for %1 cannot be authenticated.\n"
                                     "Do you want to disable SSL verification for this server?\n"
                                     "Note: This can expose you to man-in-the-middle attack.")
                              .arg(serverHost))
            != QMessageBox::Yes) {
        return false;
    }
    settings().disableCertificateValidation(serverId);
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
    QTC_ASSERT(dd, return);
#if QT_CONFIG(ssl)
    const QList<QSslError::SslError> accepted{
        QSslError::CertificateNotYetValid, QSslError::CertificateExpired,
        QSslError::InvalidCaCertificate, QSslError::CertificateUntrusted,
        QSslError::HostNameMismatch
    };
    if (Utils::allOf(errors,
                     [&accepted](const QSslError &e) { return accepted.contains(e.error()); })) {
        const bool shouldValidate = settings().serverForId(dd->m_dashboardServerId).validateCert;
        if (!shouldValidate || handleCertificateIssue(dd->m_dashboardServerId))
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
        disconnect(m_fileFinderConnection);

    m_project = project;
    clearAllMarks();
    m_currentProjectInfo = {};
    m_analysisVersion = {};
    updateDashboard();

    if (!m_project) {
        m_fileFinder.setProjectDirectory({});
        m_fileFinder.setProjectFiles({});
        return;
    }

    m_fileFinder.setProjectDirectory(m_project->projectDirectory());
    m_fileFinderConnection = connect(m_project, &Project::fileListChanged, this, [this] {
        m_fileFinder.setProjectFiles(m_project->files(Project::AllFiles));
        handleOpenedDocs();
    });
    const AxivionProjectSettings *projSettings = AxivionProjectSettings::projectSettings(m_project);
    switchActiveDashboardId(projSettings->dashboardId());
    fetchProjectInfo(projSettings->dashboardProjectName());
}

static QUrl constructUrl(const QString &projectName, const QString &subPath, const QUrlQuery &query)
{
    if (!dd->m_dashboardInfo)
        return {};
    QUrl url = dd->m_dashboardInfo->source.resolved(QString("api/projects/" + projectName + '/'));
    if (!subPath.isEmpty() && QTC_GUARD(!subPath.startsWith('/')))
        url = url.resolved(subPath);
    if (!query.isEmpty())
        url.setQuery(query);
    return url;
}

static constexpr int httpStatusCodeOk = 200;
constexpr char s_htmlContentType[] = "text/html";
constexpr char s_jsonContentType[] = "application/json";

static bool isServerAccessEstablished()
{
    return dd->m_serverAccess == ServerAccess::NoAuthorization
           || (dd->m_serverAccess == ServerAccess::WithAuthorization && dd->m_apiToken);
}

static Group fetchHtmlRecipe(const QUrl &url, const std::function<void(const QByteArray &)> &handler)
{
    // TODO: Refactor so that it's a common code with fetchDataRecipe().
    const auto onQuerySetup = [url](NetworkQuery &query) {
        if (!isServerAccessEstablished())
            return SetupResult::StopWithError; // TODO: start authorizationRecipe()?

        QNetworkRequest request(url);
        request.setRawHeader("Accept", s_htmlContentType);
        if (dd->m_serverAccess == ServerAccess::WithAuthorization && dd->m_apiToken)
            request.setRawHeader("Authorization", "AxToken " + *dd->m_apiToken);
        const QByteArray ua = "Axivion" + QCoreApplication::applicationName().toUtf8() +
                              "Plugin/" + QCoreApplication::applicationVersion().toUtf8();
        request.setRawHeader("X-Axivion-User-Agent", ua);
        query.setRequest(request);
        query.setNetworkAccessManager(&dd->m_networkAccessManager);
        return SetupResult::Continue;
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
    return {NetworkQueryTask(onQuerySetup, onQueryDone)};
}

template <typename DtoType, template <typename> typename DtoStorageType>
static Group dtoRecipe(const Storage<DtoStorageType<DtoType>> &dtoStorage)
{
    const Storage<std::optional<QByteArray>> storage;

    const auto onNetworkQuerySetup = [dtoStorage](NetworkQuery &query) {
        QNetworkRequest request(dtoStorage->url);
        request.setRawHeader("Accept", s_jsonContentType);
        if (dtoStorage->credential) // Unauthorized access otherwise
            request.setRawHeader("Authorization", *dtoStorage->credential);
        const QByteArray ua = "Axivion" + QCoreApplication::applicationName().toUtf8() +
                              "Plugin/" + QCoreApplication::applicationVersion().toUtf8();
        request.setRawHeader("X-Axivion-User-Agent", ua);

        if constexpr (std::is_same_v<DtoStorageType<DtoType>, PostDtoStorage<DtoType>>) {
            request.setRawHeader("Content-Type", "application/json");
            request.setRawHeader("AX-CSRF-Token", dtoStorage->csrfToken);
            query.setWriteData(dtoStorage->writeData);
            query.setOperation(NetworkOperation::Post);
        }

        query.setRequest(request);
        query.setNetworkAccessManager(&dd->m_networkAccessManager);
    };

    const auto onNetworkQueryDone = [storage, dtoStorage](const NetworkQuery &query,
                                                          DoneWith doneWith) {
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
            dtoStorage->url = reply->url();
            return DoneResult::Success;
        }

        QString errorString;
        if (contentType == s_jsonContentType) {
            const Utils::expected_str<Dto::ErrorDto> error
                = Dto::ErrorDto::deserializeExpected(reply->readAll());

            if (error) {
                if constexpr (std::is_same_v<DtoType, Dto::DashboardInfoDto>) {
                    // Suppress logging error on unauthorized dashboard fetch
                    if (!dtoStorage->credential && error->type == "UnauthenticatedException") {
                        dtoStorage->url = reply->url();
                        return DoneResult::Success;
                    }
                }

                errorString = Error(DashboardError(reply->url(), statusCode,
                    reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString(),
                                     *error)).message();
            } else {
                errorString = error.error();
            }
        } else if (statusCode != 0) {
            errorString = Error(HttpError(reply->url(), statusCode,
                reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString(),
                                 QString::fromUtf8(reply->readAll()))).message(); // encoding?
        } else {
            errorString = Error(NetworkError(reply->url(), error, reply->errorString())).message();
        }

        MessageManager::writeDisrupting(QString("Axivion: %1").arg(errorString));
        return DoneResult::Error;
    };

    const auto onDeserializeSetup = [storage](Async<expected_str<DtoType>> &task) {
        if (!*storage)
            return SetupResult::StopWithSuccess;

        const auto deserialize = [](QPromise<expected_str<DtoType>> &promise, const QByteArray &input) {
            promise.addResult(DtoType::deserializeExpected(input));
        };
        task.setConcurrentCallData(deserialize, **storage);
        return SetupResult::Continue;
    };

    const auto onDeserializeDone = [dtoStorage](const Async<expected_str<DtoType>> &task,
                                                DoneWith doneWith) {
        if (doneWith == DoneWith::Success && task.isResultAvailable()) {
            const auto result = task.result();
            if (result) {
                dtoStorage->dtoData = *result;
                return DoneResult::Success;
            }
            MessageManager::writeFlashing(QString("Axivion: %1").arg(result.error()));
        } else {
            MessageManager::writeFlashing(QString("Axivion: %1")
                .arg(Tr::tr("Unknown Dto structure deserialization error.")));
        }
        return DoneResult::Error;
    };

    return {
        storage,
        NetworkQueryTask(onNetworkQuerySetup, onNetworkQueryDone),
        AsyncTask<expected_str<DtoType>>(onDeserializeSetup, onDeserializeDone)
    };
}

static QString credentialOperationMessage(CredentialOperation operation)
{
    switch (operation) {
    case CredentialOperation::Get:
        return Tr::tr("The ApiToken cannot be read in a secure way.");
    case CredentialOperation::Set:
        return Tr::tr("The ApiToken cannot be stored in a secure way.");
    case CredentialOperation::Delete:
        return Tr::tr("The ApiToken cannot be deleted in a secure way.");
    }
    return {};
}

static void handleCredentialError(const CredentialQuery &credential)
{
    const QString keyChainMessage = credential.errorString().isEmpty() ? QString()
        : QString(" %1").arg(Tr::tr("Key chain message: \"%1\".").arg(credential.errorString()));
    MessageManager::writeFlashing(QString("Axivion: %1")
        .arg(credentialOperationMessage(credential.operation()) + keyChainMessage));
}

static Group authorizationRecipe()
{
    const Id serverId = dd->m_dashboardServerId;
    const Storage<QUrl> serverUrlStorage;
    const Storage<GetDtoStorage<Dto::DashboardInfoDto>> unauthorizedDashboardStorage;
    const auto onUnauthorizedGroupSetup = [serverUrlStorage, unauthorizedDashboardStorage] {
        unauthorizedDashboardStorage->url = *serverUrlStorage;
        return isServerAccessEstablished() ? SetupResult::StopWithSuccess : SetupResult::Continue;
    };
    const auto onUnauthorizedDashboard = [unauthorizedDashboardStorage, serverId] {
        if (unauthorizedDashboardStorage->dtoData) {
            const Dto::DashboardInfoDto &dashboardInfo = *unauthorizedDashboardStorage->dtoData;
            const QString &username = settings().serverForId(serverId).username;
            if (username.isEmpty()
                || (dashboardInfo.username && *dashboardInfo.username == username)) {
                dd->m_serverAccess = ServerAccess::NoAuthorization;
                dd->m_dashboardInfo = toDashboardInfo(*unauthorizedDashboardStorage);
                return;
            }
            MessageManager::writeFlashing(QString("Axivion: %1")
                .arg(Tr::tr("Unauthenticated access failed (wrong user), "
                            "using authenticated access...")));
        }
        dd->m_serverAccess = ServerAccess::WithAuthorization;
    };

    const auto onCredentialLoopCondition = [](int) {
        return dd->m_serverAccess == ServerAccess::WithAuthorization && !dd->m_apiToken;
    };
    const auto onGetCredentialSetup = [serverId](CredentialQuery &credential) {
        credential.setOperation(CredentialOperation::Get);
        credential.setService(s_axivionKeychainService);
        credential.setKey(credentialKey(settings().serverForId(serverId)));
    };
    const auto onGetCredentialDone = [](const CredentialQuery &credential, DoneWith result) {
        if (result == DoneWith::Success)
            dd->m_apiToken = credential.data();
        else
            handleCredentialError(credential);
        // TODO: In case of an error we are multiplying the ApiTokens on Axivion server for each
        //       Creator run, but at least things should continue to work OK in the current session.
        return DoneResult::Success;
    };

    const Storage<QString> passwordStorage;
    const Storage<GetDtoStorage<Dto::DashboardInfoDto>> dashboardStorage;
    const auto onPasswordGroupSetup
            = [serverId, serverUrlStorage, passwordStorage, dashboardStorage] {
        if (dd->m_apiToken)
            return SetupResult::StopWithSuccess;

        bool ok = false;
        const AxivionServer server = settings().serverForId(serverId);
        const QString text(Tr::tr("Enter the password for:\nDashboard: %1\nUser: %2")
                               .arg(server.dashboard, server.username));
        *passwordStorage = QInputDialog::getText(ICore::mainWindow(),
            Tr::tr("Axivion Server Password"), text, QLineEdit::Password, {}, &ok);
        if (!ok)
            return SetupResult::StopWithError;

        const QString credential = server.username + ':' + *passwordStorage;
        dashboardStorage->credential = "Basic " + credential.toUtf8().toBase64();
        dashboardStorage->url = *serverUrlStorage;
        return SetupResult::Continue;
    };

    const Storage<PostDtoStorage<Dto::ApiTokenInfoDto>> apiTokenStorage;
    const auto onApiTokenGroupSetup = [passwordStorage, dashboardStorage, apiTokenStorage] {
        if (!dashboardStorage->dtoData)
            return SetupResult::StopWithSuccess;

        dd->m_dashboardInfo = toDashboardInfo(*dashboardStorage);

        const Dto::DashboardInfoDto &dashboardDto = *dashboardStorage->dtoData;
        if (!dashboardDto.userApiTokenUrl)
            return SetupResult::StopWithError;

        apiTokenStorage->credential = dashboardStorage->credential;
        apiTokenStorage->url
            = dd->m_dashboardInfo->source.resolved(*dashboardDto.userApiTokenUrl);
        apiTokenStorage->csrfToken = dashboardDto.csrfToken.toUtf8();
        const Dto::ApiTokenCreationRequestDto requestDto{*passwordStorage, "IdePlugin",
                                                         apiTokenDescription(), 0};
        apiTokenStorage->writeData = requestDto.serialize();
        return SetupResult::Continue;
    };

    const auto onSetCredentialSetup = [apiTokenStorage, serverId](CredentialQuery &credential) {
        if (!apiTokenStorage->dtoData || !apiTokenStorage->dtoData->token)
            return SetupResult::StopWithSuccess;

        dd->m_apiToken = apiTokenStorage->dtoData->token->toUtf8();
        credential.setOperation(CredentialOperation::Set);
        credential.setService(s_axivionKeychainService);
        credential.setKey(credentialKey(settings().serverForId(serverId)));
        credential.setData(*dd->m_apiToken);
        return SetupResult::Continue;
    };
    const auto onSetCredentialDone = [](const CredentialQuery &credential) {
        handleCredentialError(credential);
        // TODO: In case of an error we are multiplying the ApiTokens on Axivion server for each
        //       Creator run, but at least things should continue to work OK in the current session.
        return DoneResult::Success;
    };

    const auto onDashboardGroupSetup = [serverUrlStorage, dashboardStorage] {
        if (dd->m_dashboardInfo || dd->m_serverAccess != ServerAccess::WithAuthorization
            || !dd->m_apiToken) {
            return SetupResult::StopWithSuccess; // Unauthorized access should have collect dashboard before
        }
        dashboardStorage->credential = "AxToken " + *dd->m_apiToken;
        dashboardStorage->url = *serverUrlStorage;
        return SetupResult::Continue;
    };
    const auto onDeleteCredentialSetup = [dashboardStorage, serverId](CredentialQuery &credential) {
        if (dashboardStorage->dtoData) {
            dd->m_dashboardInfo = toDashboardInfo(*dashboardStorage);
            return SetupResult::StopWithSuccess;
        }
        dd->m_apiToken = {};
        MessageManager::writeFlashing(QString("Axivion: %1")
            .arg(Tr::tr("The stored ApiToken is not valid anymore, removing it.")));
        credential.setOperation(CredentialOperation::Delete);
        credential.setService(s_axivionKeychainService);
        credential.setKey(credentialKey(settings().serverForId(serverId)));
        return SetupResult::Continue;
    };

    return {
        serverUrlStorage,
        onGroupSetup([serverUrlStorage, serverId] {
            *serverUrlStorage = QUrl(settings().serverForId(serverId).dashboard);
        }),
        Group {
            unauthorizedDashboardStorage,
            onGroupSetup(onUnauthorizedGroupSetup),
            dtoRecipe(unauthorizedDashboardStorage),
            Sync(onUnauthorizedDashboard),
            onGroupDone([serverUrlStorage, unauthorizedDashboardStorage] {
                *serverUrlStorage = unauthorizedDashboardStorage->url;
            }),
        },
        For {
            LoopUntil(onCredentialLoopCondition),
            CredentialQueryTask(onGetCredentialSetup, onGetCredentialDone),
            Group {
                passwordStorage,
                dashboardStorage,
                onGroupSetup(onPasswordGroupSetup),
                Group { // GET DashboardInfoDto
                    finishAllAndSuccess,
                    dtoRecipe(dashboardStorage)
                },
                Group { // POST ApiTokenCreationRequestDto, GET ApiTokenInfoDto.
                    apiTokenStorage,
                    onGroupSetup(onApiTokenGroupSetup),
                    dtoRecipe(apiTokenStorage),
                    CredentialQueryTask(onSetCredentialSetup, onSetCredentialDone, CallDoneIf::Error)
                }
            },
            Group {
                finishAllAndSuccess,
                dashboardStorage,
                onGroupSetup(onDashboardGroupSetup),
                dtoRecipe(dashboardStorage),
                CredentialQueryTask(onDeleteCredentialSetup)
            }
        }
    };
}

template<typename DtoType>
static Group fetchDataRecipe(const QUrl &url, const std::function<void(const DtoType &)> &handler)
{
    const Storage<GetDtoStorage<DtoType>> dtoStorage;

    const auto onDtoSetup = [dtoStorage, url] {
        if (!isServerAccessEstablished())
            return SetupResult::StopWithError;

        if (dd->m_serverAccess == ServerAccess::WithAuthorization && dd->m_apiToken)
            dtoStorage->credential = "AxToken " + *dd->m_apiToken;
        dtoStorage->url = url;
        return SetupResult::Continue;
    };
    const auto onDtoDone = [dtoStorage, handler] {
        if (dtoStorage->dtoData)
            handler(*dtoStorage->dtoData);
    };

    const Group recipe {
        authorizationRecipe(),
        Group {
            dtoStorage,
            onGroupSetup(onDtoSetup),
            dtoRecipe(dtoStorage),
            onGroupDone(onDtoDone)
        }
    };
    return recipe;
}

Group dashboardInfoRecipe(const DashboardInfoHandler &handler)
{
    const auto onSetup = [handler] {
        if (dd->m_dashboardInfo) {
            handler(*dd->m_dashboardInfo);
            return SetupResult::StopWithSuccess;
        }
        return SetupResult::Continue;
    };
    const auto onDone = [handler](DoneWith result) {
        if (result == DoneWith::Success && dd->m_dashboardInfo)
            handler(*dd->m_dashboardInfo);
        else
            handler(make_unexpected(QString("Error"))); // TODO: Collect error message in the storage.
    };

    const Group root {
        onGroupSetup(onSetup), // Stops if cache exists.
        authorizationRecipe(),
        onGroupDone(onDone)
    };
    return root;
}

Group issueTableRecipe(const IssueListSearch &search, const IssueTableHandler &handler)
{
    QTC_ASSERT(dd->m_currentProjectInfo, return {}); // TODO: Call handler with unexpected?

    const QUrlQuery query = search.toUrlQuery(QueryMode::FullQuery);
    if (query.isEmpty())
        return {}; // TODO: Call handler with unexpected?

    const QUrl url = constructUrl(dd->m_currentProjectInfo.value().name, "issues", query);
    return fetchDataRecipe<Dto::IssueTableDto>(url, handler);
}

Group lineMarkerRecipe(const FilePath &filePath, const LineMarkerHandler &handler)
{
    QTC_ASSERT(dd->m_currentProjectInfo, return {}); // TODO: Call handler with unexpected?
    QTC_ASSERT(!filePath.isEmpty(), return {}); // TODO: Call handler with unexpected?
    QTC_ASSERT(dd->m_analysisVersion, return {}); // TODO: Call handler with unexpected?

    const QString fileName = QString::fromUtf8(QUrl::toPercentEncoding(filePath.path()));
    const QUrlQuery query({{"filename", fileName}, {"version", *dd->m_analysisVersion}});
    const QUrl url = constructUrl(dd->m_currentProjectInfo.value().name, "files", query);
    return fetchDataRecipe<Dto::FileViewDto>(url, handler);
}

Group issueHtmlRecipe(const QString &issueId, const HtmlHandler &handler)
{
    QTC_ASSERT(dd->m_currentProjectInfo, return {}); // TODO: Call handler with unexpected?

    const QUrl url = constructUrl(dd->m_currentProjectInfo.value().name,
                                  QString("issues/" + issueId + "/properties/"),
                                  {});
    return fetchHtmlRecipe(url, handler);
}

void AxivionPluginPrivate::fetchProjectInfo(const QString &projectName)
{
    if (!m_project)
        return;

    clearAllMarks();
    if (projectName.isEmpty()) {
        m_currentProjectInfo = {};
        m_analysisVersion = {};
        updateDashboard();
        return;
    }

    const auto onTaskTreeSetup = [this, projectName](TaskTree &taskTree) {
        if (!m_dashboardInfo) {
            MessageManager::writeDisrupting(QString("Axivion: %1")
                .arg(Tr::tr("Fetching DashboardInfo error.")));
            return SetupResult::StopWithError;
        }

        const auto it = m_dashboardInfo->projectUrls.constFind(projectName);
        if (it == m_dashboardInfo->projectUrls.constEnd()) {
            MessageManager::writeDisrupting(QString("Axivion: %1")
                .arg(Tr::tr("The DashboardInfo doesn't contain project \"%1\".").arg(projectName)));
            return SetupResult::StopWithError;
        }

        const auto handler = [this](const Dto::ProjectInfoDto &data) {
            m_currentProjectInfo = data;
            updateDashboard();
            handleOpenedDocs();
        };

        taskTree.setRecipe(
            fetchDataRecipe<Dto::ProjectInfoDto>(m_dashboardInfo->source.resolved(*it), handler));
        return SetupResult::Continue;
    };

    const Group root {
        authorizationRecipe(),
        TaskTreeTask(onTaskTreeSetup)
    };
    m_taskTreeRunner.start(root);
}

Group tableInfoRecipe(const QString &prefix, const TableInfoHandler &handler)
{
    const QUrlQuery query({{"kind", prefix}});
    const QUrl url = constructUrl(dd->m_currentProjectInfo.value().name, "issues_meta", query);
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

        NavigationWidget::activateSubWidget("Axivion.Issue", Side::Right);
        dd->setIssueDetails(QString::fromUtf8(fixedHtml));
    };

    m_issueInfoRunner.start(issueHtmlRecipe(id, ruleHandler));
}

void AxivionPluginPrivate::setIssueDetails(const QString &issueDetailsHtml)
{
    emit issueDetailsChanged(issueDetailsHtml);
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
    if (filePath.isEmpty())
        return; // Empty is fine

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
    std::optional<Theme::Color> color = std::nullopt;
    if (settings().highlightMarks())
        color.emplace(Theme::Color(Theme::Bookmarks_TextMarkColor)); // FIXME!
    for (const Dto::LineMarkerDto &marker : std::as_const(fileView.lineMarkers)) {
        // FIXME the line location can be wrong (even the whole issue could be wrong)
        // depending on whether this line has been changed since the last axivion run and the
        // current state of the file - some magic has to happen here
        new AxivionTextMark(filePath, marker, color);
    }
}

void AxivionPluginPrivate::handleAnchorClicked(const QUrl &url)
{
    QTC_ASSERT(dd, return);
    QTC_ASSERT(dd->m_project, return);
    if (!url.scheme().isEmpty()) {
        const QString detail = Tr::tr("The activated link appears to be external.\n"
                                      "Do you want to open \"%1\" with its default application?")
                .arg(url.toString());
        const QMessageBox::StandardButton pressed
            = CheckableMessageBox::question(Core::ICore::dialogParent(),
                                            Tr::tr("Open External Links"),
                                            detail,
                                            Key("AxivionOpenExternalLinks"));
        if (pressed == QMessageBox::Yes)
            QDesktopServices::openUrl(url);
        return;
    }
    const QUrlQuery query(url);
    if (query.isEmpty())
        return;
    Link link;
    if (const QString path = query.queryItemValue("filename", QUrl::FullyDecoded); !path.isEmpty())
        link.targetFilePath = findFileForIssuePath(FilePath::fromUserInput(path));
    if (const QString line = query.queryItemValue("line"); !line.isEmpty())
        link.targetLine = line.toInt();
    // column entry is wrong - so, ignore it
    if (link.hasValidTarget() && link.targetFilePath.exists())
        EditorManager::openEditorAt(link);
}

class AxivionIssueWidgetFactory final : public INavigationWidgetFactory
{
public:
    AxivionIssueWidgetFactory()
    {
        setDisplayName(Tr::tr("Axivion"));
        setId("Axivion.Issue");
        setPriority(555);
    }

    NavigationView createWidget() final
    {
        QTC_ASSERT(dd, return {});
        QTextBrowser *browser = new QTextBrowser;
        const QString text = Tr::tr(
                    "Search for issues inside the Axivion dashboard or request issue details for "
                    "Axivion inline annotations to see them here.");
        browser->setText("<p style='text-align:center'>" + text + "</p>");
        browser->setOpenLinks(false);
        NavigationView view;
        view.widget = browser;
        connect(dd, &AxivionPluginPrivate::issueDetailsChanged, browser, &QTextBrowser::setHtml);
        connect(browser, &QTextBrowser::anchorClicked,
                dd, &AxivionPluginPrivate::handleAnchorClicked);
        return view;
    }
};

void setupAxivionIssueWidgetFactory()
{
    static AxivionIssueWidgetFactory issueWidgetFactory;
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
        setupAxivionOutputPane(this);

        dd = new AxivionPluginPrivate;

        AxivionProjectSettings::setupProjectPanel();
        setupAxivionIssueWidgetFactory();

        connect(ProjectManager::instance(), &ProjectManager::startupProjectChanged,
                dd, &AxivionPluginPrivate::onStartupProjectChanged);
        connect(EditorManager::instance(), &EditorManager::documentOpened,
                dd, &AxivionPluginPrivate::onDocumentOpened);
        connect(EditorManager::instance(), &EditorManager::documentClosed,
                dd, &AxivionPluginPrivate::onDocumentClosed);
    }
};

void fetchIssueInfo(const QString &id)
{
    QTC_ASSERT(dd, return);
    dd->fetchIssueInfo(id);
}

void switchActiveDashboardId(const Id &toDashboardId)
{
    QTC_ASSERT(dd, return);
    dd->m_dashboardServerId = toDashboardId;
    dd->m_serverAccess = ServerAccess::Unknown;
    dd->m_apiToken.reset();
    dd->m_dashboardInfo.reset();
}

const std::optional<DashboardInfo> currentDashboardInfo()
{
    QTC_ASSERT(dd, return std::nullopt);
    return dd->m_dashboardInfo;
}

void setAnalysisVersion(const QString &version)
{
    QTC_ASSERT(dd, return);
    if (dd->m_analysisVersion.value_or("") == version)
        return;
    dd->m_analysisVersion = version;
    // refetch issues for already opened docs
    dd->clearAllMarks();
    dd->handleOpenedDocs();
}

Utils::FilePath findFileForIssuePath(const Utils::FilePath &issuePath)
{
    QTC_ASSERT(dd, return {});
    const FilePaths result = dd->m_fileFinder.findFile(QUrl::fromLocalFile(issuePath.toString()));
    if (result.size() == 1)
        return dd->m_project->projectDirectory().resolvePath(result.first());
    return {};
}

} // Axivion::Internal

#include "axivionplugin.moc"
