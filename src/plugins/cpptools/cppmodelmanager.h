/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef CPPMODELMANAGER_H
#define CPPMODELMANAGER_H

#include <cpptools/cppmodelmanagerinterface.h>
#include <projectexplorer/project.h>
#include <cplusplus/CppDocument.h>

#include <texteditor/basetexteditor.h>

#include <QMap>
#include <QFutureInterface>
#include <QFutureSynchronizer>
#include <QMutex>
#include <QTimer>
#include <QTextEdit>

namespace Core {
class ICore;
class IEditor;
}

namespace TextEditor {
class ITextEditor;
class BaseTextEditor;
}

namespace ProjectExplorer {
class ProjectExplorerPlugin;
}

namespace CppTools {
namespace Internal {

class CppEditorSupport;
class CppPreprocessor;
class CppFindReferences;

class CppModelManager : public CppModelManagerInterface
{
    Q_OBJECT

public:
    CppModelManager(QObject *parent);
    virtual ~CppModelManager();

    virtual void updateSourceFiles(const QStringList &sourceFiles);
    virtual QMap<QString, QString> workingCopy() const;

    virtual QList<ProjectInfo> projectInfos() const;
    virtual ProjectInfo projectInfo(ProjectExplorer::Project *project) const;
    virtual void updateProjectInfo(const ProjectInfo &pinfo);

    virtual QStringList includesInPath(const QString &path) const;

    virtual CPlusPlus::Snapshot snapshot() const;
    virtual void GC();

    QFuture<void> refreshSourceFiles(const QStringList &sourceFiles);

    inline Core::ICore *core() const { return m_core; }

    bool isCppEditor(Core::IEditor *editor) const; // ### private

    CppEditorSupport *editorSupport(TextEditor::ITextEditor *editor) const
    { return m_editorSupport.value(editor); }

    void emitDocumentUpdated(CPlusPlus::Document::Ptr doc);

    void stopEditorSelectionsUpdate()
    { m_updateEditorSelectionsTimer->stop(); }

    virtual void addEditorSupport(AbstractEditorSupport *editorSupport);
    virtual void removeEditorSupport(AbstractEditorSupport *editorSupport);

    virtual QList<int> references(CPlusPlus::Symbol *symbol,
                                  CPlusPlus::Document::Ptr doc,
                                  const CPlusPlus::Snapshot &snapshot);

    virtual void findUsages(CPlusPlus::Symbol *symbol);
    virtual void renameUsages(CPlusPlus::Symbol *symbol);

    void setHeaderSuffixes(const QStringList &suffixes)
    { m_headerSuffixes = suffixes; }

Q_SIGNALS:
    void projectPathChanged(const QString &projectPath);

    void documentUpdated(CPlusPlus::Document::Ptr doc);
    void aboutToRemoveFiles(const QStringList &files);

public Q_SLOTS:
    void editorOpened(Core::IEditor *editor);
    void editorAboutToClose(Core::IEditor *editor);

private Q_SLOTS:
    // this should be executed in the GUI thread.
    void onDocumentUpdated(CPlusPlus::Document::Ptr doc);
    void onAboutToRemoveProject(ProjectExplorer::Project *project);
    void onAboutToUnloadSession();
    void onProjectAdded(ProjectExplorer::Project *project);
    void postEditorUpdate();
    void updateEditorSelections();

private:
    QMap<QString, QString> buildWorkingCopyList();

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

    void ensureUpdated();
    QStringList internalProjectFiles() const;
    QStringList internalIncludePaths() const;
    QStringList internalFrameworkPaths() const;
    QByteArray internalDefinedMacros() const;

    void setIncludesInPaths(const QMap<QString, QStringList> &includesInPaths);

    static void updateIncludesInPaths(QFutureInterface<void> &future,
                                      CppModelManager *manager,
                                      QStringList paths,
                                      QStringList frameworkPaths,
                                      QStringList suffixes);

    static void parse(QFutureInterface<void> &future,
                      CppPreprocessor *preproc,
                      QStringList files);

private:
    Core::ICore *m_core;
    CPlusPlus::Snapshot m_snapshot;

    // cache
    bool m_dirty;
    QStringList m_projectFiles;
    QStringList m_includePaths;
    QStringList m_frameworkPaths;
    QByteArray m_definedMacros;

    QMap<QString, QStringList> m_includesInPaths;
    QStringList m_headerSuffixes;

    // editor integration
    QMap<TextEditor::ITextEditor *, CppEditorSupport *> m_editorSupport;

    QSet<AbstractEditorSupport *> m_addtionalEditorSupport;

    // project integration
    QMap<ProjectExplorer::Project *, ProjectInfo> m_projects;

    mutable QMutex mutex;
    mutable QMutex protectSnapshot;

    struct Editor {
        Editor()
                : revision(-1) {}
        int revision;
        QPointer<TextEditor::ITextEditor> textEditor;
        QList<QTextEdit::ExtraSelection> selections;
        QList<TextEditor::BaseTextEditor::BlockRange> ifdefedOutBlocks;
    };

    QList<Editor> m_todo;

    QTimer *m_updateEditorSelectionsTimer;

    QFutureSynchronizer<void> m_synchronizer;
    unsigned m_revision;

    CppFindReferences *m_findReferences;
};

} // namespace Internal
} // namespace CppTools

#endif // CPPMODELMANAGER_H
