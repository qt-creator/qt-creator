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

#include "cppmodelmanagerinterface.h"

#include <projectexplorer/project.h>
#include <texteditor/basetexteditor.h>

#include <QHash>
#include <QMutex>
#include <QTimer>

namespace Core { class IEditor; }
namespace TextEditor { class BaseTextEditorWidget; }

namespace CppTools {

class CppEditorSupport;

namespace Internal {

class CppFindReferences;

class CPPTOOLS_EXPORT CppModelManager : public CppTools::CppModelManagerInterface
{
    Q_OBJECT

public:
    typedef CPlusPlus::Document Document;

public:
    CppModelManager(QObject *parent = 0);
    virtual ~CppModelManager();

    static CppModelManager *instance();

    virtual QFuture<void> updateSourceFiles(const QStringList &sourceFiles,
        ProgressNotificationMode mode = ReservedProgressNotification);
    virtual WorkingCopy workingCopy() const;
    virtual QByteArray codeModelConfiguration() const;

    virtual QList<ProjectInfo> projectInfos() const;
    virtual ProjectInfo projectInfo(ProjectExplorer::Project *project) const;
    virtual QFuture<void> updateProjectInfo(const ProjectInfo &newProjectInfo);

    /// \return The project part with the given project file
    virtual ProjectPart::Ptr projectPartForProjectFile(const QString &projectFile) const;
    /// \return All project parts that mention the given file name as one of the sources/headers.
    virtual QList<ProjectPart::Ptr> projectPart(const QString &fileName) const;
    /// This is a fall-back function: find all files that includes the file directly or indirectly,
    /// and return its \c ProjectPart list for use with this file.
    virtual QList<ProjectPart::Ptr> projectPartFromDependencies(const QString &fileName) const;
    /// \return A synthetic \c ProjectPart which consists of all defines/includes/frameworks from
    ///         all loaded projects.
    virtual ProjectPart::Ptr fallbackProjectPart() const;

    virtual CPlusPlus::Snapshot snapshot() const;
    virtual Document::Ptr document(const QString &fileName) const;
    bool replaceDocument(Document::Ptr newDoc);

    void emitDocumentUpdated(CPlusPlus::Document::Ptr doc);

    virtual bool isCppEditor(Core::IEditor *editor) const;

    virtual void addExtraEditorSupport(AbstractEditorSupport *editorSupport);
    virtual void removeExtraEditorSupport(AbstractEditorSupport *editorSupport);
    virtual CppEditorSupport *cppEditorSupport(TextEditor::BaseTextEditor *textEditor);
    virtual void deleteCppEditorSupport(TextEditor::BaseTextEditor *textEditor);

    virtual QList<int> references(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context);

    virtual void renameUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context,
                              const QString &replacement = QString());
    virtual void findUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context);

    virtual void findMacroUsages(const CPlusPlus::Macro &macro);
    virtual void renameMacroUsages(const CPlusPlus::Macro &macro, const QString &replacement);

    virtual bool setExtraDiagnostics(const QString &fileName, const QString &key,
                                     const QList<Document::DiagnosticMessage> &diagnostics);
    virtual void setIfdefedOutBlocks(const QString &fileName,
                                     const QList<TextEditor::BlockRange> &ifdeffedOutBlocks);

    void finishedRefreshingSourceFiles(const QStringList &files);

    virtual void addModelManagerSupport(ModelManagerSupport *modelManagerSupport);
    virtual ModelManagerSupport *modelManagerSupportForMimeType(const QString &mimeType) const;
    virtual CppCompletionAssistProvider *completionAssistProvider(Core::IEditor *editor) const;
    virtual CppHighlightingSupport *highlightingSupport(Core::IEditor *editor) const;

    virtual void setIndexingSupport(CppIndexingSupport *indexingSupport);
    virtual CppIndexingSupport *indexingSupport();

    QStringList projectFiles()
    {
        ensureUpdated();
        return m_projectFiles;
    }

    QStringList includePaths()
    {
        ensureUpdated();
        return m_includePaths;
    }

    // Use this *only* for auto tests
    void setIncludePaths(const QStringList &includePaths)
    {
        m_includePaths = includePaths;
    }

    QStringList frameworkPaths()
    {
        ensureUpdated();
        return m_frameworkPaths;
    }

    QByteArray definedMacros()
    {
        ensureUpdated();
        return m_definedMacros;
    }

    void enableGarbageCollector(bool enable);

    static QStringList timeStampModifiedFiles(const QList<Document::Ptr> &documentsToCheck);

signals:
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
    void onCoreAboutToClose();

private:
    void delayedGC();
    void recalculateFileToProjectParts();

    void replaceSnapshot(const CPlusPlus::Snapshot &newSnapshot);
    void removeFilesFromSnapshot(const QSet<QString> &removedFiles);
    void removeProjectInfoFilesAndIncludesFromSnapshot(const ProjectInfo &projectInfo);

    WorkingCopy buildWorkingCopyList();

    void ensureUpdated();
    QStringList internalProjectFiles() const;
    QStringList internalIncludePaths() const;
    QStringList internalFrameworkPaths() const;
    QByteArray internalDefinedMacros() const;

    void dumpModelManagerConfiguration(const QString &logFileId);

private:
    static QMutex m_instanceMutex;
    static CppModelManager *m_instance;

private:
    // Snapshot
    mutable QMutex m_snapshotMutex;
    CPlusPlus::Snapshot m_snapshot;

    // Project integration
    mutable QMutex m_projectMutex;
    QMap<ProjectExplorer::Project *, ProjectInfo> m_projectToProjectsInfo;
    QMap<QString, QList<CppTools::ProjectPart::Ptr> > m_fileToProjectParts;
    QMap<QString, CppTools::ProjectPart::Ptr> m_projectFileToProjectPart;
    // The members below are cached/(re)calculated from the projects and/or their project parts
    bool m_dirty;
    QStringList m_projectFiles;
    QStringList m_includePaths;
    QStringList m_frameworkPaths;
    QByteArray m_definedMacros;

    // Editor integration
    mutable QMutex m_cppEditorSupportsMutex;
    QMap<TextEditor::BaseTextEditor *, CppEditorSupport *> m_cppEditorSupports;
    QSet<AbstractEditorSupport *> m_extraEditorSupports;

    // Completion & highlighting
    QHash<QString, ModelManagerSupport *> m_idTocodeModelSupporter;
    QScopedPointer<ModelManagerSupport> m_modelManagerSupportFallback;

    // Indexing
    CppIndexingSupport *m_indexingSupporter;
    CppIndexingSupport *m_internalIndexingSupport;
    bool m_indexerEnabled;

    CppFindReferences *m_findReferences;

    bool m_enableGC;
    QTimer *m_delayedGcTimer;
};

} // namespace Internal
} // namespace CppTools

#endif // CPPMODELMANAGER_H
