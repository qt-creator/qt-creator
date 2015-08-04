/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPPMODELMANAGER_H
#define CPPMODELMANAGER_H

#include "cpptools_global.h"

#include "cppmodelmanagersupport.h"
#include "cppprojects.h"

#include <cplusplus/cppmodelmanagerbase.h>

#include <QFuture>
#include <QObject>
#include <QStringList>

namespace Core { class IEditor; }
namespace CPlusPlus { class LookupContext; }
namespace ProjectExplorer { class Project; }
namespace TextEditor { class TextDocument; }

namespace CppTools {

class AbstractEditorSupport;
class BaseEditorDocumentProcessor;
class CppCompletionAssistProvider;
class CppEditorDocumentHandle;
class CppIndexingSupport;
class WorkingCopy;

namespace Internal {
class CppSourceProcessor;
class CppModelManagerPrivate;
}

namespace Tests {
class ModelManagerTestHelper;
}

class CPPTOOLS_EXPORT CppModelManager : public CPlusPlus::CppModelManagerBase
{
    Q_OBJECT

public:
    typedef CPlusPlus::Document Document;

public:
    CppModelManager(QObject *parent = 0);
    ~CppModelManager();

    static CppModelManager *instance();

     // Documented in source file.
     enum ProgressNotificationMode {
        ForcedProgressNotification,
        ReservedProgressNotification
    };

    QFuture<void> updateSourceFiles(const QSet<QString> &sourceFiles,
        ProgressNotificationMode mode = ReservedProgressNotification);
    WorkingCopy workingCopy() const;
    QByteArray codeModelConfiguration() const;

    QList<ProjectInfo> projectInfos() const;
    ProjectInfo projectInfo(ProjectExplorer::Project *project) const;
    QFuture<void> updateProjectInfo(const ProjectInfo &newProjectInfo);

    /// \return The project part with the given project file
    ProjectPart::Ptr projectPartForId(const QString &projectPartId) const;
    /// \return All project parts that mention the given file name as one of the sources/headers.
    QList<ProjectPart::Ptr> projectPart(const Utils::FileName &fileName) const;
    QList<ProjectPart::Ptr> projectPart(const QString &fileName) const
    { return projectPart(Utils::FileName::fromString(fileName)); }
    /// This is a fall-back function: find all files that includes the file directly or indirectly,
    /// and return its \c ProjectPart list for use with this file.
    QList<ProjectPart::Ptr> projectPartFromDependencies(const Utils::FileName &fileName) const;
    /// \return A synthetic \c ProjectPart which consists of all defines/includes/frameworks from
    ///         all loaded projects.
    ProjectPart::Ptr fallbackProjectPart() const;

    CPlusPlus::Snapshot snapshot() const;
    Document::Ptr document(const QString &fileName) const;
    bool replaceDocument(Document::Ptr newDoc);

    void emitDocumentUpdated(CPlusPlus::Document::Ptr doc);
    void emitAbstractEditorSupportContentsUpdated(const QString &filePath,
                                                  const QByteArray &contents);
    void emitAbstractEditorSupportRemoved(const QString &filePath);

    bool isCppEditor(Core::IEditor *editor) const;

    QSet<AbstractEditorSupport*> abstractEditorSupports() const;
    void addExtraEditorSupport(AbstractEditorSupport *editorSupport);
    void removeExtraEditorSupport(AbstractEditorSupport *editorSupport);

    QList<CppEditorDocumentHandle *> cppEditorDocuments() const;
    CppEditorDocumentHandle *cppEditorDocument(const QString &filePath) const;
    void registerCppEditorDocument(CppEditorDocumentHandle *cppEditorDocument);
    void unregisterCppEditorDocument(const QString &filePath);

    QList<int> references(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context);

    void renameUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context,
                      const QString &replacement = QString());
    void findUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context);

    void findMacroUsages(const CPlusPlus::Macro &macro);
    void renameMacroUsages(const CPlusPlus::Macro &macro, const QString &replacement);

    void finishedRefreshingSourceFiles(const QSet<QString> &files);

    void addModelManagerSupportProvider(ModelManagerSupportProvider *modelManagerSupportProvider);
    CppCompletionAssistProvider *completionAssistProvider(const QString &mimeType) const;
    BaseEditorDocumentProcessor *editorDocumentProcessor(
        TextEditor::TextDocument *baseTextDocument) const;

    void setIndexingSupport(CppIndexingSupport *indexingSupport);
    CppIndexingSupport *indexingSupport();

    QStringList projectFiles();

    ProjectPart::HeaderPaths headerPaths();

    // Use this *only* for auto tests
    void setHeaderPaths(const ProjectPart::HeaderPaths &headerPaths);

    QByteArray definedMacros();

    void enableGarbageCollector(bool enable);

    static QSet<QString> timeStampModifiedFiles(const QList<Document::Ptr> &documentsToCheck);

    static Internal::CppSourceProcessor *createSourceProcessor();
    static QString configurationFileName();
    static QString editorConfigurationFileName();

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

private slots:
    // This should be executed in the GUI thread.
    friend class Tests::ModelManagerTestHelper;
    void onAboutToLoadSession();
    void onAboutToUnloadSession();
    void renameIncludes(const QString &oldFileName, const QString &newFileName);
    void onProjectAdded(ProjectExplorer::Project *project);
    void onAboutToRemoveProject(ProjectExplorer::Project *project);
    void onSourceFilesRefreshed() const;
    void onCurrentEditorChanged(Core::IEditor *editor);
    void onCodeModelSettingsChanged();
    void onCoreAboutToClose();

private:
    void delayedGC();
    void recalculateProjectPartMappings();
    void updateCppEditorDocuments() const;

    void replaceSnapshot(const CPlusPlus::Snapshot &newSnapshot);
    void removeFilesFromSnapshot(const QSet<QString> &removedFiles);
    void removeProjectInfoFilesAndIncludesFromSnapshot(const ProjectInfo &projectInfo);

    void handleAddedModelManagerSupports(const QSet<QString> &supportIds);
    QList<ModelManagerSupport::Ptr> handleRemovedModelManagerSupports(
            const QSet<QString> &supportIds);
    void closeCppEditorDocuments();

    ModelManagerSupport::Ptr modelManagerSupportForMimeType(const QString &mimeType) const;

    WorkingCopy buildWorkingCopyList();

    void ensureUpdated();
    QStringList internalProjectFiles() const;
    ProjectPart::HeaderPaths internalHeaderPaths() const;
    QByteArray internalDefinedMacros() const;

    void dumpModelManagerConfiguration(const QString &logFileId);

private:
    Internal::CppModelManagerPrivate *d;
};

} // namespace CppTools

#endif // CPPMODELMANAGER_H
