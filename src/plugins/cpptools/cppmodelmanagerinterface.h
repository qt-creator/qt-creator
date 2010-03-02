/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef CPPMODELMANAGERINTERFACE_H
#define CPPMODELMANAGERINTERFACE_H

#include <cpptools/cpptools_global.h>
#include <cplusplus/CppDocument.h>
#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QPointer>
#include <QtCore/QStringList>
#include <QtCore/QFuture>

namespace ProjectExplorer {
    class Project;
}

namespace CppTools {

class AbstractEditorSupport;

class CPPTOOLS_EXPORT CppModelManagerInterface : public QObject
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
    CppModelManagerInterface(QObject *parent = 0) : QObject(parent) {}
    virtual ~CppModelManagerInterface() {}

    static CppModelManagerInterface *instance();

    virtual WorkingCopy workingCopy() const = 0;
    virtual CPlusPlus::Snapshot snapshot() const = 0;

    virtual QList<ProjectInfo> projectInfos() const = 0;
    virtual ProjectInfo projectInfo(ProjectExplorer::Project *project) const = 0;
    virtual void updateProjectInfo(const ProjectInfo &pinfo) = 0;

    virtual QStringList includesInPath(const QString &path) const = 0;

    virtual void addEditorSupport(AbstractEditorSupport *editorSupport) = 0;
    virtual void removeEditorSupport(AbstractEditorSupport *editorSupport) = 0;

    virtual QList<int> references(CPlusPlus::Symbol *symbol,
                                  CPlusPlus::Document::Ptr doc,
                                  const CPlusPlus::Snapshot &snapshot) = 0;

    virtual void renameUsages(CPlusPlus::Document::Ptr symbolDocument, CPlusPlus::Symbol *symbol) = 0;
    virtual void findUsages(CPlusPlus::Document::Ptr symbolDocument, CPlusPlus::Symbol *symbol) = 0;

    virtual void findMacroUsages(const CPlusPlus::Macro &macro) = 0;

public Q_SLOTS:
    void updateModifiedSourceFiles();
    virtual QFuture<void> updateSourceFiles(const QStringList &sourceFiles) = 0;
    virtual void GC() = 0;
};

class CPPTOOLS_EXPORT AbstractEditorSupport
{
public:
    explicit AbstractEditorSupport(CppModelManagerInterface *modelmanager);
    virtual ~AbstractEditorSupport();

    virtual QByteArray contents() const = 0;
    virtual QString fileName() const = 0;

    void updateDocument();

    // TODO: find a better place for common utility functions
    static QString functionAt(const CppModelManagerInterface *mm,
                              const QString &fileName,
                              int line, int column);

    static QString licenseTemplate();

private:
    CppModelManagerInterface *m_modelmanager;
};

} // namespace CppTools

#endif // CPPMODELMANAGERINTERFACE_H
