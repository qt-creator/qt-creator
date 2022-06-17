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
class QWidget;
QT_END_NAMESPACE

namespace Core { class IEditor; }
namespace CppEditor { class RefactoringEngineInterface; }
namespace TextEditor { class TextEditorWidget; }

namespace ClangCodeModel {
namespace Internal {

class ClangdClient;

class ClangModelManagerSupport:
        public QObject,
        public CppEditor::ModelManagerSupport
{
    Q_OBJECT

public:
    ClangModelManagerSupport();
    ~ClangModelManagerSupport() override;

    CppEditor::CppCompletionAssistProvider *completionAssistProvider() override;
    TextEditor::BaseHoverHandler *createHoverHandler() override { return nullptr; }
    CppEditor::BaseEditorDocumentProcessor *createEditorDocumentProcessor(
                TextEditor::TextDocument *baseTextDocument) override;
    std::unique_ptr<CppEditor::AbstractOverviewModel> createOverviewModel() override;
    bool usesClangd(const TextEditor::TextDocument *document) const override;

    ClangdClient *clientForProject(const ProjectExplorer::Project *project) const;
    ClangdClient *clientForFile(const Utils::FilePath &file) const;

    static ClangModelManagerSupport *instance();

signals:
    void createdClient(ClangdClient *client);

private:
    void followSymbol(const CppEditor::CursorInEditor &data,
                      const Utils::LinkHandler &processLinkCallback, bool resolveTarget,
                      bool inNextSplit) override;
    void switchDeclDef(const CppEditor::CursorInEditor &data,
                       const Utils::LinkHandler &processLinkCallback) override;
    void startLocalRenaming(const CppEditor::CursorInEditor &data,
                            const CppEditor::ProjectPart *projectPart,
                            CppEditor::RenameCallback &&renameSymbolsCallback) override;
    void globalRename(const CppEditor::CursorInEditor &cursor, const QString &replacement) override;
    void findUsages(const CppEditor::CursorInEditor &cursor) const override;
    void switchHeaderSource(const Utils::FilePath &filePath, bool inNextSplit) override;

    void onEditorOpened(Core::IEditor *editor);
    void onCurrentEditorChanged(Core::IEditor *newCurrent);

    void onAbstractEditorSupportContentsUpdated(const QString &filePath,
                                                const QString &sourceFilePath,
                                                const QByteArray &content);
    void onAbstractEditorSupportRemoved(const QString &filePath);

    void onTextMarkContextMenuRequested(TextEditor::TextEditorWidget *widget,
                                        int lineNumber,
                                        QMenu *menu);

    void onProjectPartsUpdated(ProjectExplorer::Project *project);
    void onProjectPartsRemoved(const QStringList &projectPartIds);
    void onClangdSettingsChanged();

    void reinitializeBackendDocuments(const QStringList &projectPartIds);

    void connectTextDocumentToTranslationUnit(TextEditor::TextDocument *textDocument);
    void connectToWidgetsMarkContextMenuRequested(QWidget *editorWidget);

    void updateLanguageClient(ProjectExplorer::Project *project,
                              const CppEditor::ProjectInfo::ConstPtr &projectInfo);
    ClangdClient *createClient(ProjectExplorer::Project *project, const Utils::FilePath &jsonDbDir);
    void claimNonProjectSources(ClangdClient *client);
    void watchForExternalChanges();
    void watchForInternalChanges();

    Utils::FutureSynchronizer m_generatorSynchronizer;
    QList<QPointer<ClangdClient>> m_clientsToRestart;
    QHash<Utils::FilePath, QString> m_queuedShadowDocuments;
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
