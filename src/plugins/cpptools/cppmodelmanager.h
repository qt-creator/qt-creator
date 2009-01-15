/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef CPPMODELMANAGER_H
#define CPPMODELMANAGER_H

#include <cpptools/cppmodelmanagerinterface.h>
#include <projectexplorer/project.h>
#include <cplusplus/CppDocument.h>

#include <QMap>
#include <QFutureInterface>
#include <QMutex>

namespace Core {
class ICore;
class IEditor;
}

namespace TextEditor {
class ITextEditor;
}

namespace ProjectExplorer {
class ProjectExplorerPlugin;
}

namespace CppTools {
namespace Internal {

class CppEditorSupport;
class CppPreprocessor;

class CppModelManager : public CppModelManagerInterface
{
    Q_OBJECT

public:
    CppModelManager(QObject *parent);
    virtual ~CppModelManager();

    virtual void updateSourceFiles(const QStringList &sourceFiles);

    virtual QList<ProjectInfo> projectInfos() const;
    virtual ProjectInfo projectInfo(ProjectExplorer::Project *project) const;
    virtual void updateProjectInfo(const ProjectInfo &pinfo);

    virtual CPlusPlus::Snapshot snapshot() const;
    virtual void GC();

    QFuture<void> refreshSourceFiles(const QStringList &sourceFiles);

    inline Core::ICore *core() const { return m_core; }

    bool isCppEditor(Core::IEditor *editor) const; // ### private

    void emitDocumentUpdated(CPlusPlus::Document::Ptr doc);

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
    void onSessionUnloaded();
    void onProjectAdded(ProjectExplorer::Project *project);

private:
    QMap<QString, QByteArray> buildWorkingCopyList();

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

    static void parse(QFutureInterface<void> &future,
                      CppPreprocessor *preproc,
                      QStringList files);

private:
    Core::ICore *m_core;
    ProjectExplorer::ProjectExplorerPlugin *m_projectExplorer;
    CPlusPlus::Snapshot m_snapshot;

    // cache
    bool m_dirty;
    QStringList m_projectFiles;
    QStringList m_includePaths;
    QStringList m_frameworkPaths;
    QByteArray m_definedMacros;

    // editor integration
    QMap<TextEditor::ITextEditor *, CppEditorSupport *> m_editorSupport;

    // project integration
    QMap<ProjectExplorer::Project *, ProjectInfo> m_projects;

    mutable QMutex mutex;

    enum {
        MAX_SELECTION_COUNT = 5
    };
};

} // namespace Internal
} // namespace CppTools

#endif // CPPMODELMANAGER_H
