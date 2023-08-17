// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "axivionplugin.h"

#include "axivionoutputpane.h"
#include "axivionprojectsettings.h"
#include "axivionquery.h"
#include "axivionresultparser.h"
#include "axiviontr.h"

#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/messagemanager.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectpanelfactory.h>

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/textmark.h>

#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QTimer>

constexpr char AxivionTextMarkId[] = "AxivionTextMark";

namespace Axivion::Internal {

class AxivionPluginPrivate : public QObject
{
public:
    void onStartupProjectChanged();
    void fetchProjectInfo(const QString &projectName);
    void handleProjectInfo(const ProjectInfo &info);
    void handleOpenedDocs(ProjectExplorer::Project *project);
    void onDocumentOpened(Core::IDocument *doc);
    void onDocumentClosed(Core::IDocument * doc);
    void clearAllMarks();
    void handleIssuesForFile(const IssuesList &issues);
    void fetchRuleInfo(const QString &id);

    AxivionOutputPane m_axivionOutputPane;
    ProjectInfo m_currentProjectInfo;
    bool m_runningQuery = false;
};

static AxivionPluginPrivate *dd = nullptr;

class AxivionTextMark : public TextEditor::TextMark
{
public:
    AxivionTextMark(const Utils::FilePath &filePath, const ShortIssue &issue);

private:
    QString m_id;
};

AxivionTextMark::AxivionTextMark(const Utils::FilePath &filePath, const ShortIssue &issue)
    : TextEditor::TextMark(filePath, issue.lineNumber, {Tr::tr("Axivion"), AxivionTextMarkId})
    , m_id(issue.id)
{
    const QString markText = issue.entity.isEmpty() ? issue.message
                                                    : issue.entity + ": " + issue.message;
    setToolTip(issue.errorNumber + " " + markText);
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

AxivionPlugin::~AxivionPlugin()
{
    AxivionProjectSettings::destroyProjectSettings();
    delete dd;
    dd = nullptr;
}

void AxivionPlugin::initialize()
{
    dd = new AxivionPluginPrivate;

    auto panelFactory = new ProjectExplorer::ProjectPanelFactory;
    panelFactory->setPriority(250);
    panelFactory->setDisplayName(Tr::tr("Axivion"));
    panelFactory->setCreateWidgetFunction([](ProjectExplorer::Project *project){
        return new AxivionProjectSettingsWidget(project);
    });
    ProjectExplorer::ProjectPanelFactory::registerFactory(panelFactory);
    connect(ProjectExplorer::ProjectManager::instance(),
            &ProjectExplorer::ProjectManager::startupProjectChanged,
            dd, &AxivionPluginPrivate::onStartupProjectChanged);
    connect(Core::EditorManager::instance(), &Core::EditorManager::documentOpened,
            dd, &AxivionPluginPrivate::onDocumentOpened);
    connect(Core::EditorManager::instance(), &Core::EditorManager::documentClosed,
            dd, &AxivionPluginPrivate::onDocumentClosed);
}

void AxivionPlugin::fetchProjectInfo(const QString &projectName)
{
    QTC_ASSERT(dd, return);
    dd->fetchProjectInfo(projectName);
}

ProjectInfo AxivionPlugin::projectInfo()
{
    QTC_ASSERT(dd, return {});
    return dd->m_currentProjectInfo;
}

void AxivionPluginPrivate::onStartupProjectChanged()
{
    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    if (!project) {
        clearAllMarks();
        m_currentProjectInfo = ProjectInfo();
        m_axivionOutputPane.updateDashboard();
        return;
    }

    const AxivionProjectSettings *projSettings = AxivionProjectSettings::projectSettings(project);
    fetchProjectInfo(projSettings->dashboardProjectName());
}

void AxivionPluginPrivate::fetchProjectInfo(const QString &projectName)
{
    if (m_runningQuery) { // re-schedule
        QTimer::singleShot(3000, this, [this, projectName] { fetchProjectInfo(projectName); });
        return;
    }
    clearAllMarks();
    if (projectName.isEmpty()) {
        m_currentProjectInfo = ProjectInfo();
        m_axivionOutputPane.updateDashboard();
        return;
    }
    m_runningQuery = true;

    AxivionQuery query(AxivionQuery::ProjectInfo, {projectName});
    AxivionQueryRunner *runner = new AxivionQueryRunner(query, this);
    connect(runner, &AxivionQueryRunner::resultRetrieved, this, [this](const QByteArray &result){
        handleProjectInfo(ResultParser::parseProjectInfo(result));
    });
    connect(runner, &AxivionQueryRunner::finished, [runner]{ runner->deleteLater(); });
    runner->start();
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

void AxivionPluginPrivate::handleProjectInfo(const ProjectInfo &info)
{
    m_runningQuery = false;
    if (!info.error.isEmpty()) {
        Core::MessageManager::writeFlashing("Axivion: " + info.error);
        return;
    }

    m_currentProjectInfo = info;
    m_axivionOutputPane.updateDashboard();

    if (m_currentProjectInfo.name.isEmpty())
        return;

    // handle already opened documents
    if (auto buildSystem = ProjectExplorer::ProjectManager::startupBuildSystem();
            !buildSystem || !buildSystem->isParsing()) {
        handleOpenedDocs(nullptr);
    } else {
        connect(ProjectExplorer::ProjectManager::instance(),
                &ProjectExplorer::ProjectManager::projectFinishedParsing,
                this, &AxivionPluginPrivate::handleOpenedDocs);
    }
}

void AxivionPluginPrivate::onDocumentOpened(Core::IDocument *doc)
{
    if (m_currentProjectInfo.name.isEmpty()) // we do not have a project info (yet)
        return;

    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    if (!doc || !project->isKnownFile(doc->filePath()))
        return;

    Utils::FilePath relative = doc->filePath().relativeChildPath(project->projectDirectory());
    // for now only style violations
    AxivionQuery query(AxivionQuery::IssuesForFileList, {m_currentProjectInfo.name, "SV",
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

    const Utils::FilePath filePath = project->projectDirectory()
            .pathAppended(issues.issues.first().filePath);

    const Utils::Id axivionId(AxivionTextMarkId);
    for (const ShortIssue &issue : std::as_const(issues.issues)) {
        // FIXME the line location can be wrong (even the whole issue could be wrong)
        // depending on whether this line has been changed since the last axivion run and the
        // current state of the file - some magic has to happen here
        new AxivionTextMark(filePath, issue);
    }
}

} // Axivion::Internal
