// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cppeditor/cppmodelmanagersupport.h>
#include <cppeditor/projectinfo.h>

#include <utils/filepath.h>
#include <utils/futuresynchronizer.h>
#include <utils/id.h>

#include <QHash>
#include <QObject>
#include <QPointer>

#include <memory>

QT_BEGIN_NAMESPACE
class QMenu;
class QTimer;
class QWidget;
QT_END_NAMESPACE

namespace Core { class IEditor; }
namespace CppEditor { class RefactoringEngineInterface; }
namespace LanguageClient { class Client; }
namespace TextEditor { class TextEditorWidget; }

namespace ClangCodeModel {
namespace Internal {

class ClangdClient;

class ClangModelManagerSupport :
        public QObject,
        public CppEditor::ModelManagerSupport
{
    Q_OBJECT

public:
    ClangModelManagerSupport();
    ~ClangModelManagerSupport() override;

    CppEditor::BaseEditorDocumentProcessor *createEditorDocumentProcessor(
                TextEditor::TextDocument *baseTextDocument) override;
    bool usesClangd(const TextEditor::TextDocument *document) const override;

    static QList<LanguageClient::Client *> clientsForOpenProjects();
    static ClangdClient *clientForProject(const ProjectExplorer::Project *project);
    static ClangdClient *clientForFile(const Utils::FilePath &file);

private:
    void followSymbol(const CppEditor::CursorInEditor &data,
                      const Utils::LinkHandler &processLinkCallback, bool resolveTarget,
                      bool inNextSplit) override;
    void followSymbolToType(const CppEditor::CursorInEditor &data,
                            const Utils::LinkHandler &processLinkCallback,
                            bool inNextSplit) override;
    void switchDeclDef(const CppEditor::CursorInEditor &data,
                       const Utils::LinkHandler &processLinkCallback) override;
    void startLocalRenaming(const CppEditor::CursorInEditor &data,
                            const CppEditor::ProjectPart *projectPart,
                            CppEditor::RenameCallback &&renameSymbolsCallback) override;
    void globalRename(const CppEditor::CursorInEditor &cursor, const QString &replacement,
                      const std::function<void()> &callback) override;
    void findUsages(const CppEditor::CursorInEditor &cursor) const override;
    void switchHeaderSource(const Utils::FilePath &filePath, bool inNextSplit) override;
    void checkUnused(const Utils::Link &link, Core::SearchResult *search,
                     const Utils::LinkHandler &callback) override;

    void onEditorOpened(Core::IEditor *editor);
    void onCurrentEditorChanged(Core::IEditor *newCurrent);

    void onAbstractEditorSupportContentsUpdated(const QString &filePath,
                                                const QString &sourceFilePath,
                                                const QByteArray &content);
    void onAbstractEditorSupportRemoved(const QString &filePath);

    void onTextMarkContextMenuRequested(TextEditor::TextEditorWidget *widget,
                                        int lineNumber,
                                        QMenu *menu);

    void onClangdSettingsChanged();

    void connectTextDocumentToTranslationUnit(TextEditor::TextDocument *textDocument);
    void connectToWidgetsMarkContextMenuRequested(QWidget *editorWidget);

    void updateLanguageClient(ProjectExplorer::Project *project);
    void claimNonProjectSources(ClangdClient *client);
    void watchForExternalChanges();
    void watchForInternalChanges();
    void scheduleClientRestart(ClangdClient *client);
    static ClangdClient *clientWithProject(const ProjectExplorer::Project *project);

    Utils::FutureSynchronizer m_generatorSynchronizer;
    QList<QPointer<ClangdClient>> m_clientsToRestart;
    QTimer * const m_clientRestartTimer;
    QHash<Utils::FilePath, QString> m_potentialShadowDocuments;
};

} // namespace Internal
} // namespace ClangCodeModel
