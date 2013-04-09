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
#include "cpptoolsconstants.h"
#include "ModelManagerInterface.h"

#include <projectexplorer/project.h>
#include <texteditor/basetexteditor.h>

#include <cplusplus/PreprocessorEnvironment.h>
#include <cplusplus/pp-engine.h>

#include <QHash>
#include <QMutex>
#include <QTimer>
#include <QTextEdit> // for QTextEdit::ExtraSelection

namespace Core { class IEditor; }

namespace TextEditor {
class ITextEditor;
class BaseTextEditorWidget;
} // namespace TextEditor

namespace ProjectExplorer { class ProjectExplorerPlugin; }

namespace CPlusPlus { class ParseManager; }

namespace CppTools {

class CppCompletionSupportFactory;
class CppHighlightingSupportFactory;

namespace Internal {

class CppEditorSupport;
class CppPreprocessor;
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

    CppEditorSupport *editorSupport(TextEditor::ITextEditor *editor) const
    { return m_editorSupport.value(editor); }

    void emitDocumentUpdated(CPlusPlus::Document::Ptr doc);

    void stopEditorSelectionsUpdate()
    { m_updateEditorSelectionsTimer->stop(); }

    virtual void addEditorSupport(AbstractEditorSupport *editorSupport);
    virtual void removeEditorSupport(AbstractEditorSupport *editorSupport);

    virtual QList<int> references(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context);

    virtual void renameUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context,
                              const QString &replacement = QString());
    virtual void findUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context);

    virtual void findMacroUsages(const CPlusPlus::Macro &macro);
    virtual void renameMacroUsages(const CPlusPlus::Macro &macro, const QString &replacement);

    virtual void setExtraDiagnostics(const QString &fileName, int key,
                                     const QList<Document::DiagnosticMessage> &diagnostics);
    virtual QList<Document::DiagnosticMessage> extraDiagnostics(
            const QString &fileName, int key = AllExtraDiagnostics) const;

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
    void projectPathChanged(const QString &projectPath);

    void aboutToRemoveFiles(const QStringList &files);

public Q_SLOTS:
    void editorOpened(Core::IEditor *editor);
    void editorAboutToClose(Core::IEditor *editor);
    virtual void updateModifiedSourceFiles();

private Q_SLOTS:
    // this should be executed in the GUI thread.
    void onDocumentUpdated(CPlusPlus::Document::Ptr doc);
    void onExtraDiagnosticsUpdated(const QString &fileName);
    void onAboutToRemoveProject(ProjectExplorer::Project *project);
    void onAboutToUnloadSession();
    void onProjectAdded(ProjectExplorer::Project *project);
    void postEditorUpdate();
    void updateEditorSelections();

private:
    void updateEditor(Document::Ptr doc);

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
    CPlusPlus::Snapshot m_snapshot;

    // cache
    bool m_dirty;
    QStringList m_projectFiles;
    QStringList m_includePaths;
    QStringList m_frameworkPaths;
    QByteArray m_definedMacros;

    // editor integration
    QMap<TextEditor::ITextEditor *, CppEditorSupport *> m_editorSupport;

    QSet<AbstractEditorSupport *> m_addtionalEditorSupport;

    // project integration
    QMap<ProjectExplorer::Project *, ProjectInfo> m_projects;

    mutable QMutex m_mutex;
    mutable QMutex m_protectSnapshot;

    struct Editor {
        Editor()
            : revision(-1)
            , updateSelections(true)
        {}
        int revision;
        bool updateSelections;
        QPointer<TextEditor::ITextEditor> textEditor;
        QList<QTextEdit::ExtraSelection> selections;
        QList<TextEditor::BaseTextEditorWidget::BlockRange> ifdefedOutBlocks;
    };

    QList<Editor> m_todo;

    QTimer *m_updateEditorSelectionsTimer;

    CppFindReferences *m_findReferences;
    bool m_indexerEnabled;

    mutable QMutex m_protectExtraDiagnostics;
    QHash<QString, QHash<int, QList<Document::DiagnosticMessage> > > m_extraDiagnostics;

    QMap<QString, QList<CppTools::ProjectPart::Ptr> > m_srcToProjectPart;

    CppCompletionAssistProvider *m_completionAssistProvider;
    CppCompletionAssistProvider *m_completionFallback;
    CppHighlightingSupportFactory *m_highlightingFactory;
    CppHighlightingSupportFactory *m_highlightingFallback;
    CppIndexingSupport *m_indexingSupporter;
    CppIndexingSupport *m_internalIndexingSupport;
};

class CPPTOOLS_EXPORT CppPreprocessor: public CPlusPlus::Client
{
    Q_DISABLE_COPY(CppPreprocessor)

public:
    CppPreprocessor(QPointer<CppModelManager> modelManager, bool dumpFileNameWhileParsing = false);
    virtual ~CppPreprocessor();

    void setRevision(unsigned revision);
    void setWorkingCopy(const CppTools::CppModelManagerInterface::WorkingCopy &workingCopy);
    void setIncludePaths(const QStringList &includePaths);
    void setFrameworkPaths(const QStringList &frameworkPaths);
    void addFrameworkPath(const QString &frameworkPath);
    void setProjectFiles(const QStringList &files);
    void setTodo(const QStringList &files);

    void run(const QString &fileName);
    void removeFromCache(const QString &fileName);

    void resetEnvironment();
    static QString cleanPath(const QString &path);

    const QSet<QString> &todo() const
    { return m_todo; }

    CppModelManager *modelManager() const
    { return m_modelManager.data(); }

protected:
    CPlusPlus::Document::Ptr switchDocument(CPlusPlus::Document::Ptr doc);

    void getFileContents(const QString &absoluteFilePath, QString *contents, unsigned *revision) const;
    bool checkFile(const QString &absoluteFilePath) const;
    QString resolveFile(const QString &fileName, IncludeType type);
    QString resolveFile_helper(const QString &fileName, IncludeType type);

    void mergeEnvironment(CPlusPlus::Document::Ptr doc);

    virtual void macroAdded(const CPlusPlus::Macro &macro);
    virtual void passedMacroDefinitionCheck(unsigned offset, unsigned line,
                                            const CPlusPlus::Macro &macro);
    virtual void failedMacroDefinitionCheck(unsigned offset, const CPlusPlus::ByteArrayRef &name);
    virtual void notifyMacroReference(unsigned offset, unsigned line,
                                      const CPlusPlus::Macro &macro);
    virtual void startExpandingMacro(unsigned offset,
                                     unsigned line,
                                     const CPlusPlus::Macro &macro,
                                     const QVector<CPlusPlus::MacroArgumentReference> &actuals);
    virtual void stopExpandingMacro(unsigned offset, const CPlusPlus::Macro &macro);
    virtual void markAsIncludeGuard(const QByteArray &macroName);
    virtual void startSkippingBlocks(unsigned offset);
    virtual void stopSkippingBlocks(unsigned offset);
    virtual void sourceNeeded(unsigned line, const QString &fileName, IncludeType type);

private:
    CPlusPlus::Snapshot m_snapshot;
    QPointer<CppModelManager> m_modelManager;
    bool m_dumpFileNameWhileParsing;
    CPlusPlus::Environment m_env;
    CPlusPlus::Preprocessor m_preprocess;
    QStringList m_includePaths;
    CppTools::CppModelManagerInterface::WorkingCopy m_workingCopy;
    QStringList m_frameworkPaths;
    QSet<QString> m_included;
    CPlusPlus::Document::Ptr m_currentDoc;
    QSet<QString> m_todo;
    QSet<QString> m_processed;
    unsigned m_revision;
    QHash<QString, QString> m_fileNameCache;
};

} // namespace Internal
} // namespace CppTools

#endif // CPPMODELMANAGER_H
