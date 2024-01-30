// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "documentclangtoolrunner.h"

#include "clangtoolsconstants.h"
#include "clangtoolslogfilereader.h"
#include "clangtoolrunner.h"
#include "clangtoolsutils.h"
#include "diagnosticmark.h"
#include "executableinfo.h"
#include "virtualfilesystemoverlay.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <cppeditor/cppmodelmanager.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildtargettype.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <solutions/tasking/tasktree.h>

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/qtcassert.h>

#include <QLoggingCategory>
#include <QScopeGuard>

static Q_LOGGING_CATEGORY(LOG, "qtc.clangtools.cftr", QtWarningMsg)

using namespace Core;
using namespace CppEditor;
using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace ClangTools {
namespace Internal {

DocumentClangToolRunner::DocumentClangToolRunner(IDocument *document)
    : QObject(document)
    , m_document(document)
    , m_temporaryDir("clangtools-single-XXXXXX")
{
    m_runTimer.setInterval(500);
    m_runTimer.setSingleShot(true);

    connect(m_document, &IDocument::contentsChanged,
            this, &DocumentClangToolRunner::scheduleRun);
    connect(CppModelManager::instance(), &CppModelManager::projectPartsUpdated,
            this, &DocumentClangToolRunner::scheduleRun);
    connect(ClangToolsSettings::instance(), &ClangToolsSettings::changed,
            this, &DocumentClangToolRunner::scheduleRun);
    connect(&m_runTimer, &QTimer::timeout, this, &DocumentClangToolRunner::run);
    run();
}

DocumentClangToolRunner::~DocumentClangToolRunner()
{
    qDeleteAll(m_marks);
}

FilePath DocumentClangToolRunner::filePath() const
{
    return m_document->filePath();
}

Diagnostics DocumentClangToolRunner::diagnosticsAtLine(int lineNumber) const
{
    Diagnostics diagnostics;
    if (auto textDocument = qobject_cast<TextEditor::TextDocument *>(m_document)) {
        for (auto mark : textDocument->marksAt(lineNumber)) {
            if (mark->category().id == Constants::DIAGNOSTIC_MARK_ID)
                diagnostics << static_cast<DiagnosticMark *>(mark)->diagnostic();
        }
    }
    return diagnostics;
}

static void removeClangToolRefactorMarkers(TextEditor::TextEditorWidget *editor)
{
    if (!editor)
        return;
    editor->clearRefactorMarkers(Constants::CLANG_TOOL_FIXIT_AVAILABLE_MARKER_ID);
}

void DocumentClangToolRunner::scheduleRun()
{
    for (DiagnosticMark *mark : std::as_const(m_marks))
        mark->disable();
    for (TextEditor::TextEditorWidget *editor : std::as_const(m_editorsWithMarkers))
        removeClangToolRefactorMarkers(editor);
    m_runTimer.start();
}

static Project *findProject(const FilePath &file)
{
    Project *project = ProjectManager::projectForFile(file);
    return project ? project : ProjectManager::startupProject();
}

static VirtualFileSystemOverlay &vfso()
{
    static VirtualFileSystemOverlay overlay("clangtools-vfso-XXXXXX");
    return overlay;
}

static FileInfo getFileInfo(const FilePath &file, Project *project)
{
    const ProjectInfo::ConstPtr projectInfo = CppModelManager::projectInfo(project);
    if (!projectInfo)
        return {};

    FileInfo candidate;
    for (const ProjectPart::ConstPtr &projectPart : projectInfo->projectParts()) {
        QTC_ASSERT(projectPart, continue);

        for (const ProjectFile &projectFile : std::as_const(projectPart->files)) {
            QTC_ASSERT(projectFile.kind != ProjectFile::Unclassified, continue);
            QTC_ASSERT(projectFile.kind != ProjectFile::Unsupported, continue);
            if (projectFile.path == CppModelManager::configurationFileName())
                continue;
            if (file != projectFile.path)
                continue;
            if (!projectFile.active)
                continue;
            // found the best candidate, early return
            ProjectFile::Kind sourceKind = ProjectFile::sourceKind(projectFile.kind);
            if (projectPart->buildTargetType != BuildTargetType::Unknown)
                return FileInfo(projectFile.path, sourceKind, projectPart);
            // found something but keep looking for better candidates
            if (candidate.projectPart.isNull())
                candidate = FileInfo(projectFile.path, sourceKind, projectPart);
        }
    }

    return candidate;
}

static Environment projectBuildEnvironment(Project *project)
{
    Environment env;
    if (Target *target = project->activeTarget()) {
        if (BuildConfiguration *buildConfig = target->activeBuildConfiguration())
            env = buildConfig->environment();
    }
    if (!env.hasChanges())
        env = Environment::systemEnvironment();
    return env;
}

void DocumentClangToolRunner::run()
{
    if (m_projectSettingsUpdate)
        disconnect(m_projectSettingsUpdate);
    m_taskTree.reset();
    QScopeGuard cleanup([this] { finalize(); });

    auto isEditorForCurrentDocument = [this](const IEditor *editor) {
        return editor->document() == m_document;
    };
    if (!Utils::anyOf(EditorManager::visibleEditors(), isEditorForCurrentDocument)) {
        deleteLater();
        return;
    }
    const FilePath filePath = m_document->filePath();
    Project *project = findProject(filePath);
    if (!project)
        return;

    m_fileInfo = getFileInfo(filePath, project);
    if (!m_fileInfo.file.exists())
        return;

    const auto projectSettings = ClangToolsProjectSettings::getSettings(project);
    const RunSettings &runSettings = projectSettings->useGlobalSettings()
                                   ? ClangToolsSettings::instance()->runSettings()
                                   : projectSettings->runSettings();
    m_suppressed = projectSettings->suppressedDiagnostics();
    m_lastProjectDirectory = project->projectDirectory();
    m_projectSettingsUpdate = connect(projectSettings.data(), &ClangToolsProjectSettings::changed,
                                      this, &DocumentClangToolRunner::run);
    if (!runSettings.analyzeOpenFiles())
        return;

    vfso().update();
    const ClangDiagnosticConfig config = diagnosticConfig(runSettings.diagnosticConfigId());
    const Environment env = projectBuildEnvironment(project);
    QList<GroupItem> tasks{parallel};
    const auto addClangTool = [this, &runSettings, &config, &env, &tasks](ClangToolType tool) {
        if (!toolEnabled(tool, config, runSettings))
            return;
        if (!config.isEnabled(tool) && !runSettings.hasConfigFileForSourceFile(m_fileInfo.file))
            return;
        const FilePath executable = toolExecutable(tool);
        if (executable.isEmpty() || !executable.isExecutableFile())
            return;
        const auto [includeDir, clangVersion] = getClangIncludeDirAndVersion(executable);
        if (includeDir.isEmpty() || clangVersion.isEmpty())
            return;
        const AnalyzeUnit unit(m_fileInfo, includeDir, clangVersion);
        auto diagnosticFilter = [mappedPath = vfso().autoSavedFilePath(m_document)](
                                    const FilePath &path) { return path == mappedPath; };
        const AnalyzeInputData input{tool,
                                     runSettings,
                                     config,
                                     m_temporaryDir.path(),
                                     env,
                                     unit,
                                     vfso().overlayFilePath().toString(),
                                     diagnosticFilter};
        const auto setupHandler = [this, executable] {
            return !m_document->isModified() || isVFSOverlaySupported(executable);
        };
        const auto outputHandler = [this](const AnalyzeOutputData &output) { onDone(output); };
        tasks.append(Group{finishAllAndDone, clangToolTask(input, setupHandler, outputHandler)});
    };
    addClangTool(ClangToolType::Tidy);
    addClangTool(ClangToolType::Clazy);
    if (tasks.isEmpty())
        return;

    cleanup.dismiss();
    m_taskTree.reset(new TaskTree(tasks));
    connect(m_taskTree.get(), &TaskTree::done, this, &DocumentClangToolRunner::finalize);
    connect(m_taskTree.get(), &TaskTree::errorOccurred, this, &DocumentClangToolRunner::finalize);
    m_taskTree->start();
}

static void updateLocation(Debugger::DiagnosticLocation &location)
{
    location.filePath = vfso().originalFilePath(location.filePath);
}

void DocumentClangToolRunner::onDone(const AnalyzeOutputData &output)
{
    if (!output.success) {
        qCDebug(LOG) << "Failed to analyze " << m_fileInfo.file
                     << ":" << output.errorMessage << output.errorDetails;
        return;
    }

    Diagnostics diagnostics = output.diagnostics;
    for (Diagnostic &diag : diagnostics) {
        updateLocation(diag.location);
        for (ExplainingStep &explainingStep : diag.explainingSteps) {
            updateLocation(explainingStep.location);
            for (Debugger::DiagnosticLocation &rangeLocation : explainingStep.ranges)
                updateLocation(rangeLocation);
        }
    }

    const CppEditor::ClangToolType toolType = output.toolType;
    // remove outdated marks of the current runner
    const auto [toDelete, newMarks] = Utils::partition(m_marks, [toolType](DiagnosticMark *mark) {
        return mark->toolType == toolType;
    });
    m_marks = newMarks;
    qDeleteAll(toDelete);

    auto doc = qobject_cast<TextEditor::TextDocument *>(m_document);

    TextEditor::RefactorMarkers markers;

    for (const Diagnostic &diagnostic : std::as_const(diagnostics)) {
        if (isSuppressed(diagnostic))
            continue;

        auto mark = new DiagnosticMark(diagnostic, doc);
        mark->toolType = toolType;

        if (doc && Utils::anyOf(diagnostic.explainingSteps, &ExplainingStep::isFixIt)) {
            TextEditor::RefactorMarker marker;
            marker.tooltip = diagnostic.description;
            QTextCursor cursor(doc->document());
            cursor.setPosition(Text::positionInText(doc->document(),
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
            widget->setRefactorMarkers(markers, Constants::CLANG_TOOL_FIXIT_AVAILABLE_MARKER_ID);
            if (!m_editorsWithMarkers.contains(widget))
                m_editorsWithMarkers << widget;
        }
    }
}

void DocumentClangToolRunner::finalize()
{
    if (m_taskTree)
        m_taskTree.release()->deleteLater();
    // remove all disabled marks
    const auto [newMarks, toDelete] = Utils::partition(m_marks, &DiagnosticMark::enabled);
    m_marks = newMarks;
    qDeleteAll(toDelete);
}

bool DocumentClangToolRunner::isSuppressed(const Diagnostic &diagnostic) const
{
    auto equalsSuppressed = [this, &diagnostic](const SuppressedDiagnostic &suppressed) {
        if (suppressed.description != diagnostic.description)
            return false;
        FilePath filePath = suppressed.filePath;
        if (filePath.toFileInfo().isRelative())
            filePath = m_lastProjectDirectory.pathAppended(filePath.toString());
        return filePath == diagnostic.location.filePath;
    };
    return Utils::anyOf(m_suppressed, equalsSuppressed);
}

} // namespace Internal
} // namespace ClangTools
