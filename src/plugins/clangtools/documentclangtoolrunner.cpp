/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "documentclangtoolrunner.h"

#include "clangfileinfo.h"
#include "clangfixitsrefactoringchanges.h"
#include "clangtidyclazyrunner.h"
#include "clangtoolruncontrol.h"
#include "clangtoolsconstants.h"
#include "clangtoolsprojectsettings.h"
#include "clangtoolsutils.h"
#include "diagnosticmark.h"
#include "executableinfo.h"
#include "virtualfilesystemoverlay.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <cpptools/cppmodelmanager.h>
#include <projectexplorer/buildtargettype.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/textmark.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QLoggingCategory>

static Q_LOGGING_CATEGORY(LOG, "qtc.clangtools.cftr", QtWarningMsg)

namespace ClangTools {
namespace Internal {

DocumentClangToolRunner::DocumentClangToolRunner(Core::IDocument *document)
    : QObject(document)
    , m_document(document)
    , m_temporaryDir("clangtools-single-XXXXXX")
{
    using namespace CppTools;

    m_runTimer.setInterval(500);
    m_runTimer.setSingleShot(true);

    connect(m_document,
            &Core::IDocument::contentsChanged,
            this,
            &DocumentClangToolRunner::scheduleRun);
    connect(CppModelManager::instance(),
            &CppModelManager::projectPartsUpdated,
            this,
            &DocumentClangToolRunner::scheduleRun);
    connect(ClangToolsSettings::instance(),
            &ClangToolsSettings::changed,
            this,
            &DocumentClangToolRunner::scheduleRun);
    connect(&m_runTimer, &QTimer::timeout, this, &DocumentClangToolRunner::run);
    run();
}

DocumentClangToolRunner::~DocumentClangToolRunner()
{
    cancel();
    qDeleteAll(m_marks);
}

Utils::FilePath DocumentClangToolRunner::filePath() const
{
    return m_document->filePath();
}

Diagnostics DocumentClangToolRunner::diagnosticsAtLine(int lineNumber) const
{
    Diagnostics diagnostics;
    if (auto textDocument = qobject_cast<TextEditor::TextDocument *>(m_document)) {
        for (auto mark : textDocument->marksAt(lineNumber)) {
            if (mark->category() == Constants::DIAGNOSTIC_MARK_ID)
                diagnostics << static_cast<DiagnosticMark *>(mark)->diagnostic();
        }
    }
    return diagnostics;
}

static void removeClangToolRefactorMarkers(TextEditor::TextEditorWidget *editor)
{
    if (!editor)
        return;
    editor->setRefactorMarkers(
        TextEditor::RefactorMarker::filterOutType(editor->refactorMarkers(),
                                                  Constants::CLANG_TOOL_FIXIT_AVAILABLE_MARKER_ID));
}

void DocumentClangToolRunner::scheduleRun()
{
    for (DiagnosticMark *mark : qAsConst(m_marks))
        mark->disable();
    for (TextEditor::TextEditorWidget *editor : qAsConst(m_editorsWithMarkers))
        removeClangToolRefactorMarkers(editor);
    m_runTimer.start();
}

static ProjectExplorer::Project *findProject(const Utils::FilePath &file)
{
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::projectForFile(file);
    return project ? project : ProjectExplorer::SessionManager::startupProject();
}

static VirtualFileSystemOverlay &vfso()
{
    static VirtualFileSystemOverlay overlay("clangtools-vfso-XXXXXX");
    return overlay;
}

static FileInfo getFileInfo(const Utils::FilePath &file, ProjectExplorer::Project *project)
{
    CppTools::ProjectInfo projectInfo = CppTools::CppModelManager::instance()->projectInfo(project);
    if (!projectInfo.isValid())
        return {};

    FileInfo candidate;
    for (const CppTools::ProjectPart::Ptr &projectPart : projectInfo.projectParts()) {
        QTC_ASSERT(projectPart, continue);

        for (const CppTools::ProjectFile &projectFile : qAsConst(projectPart->files)) {
            QTC_ASSERT(projectFile.kind != CppTools::ProjectFile::Unclassified, continue);
            QTC_ASSERT(projectFile.kind != CppTools::ProjectFile::Unsupported, continue);
            if (projectFile.path == CppTools::CppModelManager::configurationFileName())
                continue;
            const auto projectFilePath = Utils::FilePath::fromString(projectFile.path);
            if (file != projectFilePath)
                continue;
            if (!projectFile.active)
                continue;
            // found the best candidate, early return
            if (projectPart->buildTargetType != ProjectExplorer::BuildTargetType::Unknown)
                return FileInfo(projectFilePath, projectFile.kind, projectPart);
            // found something but keep looking for better candidates
            if (candidate.projectPart.isNull())
                candidate = FileInfo(projectFilePath, projectFile.kind, projectPart);
        }
    }

    return candidate;
}

static Utils::Environment projectBuildEnvironment(ProjectExplorer::Project *project)
{
    Utils::Environment env;
    if (ProjectExplorer::Target *target = project->activeTarget()) {
        if (ProjectExplorer::BuildConfiguration *buildConfig = target->activeBuildConfiguration())
            env = buildConfig->environment();
    }
    if (env.size() == 0)
        env = Utils::Environment::systemEnvironment();
    return env;
}

void DocumentClangToolRunner::run()
{
    cancel();
    auto isEditorForCurrentDocument = [this](const Core::IEditor *editor) {
        return editor->document() == m_document;
    };
    if (Utils::anyOf(Core::EditorManager::visibleEditors(), isEditorForCurrentDocument)) {
        const Utils::FilePath filePath = m_document->filePath();
        if (ProjectExplorer::Project *project = findProject(filePath)) {
            m_fileInfo = getFileInfo(filePath, project);
            if (m_fileInfo.file.exists()) {
                const auto projectSettings = ClangToolsProjectSettings::getSettings(project);

                const RunSettings &runSettings = projectSettings->useGlobalSettings()
                                                     ? ClangToolsSettings::instance()->runSettings()
                                                     : projectSettings->runSettings();

                m_suppressed = projectSettings->suppressedDiagnostics();
                m_lastProjectDirectory = project->projectDirectory();
                m_projectSettingsUpdate = connect(projectSettings.data(),
                                                  &ClangToolsProjectSettings::changed,
                                                  this,
                                                  &DocumentClangToolRunner::run);

                if (runSettings.analyzeOpenFiles()) {
                    vfso().update();

                    CppTools::ClangDiagnosticConfig config = diagnosticConfig(
                        runSettings.diagnosticConfigId());

                    Utils::Environment env = projectBuildEnvironment(project);
                    if (config.isClangTidyEnabled()) {
                        m_runnerCreators << [this, env, config]() {
                            return createRunner<ClangTidyRunner>(config, env);
                        };
                    }
                    if (config.isClazyEnabled()) {
                        m_runnerCreators << [this, env, config]() {
                            return createRunner<ClazyStandaloneRunner>(config, env);
                        };
                    }
                }
            }
        }
    } else {
        deleteLater();
    }

    runNext();
}

QPair<Utils::FilePath, QString> getClangIncludeDirAndVersion(ClangToolRunner *runner)
{
    static QMap<Utils::FilePath, QPair<Utils::FilePath, QString>> cache;
    const Utils::FilePath tool = Utils::FilePath::fromString(runner->executable());
    auto it = cache.find(tool);
    if (it == cache.end())
        it = cache.insert(tool, getClangIncludeDirAndVersion(tool));
    return it.value();
}

void DocumentClangToolRunner::runNext()
{
    m_currentRunner.reset(m_runnerCreators.isEmpty() ? nullptr : m_runnerCreators.takeFirst()());
    if (m_currentRunner) {
        auto [clangIncludeDir, clangVersion] = getClangIncludeDirAndVersion(m_currentRunner.get());
        qCDebug(LOG) << Q_FUNC_INFO << m_currentRunner->executable() << clangIncludeDir
                     << clangVersion << m_fileInfo.file;
        if (m_currentRunner->executable().isEmpty() || clangIncludeDir.isEmpty() || clangVersion.isEmpty()
            || (m_document->isModified() && !m_currentRunner->supportsVFSOverlay())) {
            runNext();
        } else {
            AnalyzeUnit unit(m_fileInfo, clangIncludeDir, clangVersion);
            QTC_ASSERT(Utils::FilePath::fromString(unit.file).exists(), runNext(); return;);
            m_currentRunner->setVFSOverlay(vfso().overlayFilePath().toString());
            if (!m_currentRunner->run(unit.file, unit.arguments))
                runNext();
        }
    } else {
        finalize();
    }
}

static void updateLocation(Debugger::DiagnosticLocation &location)
{
    location.filePath = vfso().originalFilePath(Utils::FilePath::fromString(location.filePath)).toString();
}

void DocumentClangToolRunner::onSuccess()
{
    QString errorMessage;
    Utils::FilePath mappedPath = vfso().autoSavedFilePath(m_document);
    Diagnostics diagnostics = readExportedDiagnostics(
        Utils::FilePath::fromString(m_currentRunner->outputFilePath()),
        [&](const Utils::FilePath &path) { return path == mappedPath; },
        &errorMessage);

    for (Diagnostic &diag : diagnostics) {
        updateLocation(diag.location);
        for (ExplainingStep &explainingStep : diag.explainingSteps) {
            updateLocation(explainingStep.location);
            for (Debugger::DiagnosticLocation &rangeLocation : explainingStep.ranges)
                updateLocation(rangeLocation);
        }
    }

    // remove outdated marks of the current runner
    auto [toDelete, newMarks] = Utils::partition(m_marks, [this](DiagnosticMark *mark) {
        return mark->source == m_currentRunner->name();
    });
    m_marks = newMarks;
    qDeleteAll(toDelete);

    auto doc = qobject_cast<TextEditor::TextDocument *>(m_document);

    TextEditor::RefactorMarkers markers;

    for (const Diagnostic &diagnostic : diagnostics) {
        if (isSuppressed(diagnostic))
            continue;

        auto mark = new DiagnosticMark(diagnostic);
        mark->source = m_currentRunner->name();

        if (doc && Utils::anyOf(diagnostic.explainingSteps, &ExplainingStep::isFixIt)) {
            TextEditor::RefactorMarker marker;
            marker.tooltip = diagnostic.description;
            QTextCursor cursor(doc->document());
            cursor.setPosition(Utils::Text::positionInText(doc->document(),
                                                           diagnostic.location.line,
                                                           diagnostic.location.column));
            cursor.movePosition(QTextCursor::EndOfLine);
            marker.cursor = cursor;
            marker.type = Constants::CLANG_TOOL_FIXIT_AVAILABLE_MARKER_ID;
            marker.callback = [marker](TextEditor::TextEditorWidget *editor) {
                editor->setTextCursor(marker.cursor);
                editor->invokeAssist(TextEditor::QuickFix);
            };
            markers << marker;
        }

        m_marks << mark;
    }

    for (auto editor : TextEditor::BaseTextEditor::textEditorsForDocument(doc)) {
        if (TextEditor::TextEditorWidget *widget = editor->editorWidget()) {
            widget->setRefactorMarkers(markers + widget->refactorMarkers());
            if (!m_editorsWithMarkers.contains(widget))
                m_editorsWithMarkers << widget;
        }
    }

    runNext();
}

void DocumentClangToolRunner::onFailure(const QString &errorMessage, const QString &errorDetails)
{
    qCDebug(LOG) << "Failed to analyze " << m_fileInfo.file << ":" << errorMessage << errorDetails;
    runNext();
}

void DocumentClangToolRunner::finalize()
{
    // remove all disabled textMarks
    auto [newMarks, toDelete] = Utils::partition(m_marks, &DiagnosticMark::enabled);
    m_marks = newMarks;
    qDeleteAll(toDelete);
}

void DocumentClangToolRunner::cancel()
{
    if (m_projectSettingsUpdate)
        disconnect(m_projectSettingsUpdate);
    m_runnerCreators.clear();
    if (m_currentRunner) {
        m_currentRunner->disconnect(this);
        m_currentRunner.reset(nullptr);
    }
}

bool DocumentClangToolRunner::isSuppressed(const Diagnostic &diagnostic) const
{
    auto equalsSuppressed = [this, &diagnostic](const SuppressedDiagnostic &suppressed) {
        if (suppressed.description != diagnostic.description)
            return false;
        QString filePath = suppressed.filePath.toString();
        QFileInfo fi(filePath);
        if (fi.isRelative())
            filePath = m_lastProjectDirectory.toString() + QLatin1Char('/') + filePath;
        return filePath == diagnostic.location.filePath;
    };
    return Utils::anyOf(m_suppressed, equalsSuppressed);
}

const CppTools::ClangDiagnosticConfig DocumentClangToolRunner::getDiagnosticConfig(ProjectExplorer::Project *project)
{
    const auto projectSettings = ClangToolsProjectSettings::getSettings(project);
    m_projectSettingsUpdate = connect(projectSettings.data(),
                                      &ClangToolsProjectSettings::changed,
                                      this,
                                      &DocumentClangToolRunner::run);

    const Utils::Id &id = projectSettings->useGlobalSettings()
                              ? ClangToolsSettings::instance()->runSettings().diagnosticConfigId()
                              : projectSettings->runSettings().diagnosticConfigId();
    return diagnosticConfig(id);
}

template<class T>
ClangToolRunner *DocumentClangToolRunner::createRunner(const CppTools::ClangDiagnosticConfig &config,
                                                       const Utils::Environment &env)
{
    auto runner = new T(config, this);
    runner->init(m_temporaryDir.path(), env);
    connect(runner, &ClangToolRunner::finishedWithSuccess,
            this, &DocumentClangToolRunner::onSuccess);
    connect(runner, &ClangToolRunner::finishedWithFailure,
            this, &DocumentClangToolRunner::onFailure);
    return runner;
}

} // namespace Internal
} // namespace ClangTools
