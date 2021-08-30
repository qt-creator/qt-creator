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

#pragma once

#include "clangcompletionassistprovider.h"
#include "clanguiheaderondiskmanager.h"

#include <cppeditor/cppmodelmanagersupport.h>
#include <cppeditor/projectinfo.h>

#include <utils/futuresynchronizer.h>
#include <utils/id.h>

#include <QObject>

#include <memory>

QT_BEGIN_NAMESPACE
class QMenu;
class QWidget;
QT_END_NAMESPACE

namespace TextEditor { class TextEditorWidget; }
namespace CppEditor {
class FollowSymbolInterface;
class RefactoringEngineInterface;
} // namespace CppEditor

namespace ClangCodeModel {
namespace Internal {

class ClangdClient;
class ClangProjectSettings;

class ClangModelManagerSupport:
        public QObject,
        public CppEditor::ModelManagerSupport
{
    Q_OBJECT

public:
    ClangModelManagerSupport();
    ~ClangModelManagerSupport() override;

    CppEditor::CppCompletionAssistProvider *completionAssistProvider() override;
    CppEditor::CppCompletionAssistProvider *functionHintAssistProvider() override;
    TextEditor::BaseHoverHandler *createHoverHandler() override;
    CppEditor::BaseEditorDocumentProcessor *createEditorDocumentProcessor(
                TextEditor::TextDocument *baseTextDocument) override;
    CppEditor::FollowSymbolInterface &followSymbolInterface() override;
    CppEditor::RefactoringEngineInterface &refactoringEngineInterface() override;
    std::unique_ptr<CppEditor::AbstractOverviewModel> createOverviewModel() override;
    bool supportsOutline(const TextEditor::TextDocument *document) const override;

    BackendCommunicator &communicator();
    QString dummyUiHeaderOnDiskDirPath() const;
    QString dummyUiHeaderOnDiskPath(const QString &filePath) const;

    ClangProjectSettings &projectSettings(ProjectExplorer::Project *project) const;

    ClangdClient *clientForProject(const ProjectExplorer::Project *project) const;
    ClangdClient *clientForFile(const Utils::FilePath &file) const;

    static ClangModelManagerSupport *instance();

signals:
    void createdClient(ClangdClient *client);

private:
    void onEditorOpened(Core::IEditor *editor);
    void onEditorClosed(const QList<Core::IEditor *> &editors);
    void onCurrentEditorChanged(Core::IEditor *newCurrent);
    void onCppDocumentAboutToReloadOnTranslationUnit();
    void onCppDocumentReloadFinishedOnTranslationUnit(bool success);
    void onCppDocumentContentsChangedOnTranslationUnit(int position,
                                                       int charsRemoved,
                                                       int charsAdded);
    void onCppDocumentAboutToReloadOnUnsavedFile();
    void onCppDocumentReloadFinishedOnUnsavedFile(bool success);
    void onCppDocumentContentsChangedOnUnsavedFile();

    void onAbstractEditorSupportContentsUpdated(const QString &filePath,
                                                const QString &sourceFilePath,
                                                const QByteArray &content);
    void onAbstractEditorSupportRemoved(const QString &filePath);

    void onTextMarkContextMenuRequested(TextEditor::TextEditorWidget *widget,
                                        int lineNumber,
                                        QMenu *menu);

    void onProjectAdded(ProjectExplorer::Project *project);
    void onAboutToRemoveProject(ProjectExplorer::Project *project);

    void onProjectPartsUpdated(ProjectExplorer::Project *project);
    void onProjectPartsRemoved(const QStringList &projectPartIds);
    void onClangdSettingsChanged();

    void onDiagnosticConfigsInvalidated(const QVector<::Utils::Id> &configIds);

    void reinitializeBackendDocuments(const QStringList &projectPartIds);

    void connectTextDocumentToTranslationUnit(TextEditor::TextDocument *textDocument);
    void connectTextDocumentToUnsavedFiles(TextEditor::TextDocument *textDocument);
    void connectToTextDocumentContentsChangedForTranslationUnit(
            TextEditor::TextDocument *textDocument);
    void connectToTextDocumentContentsChangedForUnsavedFile(TextEditor::TextDocument *textDocument);
    void connectToWidgetsMarkContextMenuRequested(QWidget *editorWidget);

    void updateLanguageClient(ProjectExplorer::Project *project,
                              const CppEditor::ProjectInfo::ConstPtr &projectInfo);
    ClangdClient *createClient(ProjectExplorer::Project *project, const Utils::FilePath &jsonDbDir);
    void claimNonProjectSources(ClangdClient *fallbackClient);

private:
    UiHeaderOnDiskManager m_uiHeaderOnDiskManager;
    BackendCommunicator m_communicator;
    ClangCompletionAssistProvider m_completionAssistProvider;
    ClangCompletionAssistProvider m_functionHintAssistProvider;
    std::unique_ptr<CppEditor::FollowSymbolInterface> m_followSymbol;
    std::unique_ptr<CppEditor::RefactoringEngineInterface> m_refactoringEngine;

    QHash<ProjectExplorer::Project *, ClangProjectSettings *> m_projectSettings;
    Utils::FutureSynchronizer m_generatorSynchronizer;
};

class ClangModelManagerSupportProvider : public CppEditor::ModelManagerSupportProvider
{
public:
    QString id() const override;
    QString displayName() const override;

    CppEditor::ModelManagerSupport::Ptr createModelManagerSupport() override;
};

} // namespace Internal
} // namespace ClangCodeModel
