/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

namespace Core { class IEditor; }

namespace TextEditor {
class BaseTextEditorWidget;
} // namespace TextEditor

namespace CppTools {

class CppEditorSupport;
class CppHighlightingSupportFactory;

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

    virtual QFuture<void> updateSourceFiles(const QStringList &sourceFiles);
    virtual WorkingCopy workingCopy() const;

    virtual QList<ProjectInfo> projectInfos() const;
    virtual ProjectInfo projectInfo(ProjectExplorer::Project *project) const;
    virtual void updateProjectInfo(const ProjectInfo &pinfo);
    virtual QList<CppTools::ProjectPart::Ptr> projectPart(const QString &fileName) const;

    virtual CPlusPlus::Snapshot snapshot() const;
    virtual Document::Ptr document(const QString &fileName) const;
    bool replaceDocument(Document::Ptr newDoc);
    virtual void GC();

    virtual bool isCppEditor(Core::IEditor *editor) const;

    void emitDocumentUpdated(CPlusPlus::Document::Ptr doc);

    virtual void addEditorSupport(AbstractEditorSupport *editorSupport);
    virtual void removeEditorSupport(AbstractEditorSupport *editorSupport);
    virtual CppEditorSupport *cppEditorSupport(TextEditor::BaseTextEditor *editor);

    virtual QList<int> references(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context);

    virtual void renameUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context,
                              const QString &replacement = QString());
    virtual void findUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context);

    virtual void findMacroUsages(const CPlusPlus::Macro &macro);
    virtual void renameMacroUsages(const CPlusPlus::Macro &macro, const QString &replacement);

    virtual void setExtraDiagnostics(const QString &fileName, const QString &key,
                                     const QList<Document::DiagnosticMessage> &diagnostics);

    void finishedRefreshingSourceFiles(const QStringList &files);

    virtual CppCompletionSupport *completionSupport(Core::IEditor *editor) const;
    virtual void setCppCompletionAssistProvider(CppCompletionAssistProvider *completionAssistProvider);

    virtual CppHighlightingSupport *highlightingSupport(Core::IEditor *editor) const;
    virtual void setHighlightingSupportFactory(CppHighlightingSupportFactory *highlightingFactory);

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

Q_SIGNALS:
    void aboutToRemoveFiles(const QStringList &files);

public Q_SLOTS:
    void editorAboutToClose(Core::IEditor *editor);
    virtual void updateModifiedSourceFiles();

private Q_SLOTS:
    // this should be executed in the GUI thread.
    void onAboutToRemoveProject(ProjectExplorer::Project *project);
    void onAboutToUnloadSession();
    void onCoreAboutToClose();
    void onProjectAdded(ProjectExplorer::Project *project);

private:
    void replaceSnapshot(const CPlusPlus::Snapshot &newSnapshot);
    WorkingCopy buildWorkingCopyList();

    void ensureUpdated();
    QStringList internalProjectFiles() const;
    QStringList internalIncludePaths() const;
    QStringList internalFrameworkPaths() const;
    QByteArray internalDefinedMacros() const;

    void dumpModelManagerConfiguration();

private:
    static QMutex m_modelManagerMutex;
    static CppModelManager *m_modelManagerInstance;

private:
    // snapshot
    mutable QMutex m_snapshotMutex;
    CPlusPlus::Snapshot m_snapshot;

    bool m_enableGC;

    // project integration
    mutable QMutex m_projectMutex;
    QMap<ProjectExplorer::Project *, ProjectInfo> m_projects;
    QMap<QString, QList<CppTools::ProjectPart::Ptr> > m_srcToProjectPart;
    // cached/calculated from the projects and/or their project-parts
    bool m_dirty;
    QStringList m_projectFiles;
    QStringList m_includePaths;
    QStringList m_frameworkPaths;
    QByteArray m_definedMacros;

    // editor integration
    mutable QMutex m_editorSupportMutex;
    QMap<TextEditor::BaseTextEditor *, CppEditorSupport *> m_editorSupport;

    QSet<AbstractEditorSupport *> m_addtionalEditorSupport;

    CppFindReferences *m_findReferences;
    bool m_indexerEnabled;

    CppCompletionAssistProvider *m_completionAssistProvider;
    CppCompletionAssistProvider *m_completionFallback;
    CppHighlightingSupportFactory *m_highlightingFactory;
    CppHighlightingSupportFactory *m_highlightingFallback;
    CppIndexingSupport *m_indexingSupporter;
    CppIndexingSupport *m_internalIndexingSupport;
};

} // namespace Internal
} // namespace CppTools

#endif // CPPMODELMANAGER_H
