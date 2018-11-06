/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "clangmodelmanagersupport.h"

#include "clangconstants.h"
#include "clangeditordocumentprocessor.h"
#include "clangutils.h"
#include "clangfollowsymbol.h"
#include "clanghoverhandler.h"
#include "clangprojectsettings.h"
#include "clangrefactoringengine.h"
#include "clangcurrentdocumentfilter.h"
#include "clangoverviewmodel.h"

#include <coreplugin/editormanager/editormanager.h>

#include <cpptools/cppcodemodelsettings.h>
#include <cpptools/cppfollowsymbolundercursor.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpptoolsreuse.h>
#include <cpptools/editordocumenthandle.h>
#include <cpptools/projectinfo.h>

#include <texteditor/quickfix.h>

#include <projectexplorer/project.h>
#include <projectexplorer/session.h>

#include <clangsupport/filecontainer.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QMenu>
#include <QTextBlock>
#include <QTimer>

using namespace ClangCodeModel;
using namespace ClangCodeModel::Internal;

static ClangModelManagerSupport *m_instance = 0;

static CppTools::CppModelManager *cppModelManager()
{
    return CppTools::CppModelManager::instance();
}

ClangModelManagerSupport::ClangModelManagerSupport()
    : m_completionAssistProvider(m_communicator)
    , m_followSymbol(new ClangFollowSymbol)
    , m_refactoringEngine(new RefactoringEngine)
{
    QTC_CHECK(!m_instance);
    m_instance = this;

    QApplication::instance()->installEventFilter(this);

    CppTools::CppModelManager::instance()->setCurrentDocumentFilter(
                std::make_unique<ClangCurrentDocumentFilter>());

    Core::EditorManager *editorManager = Core::EditorManager::instance();
    connect(editorManager, &Core::EditorManager::editorOpened,
            this, &ClangModelManagerSupport::onEditorOpened);
    connect(editorManager, &Core::EditorManager::currentEditorChanged,
            this, &ClangModelManagerSupport::onCurrentEditorChanged);
    connect(editorManager, &Core::EditorManager::editorsClosed,
            this, &ClangModelManagerSupport::onEditorClosed);

    CppTools::CppModelManager *modelManager = cppModelManager();
    connect(modelManager, &CppTools::CppModelManager::abstractEditorSupportContentsUpdated,
            this, &ClangModelManagerSupport::onAbstractEditorSupportContentsUpdated);
    connect(modelManager, &CppTools::CppModelManager::abstractEditorSupportRemoved,
            this, &ClangModelManagerSupport::onAbstractEditorSupportRemoved);
    connect(modelManager, &CppTools::CppModelManager::projectPartsUpdated,
            this, &ClangModelManagerSupport::onProjectPartsUpdated);
    connect(modelManager, &CppTools::CppModelManager::projectPartsRemoved,
            this, &ClangModelManagerSupport::onProjectPartsRemoved);

    auto *sessionManager = ProjectExplorer::SessionManager::instance();
    connect(sessionManager, &ProjectExplorer::SessionManager::projectAdded,
            this, &ClangModelManagerSupport::onProjectAdded);
    connect(sessionManager, &ProjectExplorer::SessionManager::aboutToRemoveProject,
            this, &ClangModelManagerSupport::onAboutToRemoveProject);

    CppTools::CppCodeModelSettings *settings = CppTools::codeModelSettings().data();
    connect(settings, &CppTools::CppCodeModelSettings::clangDiagnosticConfigsInvalidated,
            this, &ClangModelManagerSupport::onDiagnosticConfigsInvalidated);
}

ClangModelManagerSupport::~ClangModelManagerSupport()
{
    QTC_CHECK(m_projectSettings.isEmpty());
    m_instance = 0;
}

CppTools::CppCompletionAssistProvider *ClangModelManagerSupport::completionAssistProvider()
{
    return &m_completionAssistProvider;
}

TextEditor::BaseHoverHandler *ClangModelManagerSupport::createHoverHandler()
{
    return new Internal::ClangHoverHandler;
}

CppTools::FollowSymbolInterface &ClangModelManagerSupport::followSymbolInterface()
{
    return *m_followSymbol;
}

CppTools::RefactoringEngineInterface &ClangModelManagerSupport::refactoringEngineInterface()
{
    return *m_refactoringEngine;
}

std::unique_ptr<CppTools::AbstractOverviewModel> ClangModelManagerSupport::createOverviewModel()
{
    return std::make_unique<OverviewModel>();
}

void ClangModelManagerSupport::setBackendJobsPostponed(bool postponed)
{
    m_communicator.setBackendJobsPostponed(postponed);
}

CppTools::BaseEditorDocumentProcessor *ClangModelManagerSupport::createEditorDocumentProcessor(
        TextEditor::TextDocument *baseTextDocument)
{
    return new ClangEditorDocumentProcessor(m_communicator, baseTextDocument);
}

void ClangModelManagerSupport::onCurrentEditorChanged(Core::IEditor *editor)
{
    m_communicator.documentVisibilityChanged();

    // Update task hub issues for current CppEditorDocument
    ClangEditorDocumentProcessor::clearTaskHubIssues();
    if (!editor || !editor->document() || !cppModelManager()->isCppEditor(editor))
        return;

    const ::Utils::FileName filePath = editor->document()->filePath();
    if (auto processor = ClangEditorDocumentProcessor::get(filePath.toString()))
        processor->generateTaskHubIssues();
}

void ClangModelManagerSupport::connectTextDocumentToTranslationUnit(TextEditor::TextDocument *textDocument)
{
    // Handle externally changed documents
    connect(textDocument, &Core::IDocument::aboutToReload,
            this, &ClangModelManagerSupport::onCppDocumentAboutToReloadOnTranslationUnit,
            Qt::UniqueConnection);
    connect(textDocument, &Core::IDocument::reloadFinished,
            this, &ClangModelManagerSupport::onCppDocumentReloadFinishedOnTranslationUnit,
            Qt::UniqueConnection);

    // Handle changes from e.g. refactoring actions
    connectToTextDocumentContentsChangedForTranslationUnit(textDocument);
}

void ClangModelManagerSupport::connectTextDocumentToUnsavedFiles(TextEditor::TextDocument *textDocument)
{
    // Handle externally changed documents
    connect(textDocument, &Core::IDocument::aboutToReload,
            this, &ClangModelManagerSupport::onCppDocumentAboutToReloadOnUnsavedFile,
            Qt::UniqueConnection);
    connect(textDocument, &Core::IDocument::reloadFinished,
            this, &ClangModelManagerSupport::onCppDocumentReloadFinishedOnUnsavedFile,
            Qt::UniqueConnection);

    // Handle changes from e.g. refactoring actions
    connectToTextDocumentContentsChangedForUnsavedFile(textDocument);
}

void ClangModelManagerSupport::connectToTextDocumentContentsChangedForTranslationUnit(
        TextEditor::TextDocument *textDocument)
{
    connect(textDocument, &TextEditor::TextDocument::contentsChangedWithPosition,
            this, &ClangModelManagerSupport::onCppDocumentContentsChangedOnTranslationUnit,
            Qt::UniqueConnection);
}

void ClangModelManagerSupport::connectToTextDocumentContentsChangedForUnsavedFile(
        TextEditor::TextDocument *textDocument)
{
    connect(textDocument, &TextEditor::TextDocument::contentsChangedWithPosition,
            this, &ClangModelManagerSupport::onCppDocumentContentsChangedOnUnsavedFile,
            Qt::UniqueConnection);
}

void ClangModelManagerSupport::connectToWidgetsMarkContextMenuRequested(QWidget *editorWidget)
{
    const auto widget = qobject_cast<TextEditor::TextEditorWidget *>(editorWidget);
    if (widget) {
        connect(widget, &TextEditor::TextEditorWidget::markContextMenuRequested,
                this, &ClangModelManagerSupport::onTextMarkContextMenuRequested);
    }
}

bool ClangModelManagerSupport::eventFilter(QObject *obj, QEvent *e)
{
    if (obj == QApplication::instance() && e->type() == QEvent::ApplicationStateChange) {
        switch (QApplication::applicationState()) {
        case Qt::ApplicationInactive: setBackendJobsPostponed(true); break;
        case Qt::ApplicationActive: setBackendJobsPostponed(false); break;
        default:
            QTC_CHECK(false && "Unexpected Qt::ApplicationState");
        }
    }

    return false;
}

void ClangModelManagerSupport::onEditorOpened(Core::IEditor *editor)
{
    QTC_ASSERT(editor, return);
    Core::IDocument *document = editor->document();
    QTC_ASSERT(document, return);
    TextEditor::TextDocument *textDocument = qobject_cast<TextEditor::TextDocument *>(document);

    if (textDocument && cppModelManager()->isCppEditor(editor)) {
        connectTextDocumentToTranslationUnit(textDocument);
        connectToWidgetsMarkContextMenuRequested(editor->widget());

        // TODO: Ensure that not fully loaded documents are updated?
    }
}

void ClangModelManagerSupport::onEditorClosed(const QList<Core::IEditor *> &)
{
    m_communicator.documentVisibilityChanged();
}

void ClangModelManagerSupport::onCppDocumentAboutToReloadOnTranslationUnit()
{
    TextEditor::TextDocument *textDocument = qobject_cast<TextEditor::TextDocument *>(sender());
    disconnect(textDocument, &TextEditor::TextDocument::contentsChangedWithPosition,
               this, &ClangModelManagerSupport::onCppDocumentContentsChangedOnTranslationUnit);
}

void ClangModelManagerSupport::onCppDocumentReloadFinishedOnTranslationUnit(bool success)
{
    if (success) {
        TextEditor::TextDocument *textDocument = qobject_cast<TextEditor::TextDocument *>(sender());
        connectToTextDocumentContentsChangedForTranslationUnit(textDocument);
        m_communicator.documentsChangedWithRevisionCheck(textDocument);
    }
}

namespace {
void clearDiagnosticFixIts(const QString &filePath)
{
    auto processor = ClangEditorDocumentProcessor::get(filePath);
    if (processor)
        processor->clearDiagnosticsWithFixIts();
}
}

void ClangModelManagerSupport::onCppDocumentContentsChangedOnTranslationUnit(int position,
                                                                             int /*charsRemoved*/,
                                                                             int /*charsAdded*/)
{
    Core::IDocument *document = qobject_cast<Core::IDocument *>(sender());

    m_communicator.updateChangeContentStartPosition(document->filePath().toString(),
                                                       position);
    m_communicator.documentsChangedIfNotCurrentDocument(document);

    clearDiagnosticFixIts(document->filePath().toString());
}

void ClangModelManagerSupport::onCppDocumentAboutToReloadOnUnsavedFile()
{
    TextEditor::TextDocument *textDocument = qobject_cast<TextEditor::TextDocument *>(sender());
    disconnect(textDocument, &TextEditor::TextDocument::contentsChangedWithPosition,
               this, &ClangModelManagerSupport::onCppDocumentContentsChangedOnUnsavedFile);
}

void ClangModelManagerSupport::onCppDocumentReloadFinishedOnUnsavedFile(bool success)
{
    if (success) {
        TextEditor::TextDocument *textDocument = qobject_cast<TextEditor::TextDocument *>(sender());
        connectToTextDocumentContentsChangedForUnsavedFile(textDocument);
        m_communicator.unsavedFilesUpdated(textDocument);
    }
}

void ClangModelManagerSupport::onCppDocumentContentsChangedOnUnsavedFile()
{
    Core::IDocument *document = qobject_cast<Core::IDocument *>(sender());
    m_communicator.unsavedFilesUpdated(document);
}

void ClangModelManagerSupport::onAbstractEditorSupportContentsUpdated(const QString &filePath,
                                                                      const QByteArray &content)
{
    QTC_ASSERT(!filePath.isEmpty(), return);

    if (content.size() == 0)
        return; // Generation not yet finished.

    const QString mappedPath = m_uiHeaderOnDiskManager.write(filePath, content);
    m_communicator.unsavedFilesUpdated(mappedPath, content, 0);
}

void ClangModelManagerSupport::onAbstractEditorSupportRemoved(const QString &filePath)
{
    QTC_ASSERT(!filePath.isEmpty(), return);

    if (!cppModelManager()->cppEditorDocument(filePath)) {
        const QString mappedPath = m_uiHeaderOnDiskManager.remove(filePath);
        const QString projectPartId = Utils::projectPartIdForFile(filePath);
        m_communicator.unsavedFilesRemoved({{mappedPath, projectPartId}});
    }
}

void addFixItsActionsToMenu(QMenu *menu, const TextEditor::QuickFixOperations &fixItOperations)
{
    foreach (const auto &fixItOperation, fixItOperations) {
        QAction *action = menu->addAction(fixItOperation->description());
        QObject::connect(action, &QAction::triggered, [fixItOperation]() {
            fixItOperation->perform();
        });
    }
}

static int lineToPosition(const QTextDocument *textDocument, int lineNumber)
{
    QTC_ASSERT(textDocument, return 0);
    const QTextBlock textBlock = textDocument->findBlockByLineNumber(lineNumber);
    return textBlock.isValid() ? textBlock.position() - 1 : 0;
}

static TextEditor::AssistInterface createAssistInterface(TextEditor::TextEditorWidget *widget,
                                                         int lineNumber)
{
    return TextEditor::AssistInterface(widget->document(),
                                       lineToPosition(widget->document(), lineNumber),
                                       widget->textDocument()->filePath().toString(),
                                       TextEditor::IdleEditor);
}

void ClangModelManagerSupport::onTextMarkContextMenuRequested(TextEditor::TextEditorWidget *widget,
                                                              int lineNumber,
                                                              QMenu *menu)
{
    QTC_ASSERT(widget, return);
    QTC_ASSERT(lineNumber >= 1, return);
    QTC_ASSERT(menu, return);

    const auto filePath = widget->textDocument()->filePath().toString();
    ClangEditorDocumentProcessor *processor = ClangEditorDocumentProcessor::get(filePath);
    if (processor) {
        const auto assistInterface = createAssistInterface(widget, lineNumber);
        const auto fixItOperations = processor->extraRefactoringOperations(assistInterface);

        addFixItsActionsToMenu(menu, fixItOperations);
    }
}

using ClangEditorDocumentProcessors = QVector<ClangEditorDocumentProcessor *>;
static ClangEditorDocumentProcessors clangProcessors()
{
    ClangEditorDocumentProcessors result;
    foreach (auto *editorDocument, cppModelManager()->cppEditorDocuments())
        result.append(qobject_cast<ClangEditorDocumentProcessor *>(editorDocument->processor()));

    return result;
}

static ClangEditorDocumentProcessors
clangProcessorsWithProject(const ProjectExplorer::Project *project)
{
    return ::Utils::filtered(clangProcessors(), [project](ClangEditorDocumentProcessor *p) {
        return p->hasProjectPart() && p->projectPart()->project == project;
    });
}

static void updateProcessors(const ClangEditorDocumentProcessors &processors)
{
    CppTools::CppModelManager *modelManager = cppModelManager();
    for (ClangEditorDocumentProcessor *processor : processors)
        modelManager->cppEditorDocument(processor->filePath())->resetProcessor();
    modelManager->updateCppEditorDocuments(/*projectsUpdated=*/ false);
}

void ClangModelManagerSupport::onProjectAdded(ProjectExplorer::Project *project)
{
    QTC_ASSERT(!m_projectSettings.value(project), return);

    auto *settings = new Internal::ClangProjectSettings(project);
    connect(settings, &Internal::ClangProjectSettings::changed, [project]() {
        updateProcessors(clangProcessorsWithProject(project));
    });

    m_projectSettings.insert(project, settings);
}

void ClangModelManagerSupport::onAboutToRemoveProject(ProjectExplorer::Project *project)
{
    ClangProjectSettings * const settings = m_projectSettings.value(project);
    QTC_ASSERT(settings, return);
    m_projectSettings.remove(project);
    delete settings;
}

void ClangModelManagerSupport::onProjectPartsUpdated(ProjectExplorer::Project *project)
{
    QTC_ASSERT(project, return);
    const CppTools::ProjectInfo projectInfo = cppModelManager()->projectInfo(project);
    QTC_ASSERT(projectInfo.isValid(), return);

    QStringList projectPartIds;
    for (const CppTools::ProjectPart::Ptr &projectPart : projectInfo.projectParts())
        projectPartIds.append(projectPart->id());
    onProjectPartsRemoved(projectPartIds);
}

void ClangModelManagerSupport::onProjectPartsRemoved(const QStringList &projectPartIds)
{
    if (!projectPartIds.isEmpty())
        reinitializeBackendDocuments(projectPartIds);
}

static ClangEditorDocumentProcessors clangProcessorsWithDiagnosticConfig(
    const QVector<Core::Id> &configIds)
{
    return ::Utils::filtered(clangProcessors(), [configIds](ClangEditorDocumentProcessor *p) {
        return configIds.contains(p->diagnosticConfigId());
    });
}

void ClangModelManagerSupport::onDiagnosticConfigsInvalidated(const QVector<Core::Id> &configIds)
{
    updateProcessors(clangProcessorsWithDiagnosticConfig(configIds));
}

static ClangEditorDocumentProcessors
clangProcessorsWithProjectParts(const QStringList &projectPartIds)
{
    return ::Utils::filtered(clangProcessors(), [projectPartIds](ClangEditorDocumentProcessor *p) {
        return p->hasProjectPart() && projectPartIds.contains(p->projectPart()->id());
    });
}

void ClangModelManagerSupport::reinitializeBackendDocuments(const QStringList &projectPartIds)
{
    const auto processors = clangProcessorsWithProjectParts(projectPartIds);
    foreach (ClangEditorDocumentProcessor *processor, processors) {
        processor->closeBackendDocument();
        processor->clearProjectPart();
        processor->run();
    }
}

ClangModelManagerSupport *ClangModelManagerSupport::instance()
{
    return m_instance;
}

BackendCommunicator &ClangModelManagerSupport::communicator()
{
    return m_communicator;
}

QString ClangModelManagerSupport::dummyUiHeaderOnDiskPath(const QString &filePath) const
{
    return m_uiHeaderOnDiskManager.mapPath(filePath);
}

ClangProjectSettings &ClangModelManagerSupport::projectSettings(
    ProjectExplorer::Project *project) const
{
    return *m_projectSettings.value(project);
}

QString ClangModelManagerSupport::dummyUiHeaderOnDiskDirPath() const
{
    return m_uiHeaderOnDiskManager.directoryPath();
}

QString ClangModelManagerSupportProvider::id() const
{
    return QLatin1String(Constants::CLANG_MODELMANAGERSUPPORT_ID);
}

QString ClangModelManagerSupportProvider::displayName() const
{
    //: Display name
    return QCoreApplication::translate("ClangCodeModel::Internal::ModelManagerSupport",
                                       "Clang");
}

CppTools::ModelManagerSupport::Ptr ClangModelManagerSupportProvider::createModelManagerSupport()
{
    return CppTools::ModelManagerSupport::Ptr(new ClangModelManagerSupport);
}
