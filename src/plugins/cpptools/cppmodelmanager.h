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

#include "cpptools_global.h"

#include "cppmodelmanagerinterface.h"
#include "refactoringengineinterface.h"
#include "projectinfo.h"
#include "projectpart.h"
#include <projectexplorer/headerpath.h>

#include <cplusplus/cppmodelmanagerbase.h>
#include <coreplugin/find/ifindfilter.h>
#include <coreplugin/locator/ilocatorfilter.h>

#include <QFuture>
#include <QObject>
#include <QStringList>

namespace Core {
class IDocument;
class IEditor;
}
namespace CPlusPlus { class LookupContext; }
namespace ProjectExplorer { class Project; }
namespace TextEditor {
class BaseHoverHandler;
class Indenter;
class TextDocument;
} // namespace TextEditor

namespace CppTools {

class AbstractEditorSupport;
class AbstractOverviewModel;
class BaseEditorDocumentProcessor;
class CppCompletionAssistProvider;
class CppEditorDocumentHandle;
class CppIndexingSupport;
class ModelManagerSupportProvider;
class FollowSymbolInterface;
class SymbolFinder;
class WorkingCopy;

namespace Internal {
class CppSourceProcessor;
class CppModelManagerPrivate;
class CppToolsPlugin;
}

namespace Tests {
class ModelManagerTestHelper;
}

enum class RefactoringEngineType : int
{
    BuiltIn = 0,
    ClangCodeModel = 1,
    ClangRefactoring = 2
};

class CPPTOOLS_EXPORT CppModelManager final : public CPlusPlus::CppModelManagerBase,
        public RefactoringEngineInterface,
        public CppModelManagerInterface
{
    Q_OBJECT

public:
    typedef CPlusPlus::Document Document;

public:
    CppModelManager();
    ~CppModelManager() override;

    static CppModelManager *instance();
    static void createCppModelManager(Internal::CppToolsPlugin *parent);

     // Documented in source file.
     enum ProgressNotificationMode {
        ForcedProgressNotification,
        ReservedProgressNotification
    };

    QFuture<void> updateSourceFiles(const QSet<QString> &sourceFiles,
                                    ProgressNotificationMode mode = ReservedProgressNotification);
    QFuture<void> updateSourceFiles(const QFutureInterface<void> &superFuture,
                                    const QSet<QString> &sourceFiles,
                                    ProgressNotificationMode mode = ReservedProgressNotification);
    void updateCppEditorDocuments(bool projectsUpdated = false) const;
    WorkingCopy workingCopy() const;
    QByteArray codeModelConfiguration() const;

    QList<ProjectInfo> projectInfos() const;
    ProjectInfo projectInfo(ProjectExplorer::Project *project) const;
    QFuture<void> updateProjectInfo(const ProjectInfo &newProjectInfo);
    QFuture<void> updateProjectInfo(QFutureInterface<void> &futureInterface,
                                    const ProjectInfo &newProjectInfo);

    /// \return The project part with the given project file
    ProjectPart::Ptr projectPartForId(const QString &projectPartId) const override;
    /// \return All project parts that mention the given file name as one of the sources/headers.
    QList<ProjectPart::Ptr> projectPart(const Utils::FileName &fileName) const;
    QList<ProjectPart::Ptr> projectPart(const QString &fileName) const
    { return projectPart(Utils::FileName::fromString(fileName)); }
    /// This is a fall-back function: find all files that includes the file directly or indirectly,
    /// and return its \c ProjectPart list for use with this file.
    QList<ProjectPart::Ptr> projectPartFromDependencies(const Utils::FileName &fileName) const;
    /// \return A synthetic \c ProjectPart which consists of all defines/includes/frameworks from
    ///         all loaded projects.
    ProjectPart::Ptr fallbackProjectPart();

    CPlusPlus::Snapshot snapshot() const override;
    Document::Ptr document(const QString &fileName) const;
    bool replaceDocument(Document::Ptr newDoc);

    void emitDocumentUpdated(CPlusPlus::Document::Ptr doc);
    void emitAbstractEditorSupportContentsUpdated(const QString &filePath,
                                                  const QByteArray &contents);
    void emitAbstractEditorSupportRemoved(const QString &filePath);

    static bool isCppEditor(Core::IEditor *editor);
    bool isClangCodeModelActive() const;

    QSet<AbstractEditorSupport*> abstractEditorSupports() const;
    void addExtraEditorSupport(AbstractEditorSupport *editorSupport);
    void removeExtraEditorSupport(AbstractEditorSupport *editorSupport);

    QList<CppEditorDocumentHandle *> cppEditorDocuments() const;
    CppEditorDocumentHandle *cppEditorDocument(const QString &filePath) const;
    void registerCppEditorDocument(CppEditorDocumentHandle *cppEditorDocument);
    void unregisterCppEditorDocument(const QString &filePath);

    QList<int> references(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context);

    void startLocalRenaming(const CursorInEditor &data,
                            CppTools::ProjectPart *projectPart,
                            RenameCallback &&renameSymbolsCallback) final;
    void globalRename(const CursorInEditor &data, UsagesCallback &&renameCallback,
                      const QString &replacement) final;
    void findUsages(const CppTools::CursorInEditor &data,
                    UsagesCallback &&showUsagesCallback) const final;
    void globalFollowSymbol(const CursorInEditor &data,
                            Utils::ProcessLinkCallback &&processLinkCallback,
                            const CPlusPlus::Snapshot &snapshot,
                            const CPlusPlus::Document::Ptr &documentFromSemanticInfo,
                            SymbolFinder *symbolFinder,
                            bool inNextSplit) const final;

    void renameUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context,
                      const QString &replacement = QString());
    void findUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context);

    void findMacroUsages(const CPlusPlus::Macro &macro);
    void renameMacroUsages(const CPlusPlus::Macro &macro, const QString &replacement);

    void finishedRefreshingSourceFiles(const QSet<QString> &files);

    void activateClangCodeModel(ModelManagerSupportProvider *modelManagerSupportProvider);
    CppCompletionAssistProvider *completionAssistProvider() const;
    BaseEditorDocumentProcessor *createEditorDocumentProcessor(
                    TextEditor::TextDocument *baseTextDocument) const;
    TextEditor::BaseHoverHandler *createHoverHandler() const;
    FollowSymbolInterface &followSymbolInterface() const;
    std::unique_ptr<AbstractOverviewModel> createOverviewModel() const;

    void setIndexingSupport(CppIndexingSupport *indexingSupport);
    CppIndexingSupport *indexingSupport();

    QStringList projectFiles();

    ProjectExplorer::HeaderPaths headerPaths();

    // Use this *only* for auto tests
    void setHeaderPaths(const ProjectExplorer::HeaderPaths &headerPaths);

    ProjectExplorer::Macros definedMacros();

    void enableGarbageCollector(bool enable);

    SymbolFinder *symbolFinder();

    QThreadPool *sharedThreadPool();

    static QSet<QString> timeStampModifiedFiles(const QList<Document::Ptr> &documentsToCheck);

    static Internal::CppSourceProcessor *createSourceProcessor();
    static QString configurationFileName();
    static QString editorConfigurationFileName();

    static void addRefactoringEngine(RefactoringEngineType type,
                                     RefactoringEngineInterface *refactoringEngine);
    static void removeRefactoringEngine(RefactoringEngineType type);

    using CppIndenterCreator = std::function<TextEditor::Indenter *()>;
    void setCppIndenterCreator(CppIndenterCreator indenterCreator)
    {
        createCppIndenter = std::move(indenterCreator);
    }
    CppIndenterCreator createCppIndenter;

    void setLocatorFilter(std::unique_ptr<Core::ILocatorFilter> &&filter);
    void setClassesFilter(std::unique_ptr<Core::ILocatorFilter> &&filter);
    void setIncludesFilter(std::unique_ptr<Core::ILocatorFilter> &&filter);
    void setFunctionsFilter(std::unique_ptr<Core::ILocatorFilter> &&filter);
    void setSymbolsFindFilter(std::unique_ptr<Core::IFindFilter> &&filter);
    void setCurrentDocumentFilter(std::unique_ptr<Core::ILocatorFilter> &&filter);

    Core::ILocatorFilter *locatorFilter() const;
    Core::ILocatorFilter *classesFilter() const;
    Core::ILocatorFilter *includesFilter() const;
    Core::ILocatorFilter *functionsFilter() const;
    Core::IFindFilter *symbolsFindFilter() const;
    Core::ILocatorFilter *currentDocumentFilter() const;

    void renameIncludes(const QString &oldFileName, const QString &newFileName);

    void setBackendJobsPostponed(bool postponed);

signals:
    /// Project data might be locked while this is emitted.
    void aboutToRemoveFiles(const QStringList &files);

    void documentUpdated(CPlusPlus::Document::Ptr doc);
    void sourceFilesRefreshed(const QSet<QString> &files);

    void projectPartsUpdated(ProjectExplorer::Project *project);
    void projectPartsRemoved(const QStringList &projectPartIds);

    void globalSnapshotChanged();

    void gcFinished(); // Needed for tests.

    void abstractEditorSupportContentsUpdated(const QString &filePath, const QByteArray &contents);
    void abstractEditorSupportRemoved(const QString &filePath);

public slots:
    void updateModifiedSourceFiles();
    void GC();

private:
    // This should be executed in the GUI thread.
    friend class Tests::ModelManagerTestHelper;
    void onAboutToLoadSession();
    void onProjectAdded(ProjectExplorer::Project *project);
    void onAboutToRemoveProject(ProjectExplorer::Project *project);
    void onActiveProjectChanged(ProjectExplorer::Project *project);
    void onSourceFilesRefreshed() const;
    void onCurrentEditorChanged(Core::IEditor *editor);
    void onCoreAboutToClose();

    void initializeBuiltinModelManagerSupport();
    void delayedGC();
    void recalculateProjectPartMappings();
    void watchForCanceledProjectIndexer(const QVector<QFuture<void> > &futures,
                                        ProjectExplorer::Project *project);

    void replaceSnapshot(const CPlusPlus::Snapshot &newSnapshot);
    void removeFilesFromSnapshot(const QSet<QString> &removedFiles);
    void removeProjectInfoFilesAndIncludesFromSnapshot(const ProjectInfo &projectInfo);

    WorkingCopy buildWorkingCopyList();

    void ensureUpdated();
    QStringList internalProjectFiles() const;
    ProjectExplorer::HeaderPaths internalHeaderPaths() const;
    ProjectExplorer::Macros internalDefinedMacros() const;

    void dumpModelManagerConfiguration(const QString &logFileId);
    void initCppTools();

private:
    Internal::CppModelManagerPrivate *d;
};

} // namespace CppTools
