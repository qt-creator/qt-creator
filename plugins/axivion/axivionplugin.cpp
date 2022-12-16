// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "axivionplugin.h"

#include "axivionoutputpane.h"
#include "axivionprojectsettings.h"
#include "axivionquery.h"
#include "axivionresultparser.h"
#include "axivionsettings.h"
#include "axivionsettingspage.h"
#include "axiviontr.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/session.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/textmark.h>
#include <utils/qtcassert.h>

#ifdef LICENSECHECKER
#  include <licensechecker/licensecheckerplugin.h>
#endif

#include <QMessageBox>
#include <QTimer>

namespace Axivion::Internal {

class AxivionPluginPrivate : public QObject
{
public:
    void fetchProjectInfo(const QString &projectName);
    void handleProjectInfo(const ProjectInfo &info);
    void onDocumentOpened(Core::IDocument *doc);
    void onDocumentClosed(Core::IDocument * doc);
    void handleIssuesForFile(const IssuesList &issues);

    AxivionSettings axivionSettings;
    AxivionSettingsPage axivionSettingsPage{&axivionSettings};
    AxivionOutputPane axivionOutputPane;
    QHash<ProjectExplorer::Project *, AxivionProjectSettings *> projectSettings;
    ProjectInfo currentProjectInfo;
    bool runningQuery = false;
};

static AxivionPlugin *s_instance = nullptr;
static AxivionPluginPrivate *dd = nullptr;

AxivionPlugin::AxivionPlugin()
{
    s_instance = this;
}

AxivionPlugin::~AxivionPlugin()
{
    if (!dd->projectSettings.isEmpty()) {
        qDeleteAll(dd->projectSettings);
        dd->projectSettings.clear();
    }
    delete dd;
    dd = nullptr;
}

AxivionPlugin *AxivionPlugin::instance()
{
    return s_instance;
}

bool AxivionPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

#ifdef LICENSECHECKER
    LicenseChecker::LicenseCheckerPlugin *licenseChecker
            = ExtensionSystem::PluginManager::getObject<LicenseChecker::LicenseCheckerPlugin>();

    if (!licenseChecker || !licenseChecker->hasValidLicense() || !licenseChecker->enterpriseFeatures())
        return true;
#endif // LICENSECHECKER

    dd = new AxivionPluginPrivate;
    dd->axivionSettings.fromSettings(Core::ICore::settings());

    auto panelFactory = new ProjectExplorer::ProjectPanelFactory;
    panelFactory->setPriority(250);
    panelFactory->setDisplayName(Tr::tr("Axivion"));
    panelFactory->setCreateWidgetFunction([](ProjectExplorer::Project *project){
        return new AxivionProjectSettingsWidget(project);
    });
    ProjectExplorer::ProjectPanelFactory::registerFactory(panelFactory);
    connect(ProjectExplorer::SessionManager::instance(),
            &ProjectExplorer::SessionManager::startupProjectChanged,
            this, &AxivionPlugin::onStartupProjectChanged);
    connect(Core::EditorManager::instance(), &Core::EditorManager::documentOpened,
            dd, &AxivionPluginPrivate::onDocumentOpened);
    connect(Core::EditorManager::instance(), &Core::EditorManager::documentClosed,
            dd, &AxivionPluginPrivate::onDocumentClosed);
    return true;
}

void AxivionPlugin::onStartupProjectChanged()
{
    QTC_ASSERT(dd, return);
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project) {
        dd->currentProjectInfo = ProjectInfo();
        dd->axivionOutputPane.updateDashboard();
        return;
    }

    const AxivionProjectSettings *projSettings = projectSettings(project);
    dd->fetchProjectInfo(projSettings->dashboardProjectName());
}

AxivionSettings *AxivionPlugin::settings()
{
    QTC_ASSERT(dd, return nullptr);
    return &dd->axivionSettings;
}

AxivionProjectSettings *AxivionPlugin::projectSettings(ProjectExplorer::Project *project)
{
    QTC_ASSERT(project, return nullptr);
    QTC_ASSERT(dd, return nullptr);

    auto &settings = dd->projectSettings[project];
    if (!settings)
        settings = new AxivionProjectSettings(project);
    return settings;
}

bool AxivionPlugin::handleCertificateIssue()
{
    QTC_ASSERT(dd, return false);

    const QString serverHost = QUrl(dd->axivionSettings.server.dashboard).host();
    if (QMessageBox::question(Core::ICore::dialogParent(), Tr::tr("Certificate Error"),
                              Tr::tr("Server certificate for %1 cannot be authenticated.\n"
                                     "Do you want to disable SSL verification for this server?\n"
                                     "Note: This can expose you to man-in-the-middle attack.")
                              .arg(serverHost))
            != QMessageBox::Yes) {
        return false;
    }
    dd->axivionSettings.server.validateCert = false;
    emit s_instance->settingsChanged();
    return true;
}

void AxivionPlugin::fetchProjectInfo(const QString &projectName)
{
    QTC_ASSERT(dd, return);
    dd->fetchProjectInfo(projectName);
}

ProjectInfo AxivionPlugin::projectInfo()
{
    QTC_ASSERT(dd, return {});
    return dd->currentProjectInfo;
}

void AxivionPluginPrivate::fetchProjectInfo(const QString &projectName)
{
    if (runningQuery) { // re-schedule
        QTimer::singleShot(3000, [this, projectName]{ fetchProjectInfo(projectName); });
        return;
    }
    if (projectName.isEmpty()) {
        currentProjectInfo = ProjectInfo();
        axivionOutputPane.updateDashboard();
        return;
    }
    runningQuery = true;

    AxivionQuery query(AxivionQuery::ProjectInfo, {projectName});
    AxivionQueryRunner *runner = new AxivionQueryRunner(query, this);
    connect(runner, &AxivionQueryRunner::resultRetrieved, this, [this](const QByteArray &result){
        handleProjectInfo(ResultParser::parseProjectInfo(result));
    });
    connect(runner, &AxivionQueryRunner::finished, [runner]{ runner->deleteLater(); });
    runner->start();
}

void AxivionPluginPrivate::handleProjectInfo(const ProjectInfo &info)
{
    runningQuery = false;
    if (!info.error.isEmpty()) {
        Core::MessageManager::writeFlashing("Axivion: " + info.error);
        return;
    }

    currentProjectInfo = info;
    axivionOutputPane.updateDashboard();

    if (currentProjectInfo.name.isEmpty())
        return;
    // FIXME handle already opened documents
}

void AxivionPluginPrivate::onDocumentOpened(Core::IDocument *doc)
{
    if (currentProjectInfo.name.isEmpty()) // we do not have a project info (yet)
        return;

    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!doc || !project->isKnownFile(doc->filePath()))
        return;

    Utils::FilePath relative = doc->filePath().relativeChildPath(project->projectDirectory());
    // for now only style violations
    AxivionQuery query(AxivionQuery::IssuesForFileList, {currentProjectInfo.name, "SV",
                                                         relative.path() } );
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
    auto axivionId = Utils::Id("AxivionTextMark");
    for (auto m : marks) {
        if (m->category() == axivionId)
            delete m;
    }
}

void AxivionPluginPrivate::handleIssuesForFile(const IssuesList &issues)
{
    if (issues.issues.isEmpty())
        return;

    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project)
        return;

    const Utils::FilePath filePath = project->projectDirectory()
            .pathAppended(issues.issues.first().filePath);

    auto axivionId = Utils::Id("AxivionTextMark");
    for (const ShortIssue &issue : std::as_const(issues.issues)) {
        // FIXME the line location can be wrong (even the whole issue could be wrong)
        // depending on whether this line has been changed since the last axivion run and the
        // current state of the file - some magic has to happen here
        auto mark = new TextEditor::TextMark(filePath, issue.lineNumber, axivionId);
        mark->setToolTip(issue.errorNumber + " " + issue.entity + ": " + issue.message);
        mark->setPriority(TextEditor::TextMark::NormalPriority);
        mark->setLineAnnotation(issue.errorNumber);
    }
}

} // Axivion::Internal
