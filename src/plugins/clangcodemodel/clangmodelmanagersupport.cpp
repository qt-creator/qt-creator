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
#include <clangsupport/projectpartcontainer.h>
#include <clangsupport/projectpartsupdatedmessage.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QMenu>
#include <QTextBlock>

using namespace ClangCodeModel;
using namespace ClangCodeModel::Internal;

static ModelManagerSupportClang *m_instance = 0;

static CppTools::CppModelManager *cppModelManager()
{
    return CppTools::CppModelManager::instance();
}

ModelManagerSupportClang::ModelManagerSupportClang()
    : m_completionAssistProvider(m_communicator)
    , m_followSymbol(new ClangFollowSymbol)
    , m_refactoringEngine(new RefactoringEngine)
{
    QTC_CHECK(!m_instance);
    m_instance = this;

    CppTools::CppModelManager::instance()->setCurrentDocumentFilter(
                std::make_unique<ClangCurrentDocumentFilter>());

    Core::EditorManager *editorManager = Core::EditorManager::instance();
    connect(editorManager, &Core::EditorManager::editorOpened,
            this, &ModelManagerSupportClang::onEditorOpened);
    connect(editorManager, &Core::EditorManager::currentEditorChanged,
            this, &ModelManagerSupportClang::onCurrentEditorChanged);
    connect(editorManager, &Core::EditorManager::editorsClosed,
            this, &ModelManagerSupportClang::onEditorClosed);

    CppTools::CppModelManager *modelManager = cppModelManager();
    connect(modelManager, &CppTools::CppModelManager::abstractEditorSupportContentsUpdated,
            this, &ModelManagerSupportClang::onAbstractEditorSupportContentsUpdated);
    connect(modelManager, &CppTools::CppModelManager::abstractEditorSupportRemoved,
            this, &ModelManagerSupportClang::onAbstractEditorSupportRemoved);
    connect(modelManager, &CppTools::CppModelManager::projectPartsUpdated,
            this, &ModelManagerSupportClang::onProjectPartsUpdated);
    connect(modelManager, &CppTools::CppModelManager::projectPartsRemoved,
            this, &ModelManagerSupportClang::onProjectPartsRemoved);

    auto *sessionManager = ProjectExplorer::SessionManager::instance();
    connect(sessionManager, &ProjectExplorer::SessionManager::projectAdded,
            this, &ModelManagerSupportClang::onProjectAdded);
    connect(sessionManager, &ProjectExplorer::SessionManager::aboutToRemoveProject,
            this, &ModelManagerSupportClang::onAboutToRemoveProject);

    CppTools::CppCodeModelSettings *settings = CppTools::codeModelSettings().data();
    connect(settings, &CppTools::CppCodeModelSettings::clangDiagnosticConfigsInvalidated,
            this, &ModelManagerSupportClang::onDiagnosticConfigsInvalidated);

    m_communicator.projectPartsUpdatedForFallback();
}

ModelManagerSupportClang::~ModelManagerSupportClang()
{
    QTC_CHECK(m_projectSettings.isEmpty());
    m_instance = 0;
}

CppTools::CppCompletionAssistProvider *ModelManagerSupportClang::completionAssistProvider()
{
    return &m_completionAssistProvider;
}

TextEditor::BaseHoverHandler *ModelManagerSupportClang::createHoverHandler()
{
    return new Internal::ClangHoverHandler;
}

CppTools::FollowSymbolInterface &ModelManagerSupportClang::followSymbolInterface()
{
    return *m_followSymbol;
}

CppTools::RefactoringEngineInterface &ModelManagerSupportClang::refactoringEngineInterface()
{
    return *m_refactoringEngine;
}

std::unique_ptr<CppTools::AbstractOverviewModel> ModelManagerSupportClang::createOverviewModel()
{
    return std::make_unique<OverviewModel>();
}

CppTools::BaseEditorDocumentProcessor *ModelManagerSupportClang::createEditorDocumentProcessor(
        TextEditor::TextDocument *baseTextDocument)
{
    return new ClangEditorDocumentProcessor(m_communicator, baseTextDocument);
}

void ModelManagerSupportClang::onCurrentEditorChanged(Core::IEditor *editor)
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

void ModelManagerSupportClang::connectTextDocumentToTranslationUnit(TextEditor::TextDocument *textDocument)
{
    // Handle externally changed documents
    connect(textDocument, &Core::IDocument::aboutToReload,
            this, &ModelManagerSupportClang::onCppDocumentAboutToReloadOnTranslationUnit,
            Qt::UniqueConnection);
    connect(textDocument, &Core::IDocument::reloadFinished,
            this, &ModelManagerSupportClang::onCppDocumentReloadFinishedOnTranslationUnit,
            Qt::UniqueConnection);

    // Handle changes from e.g. refactoring actions
    connectToTextDocumentContentsChangedForTranslationUnit(textDocument);
}

void ModelManagerSupportClang::connectTextDocumentToUnsavedFiles(TextEditor::TextDocument *textDocument)
{
    // Handle externally changed documents
    connect(textDocument, &Core::IDocument::aboutToReload,
            this, &ModelManagerSupportClang::onCppDocumentAboutToReloadOnUnsavedFile,
            Qt::UniqueConnection);
    connect(textDocument, &Core::IDocument::reloadFinished,
            this, &ModelManagerSupportClang::onCppDocumentReloadFinishedOnUnsavedFile,
            Qt::UniqueConnection);

    // Handle changes from e.g. refactoring actions
    connectToTextDocumentContentsChangedForUnsavedFile(textDocument);
}

void ModelManagerSupportClang::connectToTextDocumentContentsChangedForTranslationUnit(
        TextEditor::TextDocument *textDocument)
{
    connect(textDocument, &TextEditor::TextDocument::contentsChangedWithPosition,
            this, &ModelManagerSupportClang::onCppDocumentContentsChangedOnTranslationUnit,
            Qt::UniqueConnection);
}

void ModelManagerSupportClang::connectToTextDocumentContentsChangedForUnsavedFile(
        TextEditor::TextDocument *textDocument)
{
    connect(textDocument, &TextEditor::TextDocument::contentsChangedWithPosition,
            this, &ModelManagerSupportClang::onCppDocumentContentsChangedOnUnsavedFile,
            Qt::UniqueConnection);
}

void ModelManagerSupportClang::connectToWidgetsMarkContextMenuRequested(QWidget *editorWidget)
{
    const auto widget = qobject_cast<TextEditor::TextEditorWidget *>(editorWidget);
    if (widget) {
        connect(widget, &TextEditor::TextEditorWidget::markContextMenuRequested,
                this, &ModelManagerSupportClang::onTextMarkContextMenuRequested);
    }
}

void ModelManagerSupportClang::onEditorOpened(Core::IEditor *editor)
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

void ModelManagerSupportClang::onEditorClosed(const QList<Core::IEditor *> &)
{
    m_communicator.documentVisibilityChanged();
}

void ModelManagerSupportClang::onCppDocumentAboutToReloadOnTranslationUnit()
{
    TextEditor::TextDocument *textDocument = qobject_cast<TextEditor::TextDocument *>(sender());
    disconnect(textDocument, &TextEditor::TextDocument::contentsChangedWithPosition,
               this, &ModelManagerSupportClang::onCppDocumentContentsChangedOnTranslationUnit);
}

void ModelManagerSupportClang::onCppDocumentReloadFinishedOnTranslationUnit(bool success)
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

void ModelManagerSupportClang::onCppDocumentContentsChangedOnTranslationUnit(int position,
                                                                             int /*charsRemoved*/,
                                                                             int /*charsAdded*/)
{
    Core::IDocument *document = qobject_cast<Core::IDocument *>(sender());

    m_communicator.updateChangeContentStartPosition(document->filePath().toString(),
                                                       position);
    m_communicator.documentsChangedIfNotCurrentDocument(document);

    clearDiagnosticFixIts(document->filePath().toString());
}

void ModelManagerSupportClang::onCppDocumentAboutToReloadOnUnsavedFile()
{
    TextEditor::TextDocument *textDocument = qobject_cast<TextEditor::TextDocument *>(sender());
    disconnect(textDocument, &TextEditor::TextDocument::contentsChangedWithPosition,
               this, &ModelManagerSupportClang::onCppDocumentContentsChangedOnUnsavedFile);
}

void ModelManagerSupportClang::onCppDocumentReloadFinishedOnUnsavedFile(bool success)
{
    if (success) {
        TextEditor::TextDocument *textDocument = qobject_cast<TextEditor::TextDocument *>(sender());
        connectToTextDocumentContentsChangedForUnsavedFile(textDocument);
        m_communicator.unsavedFilesUpdated(textDocument);
    }
}

void ModelManagerSupportClang::onCppDocumentContentsChangedOnUnsavedFile()
{
    Core::IDocument *document = qobject_cast<Core::IDocument *>(sender());
    m_communicator.unsavedFilesUpdated(document);
}

void ModelManagerSupportClang::onAbstractEditorSupportContentsUpdated(const QString &filePath,
                                                                      const QByteArray &content)
{
    QTC_ASSERT(!filePath.isEmpty(), return);

    const QString mappedPath = m_uiHeaderOnDiskManager.createIfNeeded(filePath);
    m_communicator.unsavedFilesUpdated(mappedPath, content, 0);
}

void ModelManagerSupportClang::onAbstractEditorSupportRemoved(const QString &filePath)
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

void ModelManagerSupportClang::onTextMarkContextMenuRequested(TextEditor::TextEditorWidget *widget,
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

void ModelManagerSupportClang::onProjectAdded(ProjectExplorer::Project *project)
{
    QTC_ASSERT(!m_projectSettings.value(project), return);

    auto *settings = new Internal::ClangProjectSettings(project);
    connect(settings, &Internal::ClangProjectSettings::changed, [project]() {
        updateProcessors(clangProcessorsWithProject(project));
    });

    m_projectSettings.insert(project, settings);
}

void ModelManagerSupportClang::onAboutToRemoveProject(ProjectExplorer::Project *project)
{
    ClangProjectSettings * const settings = m_projectSettings.value(project);
    QTC_ASSERT(settings, return);
    m_projectSettings.remove(project);
    delete settings;
}

void ModelManagerSupportClang::onProjectPartsUpdated(ProjectExplorer::Project *project)
{
    QTC_ASSERT(project, return);
    const CppTools::ProjectInfo projectInfo = cppModelManager()->projectInfo(project);
    QTC_ASSERT(projectInfo.isValid(), return);

    m_communicator.projectPartsUpdated(projectInfo.projectParts());
    m_communicator.projectPartsUpdatedForFallback();
}

void ModelManagerSupportClang::onProjectPartsRemoved(const QStringList &projectPartIds)
{
    if (!projectPartIds.isEmpty()) {
        closeBackendDocumentsWithProjectParts(projectPartIds);
        m_communicator.projectPartsRemoved(projectPartIds);
        m_communicator.projectPartsUpdatedForFallback();
    }
}

static ClangEditorDocumentProcessors clangProcessorsWithDiagnosticConfig(
    const QVector<Core::Id> &configIds)
{
    return ::Utils::filtered(clangProcessors(), [configIds](ClangEditorDocumentProcessor *p) {
        return configIds.contains(p->diagnosticConfigId());
    });
}

void ModelManagerSupportClang::onDiagnosticConfigsInvalidated(const QVector<Core::Id> &configIds)
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

void ModelManagerSupportClang::closeBackendDocumentsWithProjectParts(
        const QStringList &projectPartIds)
{
    const auto processors = clangProcessorsWithProjectParts(projectPartIds);
    foreach (ClangEditorDocumentProcessor *processor, processors) {
        processor->closeBackendDocument();
        processor->clearProjectPart();
        processor->run();
    }
}

ModelManagerSupportClang *ModelManagerSupportClang::instance()
{
    return m_instance;
}

BackendCommunicator &ModelManagerSupportClang::communicator()
{
    return m_communicator;
}

QString ModelManagerSupportClang::dummyUiHeaderOnDiskPath(const QString &filePath) const
{
    return m_uiHeaderOnDiskManager.mapPath(filePath);
}

ClangProjectSettings &ModelManagerSupportClang::projectSettings(
    ProjectExplorer::Project *project) const
{
    return *m_projectSettings.value(project);
}

QString ModelManagerSupportClang::dummyUiHeaderOnDiskDirPath() const
{
    return m_uiHeaderOnDiskManager.directoryPath();
}

QString ModelManagerSupportProviderClang::id() const
{
    return QLatin1String(Constants::CLANG_MODELMANAGERSUPPORT_ID);
}

QString ModelManagerSupportProviderClang::displayName() const
{
    //: Display name
    return QCoreApplication::translate("ClangCodeModel::Internal::ModelManagerSupport",
                                       "Clang");
}

CppTools::ModelManagerSupport::Ptr ModelManagerSupportProviderClang::createModelManagerSupport()
{
    return CppTools::ModelManagerSupport::Ptr(new ModelManagerSupportClang);
}
