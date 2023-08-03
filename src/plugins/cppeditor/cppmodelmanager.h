// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include "cursorineditor.h"
#include "projectinfo.h"
#include "projectpart.h"

#include <cplusplus/cppmodelmanagerbase.h>
#include <coreplugin/find/ifindfilter.h>
#include <coreplugin/locator/ilocatorfilter.h>
#include <projectexplorer/headerpath.h>
#include <utils/link.h>

#include <QFuture>
#include <QObject>
#include <QStringList>

#include <functional>
#include <memory>

namespace Core {
class IDocument;
class IEditor;
class SearchResult;
}
namespace CPlusPlus {
class AST;
class CallAST;
class LookupContext;
} // namespace CPlusPlus
namespace ProjectExplorer { class Project; }
namespace TextEditor {
class BaseHoverHandler;
class TextDocument;
} // namespace TextEditor

namespace CppEditor {

class AbstractEditorSupport;
class BaseEditorDocumentProcessor;
class CppCompletionAssistProvider;
class CppEditorDocumentHandle;
class CppIndexingSupport;
class CppLocatorData;
class FollowSymbolUnderCursor;
class ModelManagerSupportProvider;
class ModelManagerSupport;
class SymbolFinder;
class WorkingCopy;

namespace Internal {
class CppSourceProcessor;
class CppEditorPluginPrivate;
class CppModelManagerPrivate;
}

namespace Tests { class ModelManagerTestHelper; }

enum class SignalSlotType {
    OldStyleSignal,
    NewStyleSignal,
    None
};

class CPPEDITOR_EXPORT CppModelManager final : public QObject
{
    Q_OBJECT

private:
    friend class Internal::CppEditorPluginPrivate;
    CppModelManager();
    ~CppModelManager() override;

public:
    using Document = CPlusPlus::Document;

    static CppModelManager *instance();

    static void registerJsExtension();

    // Documented in source file.
    enum ProgressNotificationMode {
        ForcedProgressNotification,
        ReservedProgressNotification
    };

    static QFuture<void> updateSourceFiles(const QSet<Utils::FilePath> &sourceFiles,
                                            ProgressNotificationMode mode = ReservedProgressNotification);
    static void updateCppEditorDocuments(bool projectsUpdated = false);
    static WorkingCopy workingCopy();
    static QByteArray codeModelConfiguration();
    static CppLocatorData *locatorData();

    static bool setExtraDiagnostics(const Utils::FilePath &filePath,
                                    const QString &kind,
                                    const QList<Document::DiagnosticMessage> &diagnostics);

    static const QList<Document::DiagnosticMessage> diagnosticMessages();

    static ProjectInfoList projectInfos();
    static ProjectInfo::ConstPtr projectInfo(ProjectExplorer::Project *project);
    static QFuture<void> updateProjectInfo(const ProjectInfo::ConstPtr &newProjectInfo,
                                           const QSet<Utils::FilePath> &additionalFiles = {});

    /// \return The project part with the given project file
    static ProjectPart::ConstPtr projectPartForId(const QString &projectPartId);
    /// \return All project parts that mention the given file name as one of the sources/headers.
    static QList<ProjectPart::ConstPtr> projectPart(const Utils::FilePath &fileName);
    static QList<ProjectPart::ConstPtr> projectPart(const QString &fileName)
    { return projectPart(Utils::FilePath::fromString(fileName)); }
    /// This is a fall-back function: find all files that includes the file directly or indirectly,
    /// and return its \c ProjectPart list for use with this file.
    static QList<ProjectPart::ConstPtr> projectPartFromDependencies(const Utils::FilePath &fileName);
    /// \return A synthetic \c ProjectPart which consists of all defines/includes/frameworks from
    ///         all loaded projects.
    static ProjectPart::ConstPtr fallbackProjectPart();

    static CPlusPlus::Snapshot snapshot();
    static Document::Ptr document(const Utils::FilePath &filePath);
    static bool replaceDocument(Document::Ptr newDoc);

    static void emitDocumentUpdated(Document::Ptr doc);
    static void emitAbstractEditorSupportContentsUpdated(const QString &filePath,
                                                  const QString &sourcePath,
                                                  const QByteArray &contents);
    static void emitAbstractEditorSupportRemoved(const QString &filePath);

    static bool isCppEditor(Core::IEditor *editor);
    static bool usesClangd(const TextEditor::TextDocument *document);
    static bool isClangCodeModelActive();

    static QSet<AbstractEditorSupport*> abstractEditorSupports();
    static void addExtraEditorSupport(AbstractEditorSupport *editorSupport);
    static void removeExtraEditorSupport(AbstractEditorSupport *editorSupport);

    static const QList<CppEditorDocumentHandle *> cppEditorDocuments();
    static CppEditorDocumentHandle *cppEditorDocument(const Utils::FilePath &filePath);
    static BaseEditorDocumentProcessor *cppEditorDocumentProcessor(const Utils::FilePath &filePath);
    static void registerCppEditorDocument(CppEditorDocumentHandle *cppEditorDocument);
    static void unregisterCppEditorDocument(const QString &filePath);

    static QList<int> references(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context);

    static SignalSlotType getSignalSlotType(const Utils::FilePath &filePath,
                                            const QByteArray &content,
                                            int position);

    static void renameUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context,
                             const QString &replacement = QString(),
                             const std::function<void()> &callback = {});
    static void renameUsages(const CPlusPlus::Document::Ptr &doc,
                             const QTextCursor &cursor,
                             const CPlusPlus::Snapshot &snapshot,
                             const QString &replacement,
                             const std::function<void()> &callback);
    static void findUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context);

    static void findMacroUsages(const CPlusPlus::Macro &macro);
    static void renameMacroUsages(const CPlusPlus::Macro &macro, const QString &replacement);

    static void finishedRefreshingSourceFiles(const QSet<QString> &files);

    static void activateClangCodeModel(std::unique_ptr<ModelManagerSupport> &&modelManagerSupport);
    static CppCompletionAssistProvider *completionAssistProvider();
    static BaseEditorDocumentProcessor *createEditorDocumentProcessor(
                    TextEditor::TextDocument *baseTextDocument);
    static TextEditor::BaseHoverHandler *createHoverHandler();
    static FollowSymbolUnderCursor &builtinFollowSymbol();

    enum class Backend { Builtin, Best };
    static void followSymbol(const CursorInEditor &data,
                             const Utils::LinkHandler &processLinkCallback,
                             bool resolveTarget, bool inNextSplit, Backend backend = Backend::Best);
    static void followSymbolToType(const CursorInEditor &data,
                                   const Utils::LinkHandler &processLinkCallback, bool inNextSplit,
                                   Backend backend = Backend::Best);
    static void switchDeclDef(const CursorInEditor &data,
                              const Utils::LinkHandler &processLinkCallback,
                              Backend backend = Backend::Best);
    static void startLocalRenaming(const CursorInEditor &data, const ProjectPart *projectPart,
                                   RenameCallback &&renameSymbolsCallback,
                                   Backend backend = Backend::Best);
    static void globalRename(const CursorInEditor &data, const QString &replacement,
                             const std::function<void()> &callback = {},
                             Backend backend = Backend::Best);
    static void findUsages(const CursorInEditor &data, Backend backend = Backend::Best);
    static void switchHeaderSource(bool inNextSplit, Backend backend = Backend::Best);
    static void showPreprocessedFile(bool inNextSplit);
    static void foldComments();
    static void unfoldComments();
    static void findUnusedFunctions(const Utils::FilePath &folder);
    static void checkForUnusedSymbol(Core::SearchResult *search, const Utils::Link &link,
                                     CPlusPlus::Symbol *symbol,
                                     const CPlusPlus::LookupContext &context,
                                     const Utils::LinkHandler &callback);
    static CppIndexingSupport *indexingSupport();

    static Utils::FilePaths projectFiles();

    static ProjectExplorer::HeaderPaths headerPaths();

    // Use this *only* for auto tests
    static void setHeaderPaths(const ProjectExplorer::HeaderPaths &headerPaths);

    static ProjectExplorer::Macros definedMacros();

    static void enableGarbageCollector(bool enable);

    static SymbolFinder *symbolFinder();

    static QThreadPool *sharedThreadPool();

    static QSet<Utils::FilePath> timeStampModifiedFiles(const QList<Document::Ptr> &documentsToCheck);

    static Internal::CppSourceProcessor *createSourceProcessor();
    static const Utils::FilePath &configurationFileName();
    static const Utils::FilePath &editorConfigurationFileName();

    static void setLocatorFilter(std::unique_ptr<Core::ILocatorFilter> &&filter);
    static void setClassesFilter(std::unique_ptr<Core::ILocatorFilter> &&filter);
    static void setIncludesFilter(std::unique_ptr<Core::ILocatorFilter> &&filter);
    static void setFunctionsFilter(std::unique_ptr<Core::ILocatorFilter> &&filter);
    static void setSymbolsFindFilter(std::unique_ptr<Core::IFindFilter> &&filter);
    static void setCurrentDocumentFilter(std::unique_ptr<Core::ILocatorFilter> &&filter);

    static Core::ILocatorFilter *locatorFilter();
    static Core::ILocatorFilter *classesFilter();
    static Core::ILocatorFilter *includesFilter();
    static Core::ILocatorFilter *functionsFilter();
    static Core::IFindFilter *symbolsFindFilter();
    static Core::ILocatorFilter *currentDocumentFilter();

    /*
     * try to find build system target that depends on the given file - if the file is no header
     * try to find the corresponding header and use this instead to find the respective target
     */
    static QSet<QString> dependingInternalTargets(const Utils::FilePath &file);

    static QSet<QString> internalTargets(const Utils::FilePath &filePath);

    static void renameIncludes(const Utils::FilePath &oldFilePath, const Utils::FilePath &newFilePath);

    // for VcsBaseSubmitEditor
    Q_INVOKABLE QSet<QString> symbolsInFiles(const QSet<Utils::FilePath> &files) const;

    static ModelManagerSupport *modelManagerSupport(Backend backend);

signals:
    /// Project data might be locked while this is emitted.
    void aboutToRemoveFiles(const QStringList &files);

    void documentUpdated(CPlusPlus::Document::Ptr doc);
    void sourceFilesRefreshed(const QSet<QString> &files);

    void projectPartsUpdated(ProjectExplorer::Project *project);
    void projectPartsRemoved(const QStringList &projectPartIds);

    void globalSnapshotChanged();

    void gcFinished(); // Needed for tests.

    void abstractEditorSupportContentsUpdated(const QString &filePath,
                                              const QString &sourcePath,
                                              const QByteArray &contents);
    void abstractEditorSupportRemoved(const QString &filePath);
    void fallbackProjectPartUpdated();

    void diagnosticsChanged(const Utils::FilePath &filePath, const QString &kind);

public slots:
    static void updateModifiedSourceFiles();
    static void GC();

private:
    // This should be executed in the GUI thread.
    friend class Tests::ModelManagerTestHelper;
    static void onAboutToLoadSession();
    static void onProjectAdded(ProjectExplorer::Project *project);
    static void onAboutToRemoveProject(ProjectExplorer::Project *project);
    static void onActiveProjectChanged(ProjectExplorer::Project *project);
    static void onSourceFilesRefreshed();
    static void onCurrentEditorChanged(Core::IEditor *editor);
    static void onCoreAboutToClose();
    static void setupFallbackProjectPart();

    static void delayedGC();
    static void recalculateProjectPartMappings();

    static void replaceSnapshot(const CPlusPlus::Snapshot &newSnapshot);
    static void removeFilesFromSnapshot(const QSet<Utils::FilePath> &removedFiles);
    static void removeProjectInfoFilesAndIncludesFromSnapshot(const ProjectInfo &projectInfo);

    static WorkingCopy buildWorkingCopyList();

    static void ensureUpdated();
    static Utils::FilePaths internalProjectFiles();
    static ProjectExplorer::HeaderPaths internalHeaderPaths();
    static ProjectExplorer::Macros internalDefinedMacros();

    static void dumpModelManagerConfiguration(const QString &logFileId);
    static void initCppTools();
};

} // CppEditor
