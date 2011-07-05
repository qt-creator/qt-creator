/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef CPPMODELMANAGERINTERFACE_H
#define CPPMODELMANAGERINTERFACE_H

#include <cplusplus/CppDocument.h>
#include <languageutils/fakemetaobject.h>
#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QPointer>
#include <QtCore/QStringList>
#include <QtCore/QFuture>

namespace Core {
    class IEditor;
}

namespace CPlusPlus {
    class LookupContext;
}

namespace ProjectExplorer {
    class Project;
}

namespace TextEditor {
    class ITextEditor;
}

namespace CppTools {
    class AbstractEditorSupport;
}

namespace CPlusPlus {

class CPLUSPLUS_EXPORT CppModelManagerInterface : public QObject
{
    Q_OBJECT

public:
    class ProjectInfo
    {
    public:
        ProjectInfo()
        { }

        ProjectInfo(QPointer<ProjectExplorer::Project> project)
            : project(project)
        { }

        operator bool() const
        { return ! project.isNull(); }

        bool isValid() const
        { return ! project.isNull(); }

        bool isNull() const
        { return project.isNull(); }

    public: // attributes
        QPointer<ProjectExplorer::Project> project;
        QString projectPath;
        QByteArray defines;
        QStringList sourceFiles;
        QStringList includePaths;
        QStringList frameworkPaths;
        QStringList precompiledHeaders;
    };

    class WorkingCopy
    {
    public:
        void insert(const QString &fileName, const QString &source, unsigned revision = 0)
        { _elements.insert(fileName, qMakePair(source, revision)); }

        bool contains(const QString &fileName) const
        { return _elements.contains(fileName); }

        QString source(const QString &fileName) const
        { return _elements.value(fileName).first; }

        QPair<QString, unsigned> get(const QString &fileName) const
        { return _elements.value(fileName); }

    private:
        typedef QHash<QString, QPair<QString, unsigned> > Table;
        Table _elements;
    };

public:
    CppModelManagerInterface(QObject *parent = 0);
    virtual ~CppModelManagerInterface();

    static CppModelManagerInterface *instance();

    virtual bool isCppEditor(Core::IEditor *editor) const = 0;

    virtual WorkingCopy workingCopy() const = 0;
    virtual CPlusPlus::Snapshot snapshot() const = 0;

    virtual QList<ProjectInfo> projectInfos() const = 0;
    virtual ProjectInfo projectInfo(ProjectExplorer::Project *project) const = 0;
    virtual void updateProjectInfo(const ProjectInfo &pinfo) = 0;

    virtual void addEditorSupport(CppTools::AbstractEditorSupport *editorSupport) = 0;
    virtual void removeEditorSupport(CppTools::AbstractEditorSupport *editorSupport) = 0;

    virtual QList<int> references(CPlusPlus::Symbol *symbol,
                                  const CPlusPlus::LookupContext &context) = 0;

    virtual void renameUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context,
                              const QString &replacement = QString()) = 0;
    virtual void findUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context) = 0;

    virtual void findMacroUsages(const CPlusPlus::Macro &macro) = 0;

    virtual QList<LanguageUtils::FakeMetaObject::ConstPtr> exportedQmlObjects(const CPlusPlus::Document::Ptr &doc) const = 0;

Q_SIGNALS:
    void documentUpdated(CPlusPlus::Document::Ptr doc);
    void sourceFilesRefreshed(const QStringList &files);

public Q_SLOTS:
    virtual void updateModifiedSourceFiles() = 0;
    virtual QFuture<void> updateSourceFiles(const QStringList &sourceFiles) = 0;
    virtual void GC() = 0;
};

} // namespace CPlusPlus

#endif // CPPMODELMANAGERINTERFACE_H
