/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPPMODELMANAGER_H
#define CPPMODELMANAGER_H

#include "cpptools_global.h"

#include "cppprojects.h"

#include <projectexplorer/project.h>
#include <texteditor/basetexteditor.h>

#include <cplusplus/CppDocument.h>
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
class EditorDocumentHandle;
class CppIndexingSupport;
class ModelManagerSupport;
class WorkingCopy;

namespace Internal {
class CppSourceProcessor;
class CppModelManagerPrivate;
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
    ProjectPart::Ptr projectPartForProjectFile(const QString &projectFile) const;
    /// \return All project parts that mention the given file name as one of the sources/headers.
    QList<ProjectPart::Ptr> projectPart(const QString &fileName) const;
    /// This is a fall-back function: find all files that includes the file directly or indirectly,
    /// and return its \c ProjectPart list for use with this file.
    QList<ProjectPart::Ptr> projectPartFromDependencies(const QString &fileName) const;
    /// \return A synthetic \c ProjectPart which consists of all defines/includes/frameworks from
    ///         all loaded projects.
    ProjectPart::Ptr fallbackProjectPart() const;

    CPlusPlus::Snapshot snapshot() const;
    Document::Ptr document(const QString &fileName) const;
    bool replaceDocument(Document::Ptr newDoc);

    void emitDocumentUpdated(CPlusPlus::Document::Ptr doc);

    bool isCppEditor(Core::IEditor *editor) const;

    void addExtraEditorSupport(AbstractEditorSupport *editorSupport);
    void removeExtraEditorSupport(AbstractEditorSupport *editorSupport);

    EditorDocumentHandle *editorDocument(const QString &filePath);
    void registerEditorDocument(EditorDocumentHandle *editorDocument);
    void unregisterEditorDocument(const QString &filePath);

    QList<int> references(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context);

    void renameUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context,
                      const QString &replacement = QString());
    void findUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context);

    void findMacroUsages(const CPlusPlus::Macro &macro);
    void renameMacroUsages(const CPlusPlus::Macro &macro, const QString &replacement);

    void finishedRefreshingSourceFiles(const QSet<QString> &files);

    void addModelManagerSupport(ModelManagerSupport *modelManagerSupport);
    ModelManagerSupport *modelManagerSupportForMimeType(const QString &mimeType) const;
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

    /// \brief Emitted after updateProjectInfo function is called on the model-manager.
    ///
    /// Other classes can use this to get notified when the \c ProjectExplorer has updated the parts.
    void projectPartsUpdated(ProjectExplorer::Project *project);

    void globalSnapshotChanged();

    void gcFinished(); // Needed for tests.

public slots:
    virtual void updateModifiedSourceFiles();
    virtual void GC();

private slots:
    // This should be executed in the GUI thread.
    void onAboutToLoadSession();
    void onAboutToUnloadSession();
    void onProjectAdded(ProjectExplorer::Project *project);
    void onAboutToRemoveProject(ProjectExplorer::Project *project);
    void onSourceFilesRefreshed() const;
    void onCoreAboutToClose();

private:
    void delayedGC();
    void recalculateFileToProjectParts();

    void replaceSnapshot(const CPlusPlus::Snapshot &newSnapshot);
    void removeFilesFromSnapshot(const QSet<QString> &removedFiles);
    void removeProjectInfoFilesAndIncludesFromSnapshot(const ProjectInfo &projectInfo);

    QList<EditorDocumentHandle *> cppEditors() const;

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
