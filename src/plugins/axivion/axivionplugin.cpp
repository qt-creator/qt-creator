// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "axivionplugin.h"

#include "axivionoutputpane.h"
#include "axivionprojectsettings.h"
#include "axivionquery.h"
#include "axivionresultparser.h"
#include "axivionsettings.h"
#include "axiviontr.h"
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
#include <utils/networkaccessmanager.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>

#include <exception>
#include <memory>

constexpr char AxivionTextMarkId[] = "AxivionTextMark";

using namespace Tasking;
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

QString IssueListSearch::toQuery() const
{
    if (kind.isEmpty())
        return {};
    QString result;
    result.append(QString("?kind=%1&offset=%2&limit=%3").arg(kind).arg(offset).arg(limit));
    // TODO other params
    if (computeTotalRowCount)
        result.append("&computeTotalRowCount=true");
    return result;
}

class AxivionPluginPrivate : public QObject
{
public:
    AxivionPluginPrivate();
    void handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);
    void onStartupProjectChanged();
    void fetchProjectInfo(const QString &projectName);
    void handleOpenedDocs(ProjectExplorer::Project *project);
    void onDocumentOpened(Core::IDocument *doc);
    void onDocumentClosed(Core::IDocument * doc);
    void clearAllMarks();
    void handleIssuesForFile(const IssuesList &issues);
    void fetchRuleInfo(const QString &id);

    NetworkAccessManager m_networkAccessManager;
    AxivionOutputPane m_axivionOutputPane;
    std::optional<DashboardInfo> m_dashboardInfo;
    std::optional<Dto::ProjectInfoDto> m_currentProjectInfo;
    bool m_runningQuery = false;
    TaskTreeRunner m_taskTreeRunner;
};

static AxivionPluginPrivate *dd = nullptr;

class AxivionTextMark : public TextEditor::TextMark
{
public:
    AxivionTextMark(const FilePath &filePath, const ShortIssue &issue);

private:
    QString m_id;
};

AxivionTextMark::AxivionTextMark(const FilePath &filePath, const ShortIssue &issue)
    : TextEditor::TextMark(filePath, issue.lineNumber, {Tr::tr("Axivion"), AxivionTextMarkId})
    , m_id(issue.id)
{
    const QString markText = issue.entity.isEmpty() ? issue.message
                                                    : issue.entity + ": " + issue.message;
    setToolTip(issue.errorNumber + " " + markText);
    setIcon(iconForIssue("SV")); // FIXME adapt to the issue
    setPriority(TextEditor::TextMark::NormalPriority);
    setLineAnnotation(markText);
    setActionsProvider([this]{
       auto action = new QAction;
       action->setIcon(Utils::Icons::INFO.icon());
       action->setToolTip(Tr::tr("Show rule details"));
       QObject::connect(action, &QAction::triggered,
                        dd, [this]{ dd->fetchRuleInfo(m_id); });
       return QList{action};
    });
}

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
    if (QMessageBox::question(Core::ICore::dialogParent(), Tr::tr("Certificate Error"),
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

void AxivionPluginPrivate::onStartupProjectChanged()
{
    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    if (!project) {
        clearAllMarks();
        m_currentProjectInfo = {};
        m_axivionOutputPane.updateDashboard();
        return;
    }

    const AxivionProjectSettings *projSettings = AxivionProjectSettings::projectSettings(project);
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
static const QLatin1String jsonContentType{ "application/json" };

template<typename SerializableType>
static Group fetchDataRecipe(const QUrl &url,
                             const std::function<void(const SerializableType &result)> &handler)
{
    struct StorageData
    {
        QByteArray credentials;
        QByteArray serializableData;
    };

    const Storage<StorageData> storage;

    const auto onCredentialSetup = [storage] {
        storage->credentials = QByteArrayLiteral("AxToken ")
                              + settings().server.token.toUtf8();
    };

    const auto onQuerySetup = [storage, url](NetworkQuery &query) {
        QNetworkRequest request(url);
        request.setRawHeader(QByteArrayLiteral("Accept"),
                             QByteArray(jsonContentType.data(), jsonContentType.size()));
        request.setRawHeader(QByteArrayLiteral("Authorization"),
                             storage->credentials);
        const QByteArray ua = QByteArrayLiteral("Axivion")
                              + QCoreApplication::applicationName().toUtf8()
                              + QByteArrayLiteral("Plugin/")
                              + QCoreApplication::applicationVersion().toUtf8();
        request.setRawHeader(QByteArrayLiteral("X-Axivion-User-Agent"), ua);
        query.setRequest(request);
        query.setNetworkAccessManager(&dd->m_networkAccessManager);
    };

    const auto onQueryDone = [storage, url](const NetworkQuery &query, DoneWith doneWith) {
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
            && contentType == jsonContentType) {
            storage->serializableData = reply->readAll();
            return DoneResult::Success;
        }

        const auto getError = [&]() -> Error {
            if (contentType == jsonContentType) {
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

        Core::MessageManager::writeFlashing(
            QStringLiteral("Axivion: %1").arg(getError().message()));
        return DoneResult::Error;
    };

    const auto onDeserializeSetup = [storage](Async<SerializableType> &task) {
        const auto deserialize = [](QPromise<SerializableType> &promise, const QByteArray &input) {
            promise.addResult(SerializableType::deserialize(input));
        };
        task.setFutureSynchronizer(ExtensionSystem::PluginManager::futureSynchronizer());
        task.setConcurrentCallData(deserialize, storage->serializableData);
    };

    const auto onDeserializeDone = [handler](const Async<SerializableType> &task,
                                             DoneWith doneWith) {
        if (doneWith == DoneWith::Success)
            handler(task.future().result());
    };

    const Group recipe {
        storage,
        Sync(onCredentialSetup),
        NetworkQueryTask(onQuerySetup, onQueryDone),
        AsyncTask<SerializableType>(onDeserializeSetup, onDeserializeDone)
    };

    return recipe;
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

void AxivionPluginPrivate::fetchProjectInfo(const QString &projectName)
{
    if (m_taskTreeRunner.isRunning()) { // TODO: cache in queue and run when task tree finished
        QTimer::singleShot(3000, this, [this, projectName] { fetchProjectInfo(projectName); });
        return;
    }
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
            // handle already opened documents
            if (auto buildSystem = ProjectExplorer::ProjectManager::startupBuildSystem();
                !buildSystem || !buildSystem->isParsing()) {
                handleOpenedDocs(nullptr);
            } else {
                connect(ProjectExplorer::ProjectManager::instance(),
                        &ProjectExplorer::ProjectManager::projectFinishedParsing,
                        this, &AxivionPluginPrivate::handleOpenedDocs);
            }
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

void AxivionPluginPrivate::fetchRuleInfo(const QString &id)
{
    if (m_runningQuery) {
        QTimer::singleShot(3000, this, [this, id] { fetchRuleInfo(id); });
        return;
    }

    const QStringList args = id.split(':');
    QTC_ASSERT(args.size() == 2, return);
    m_runningQuery = true;
    AxivionQuery query(AxivionQuery::RuleInfo, args);
    AxivionQueryRunner *runner = new AxivionQueryRunner(query, this);
    connect(runner, &AxivionQueryRunner::resultRetrieved, this, [this](const QByteArray &result){
        m_runningQuery = false;
        m_axivionOutputPane.updateAndShowRule(ResultParser::parseRuleInfo(result));
    });
    connect(runner, &AxivionQueryRunner::finished, [runner]{ runner->deleteLater(); });
    runner->start();
}

void AxivionPluginPrivate::handleOpenedDocs(ProjectExplorer::Project *project)
{
    if (project && ProjectExplorer::ProjectManager::startupProject() != project)
        return;
    const QList<Core::IDocument *> openDocuments = Core::DocumentModel::openedDocuments();
    for (Core::IDocument *doc : openDocuments)
        onDocumentOpened(doc);
    if (project)
        disconnect(ProjectExplorer::ProjectManager::instance(),
                   &ProjectExplorer::ProjectManager::projectFinishedParsing,
                   this, &AxivionPluginPrivate::handleOpenedDocs);
}

void AxivionPluginPrivate::clearAllMarks()
{
    const QList<Core::IDocument *> openDocuments = Core::DocumentModel::openedDocuments();
    for (Core::IDocument *doc : openDocuments)
        onDocumentClosed(doc);
}

void AxivionPluginPrivate::onDocumentOpened(Core::IDocument *doc)
{
    if (!m_currentProjectInfo) // we do not have a project info (yet)
        return;

    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    // TODO: Sometimes the isKnownFile() returns false after opening a session.
    //       This happens randomly on linux.
    if (!doc || !project->isKnownFile(doc->filePath()))
        return;

    const FilePath relative = doc->filePath().relativeChildPath(project->projectDirectory());
    // for now only style violations
    const AxivionQuery query(AxivionQuery::IssuesForFileList, {m_currentProjectInfo->name, "SV",
                                                               relative.path()});
    AxivionQueryRunner *runner = new AxivionQueryRunner(query, this);
    connect(runner, &AxivionQueryRunner::resultRetrieved, this, [this](const QByteArray &result){
        handleIssuesForFile(ResultParser::parseIssuesList(result));
    });
    connect(runner, &AxivionQueryRunner::finished, [runner]{ runner->deleteLater(); });
    runner->start();
}

void AxivionPluginPrivate::onDocumentClosed(Core::IDocument *doc)
{
    const auto document = qobject_cast<TextEditor::TextDocument *>(doc);
    if (!document)
        return;

    const TextEditor::TextMarks marks = document->marks();
    for (auto m : marks) {
        if (m->category().id == AxivionTextMarkId)
            delete m;
    }
}

void AxivionPluginPrivate::handleIssuesForFile(const IssuesList &issues)
{
    if (issues.issues.isEmpty())
        return;

    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    if (!project)
        return;

    const FilePath filePath = project->projectDirectory()
            .pathAppended(issues.issues.first().filePath);

    const Id axivionId(AxivionTextMarkId);
    for (const ShortIssue &issue : std::as_const(issues.issues)) {
        // FIXME the line location can be wrong (even the whole issue could be wrong)
        // depending on whether this line has been changed since the last axivion run and the
        // current state of the file - some magic has to happen here
        new AxivionTextMark(filePath, issue);
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

        connect(ProjectExplorer::ProjectManager::instance(),
                &ProjectExplorer::ProjectManager::startupProjectChanged,
                dd, &AxivionPluginPrivate::onStartupProjectChanged);
        connect(Core::EditorManager::instance(), &Core::EditorManager::documentOpened,
                dd, &AxivionPluginPrivate::onDocumentOpened);
        connect(Core::EditorManager::instance(), &Core::EditorManager::documentClosed,
                dd, &AxivionPluginPrivate::onDocumentClosed);
    }
};

} // Axivion::Internal

#include "axivionplugin.moc"
