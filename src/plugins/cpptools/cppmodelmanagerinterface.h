/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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

namespace Internal {
class CppEditorSupport;
}

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

    virtual bool isCppEditor(Core::IEditor *editor) const = 0;

    virtual WorkingCopy workingCopy() const = 0;
    virtual CPlusPlus::Snapshot snapshot() const = 0;

    virtual QList<ProjectInfo> projectInfos() const = 0;
    virtual ProjectInfo projectInfo(ProjectExplorer::Project *project) const = 0;
    virtual void updateProjectInfo(const ProjectInfo &pinfo) = 0;

    virtual QStringList includesInPath(const QString &path) const = 0;

    virtual void addEditorSupport(AbstractEditorSupport *editorSupport) = 0;
    virtual void removeEditorSupport(AbstractEditorSupport *editorSupport) = 0;

    virtual QList<int> references(CPlusPlus::Symbol *symbol,
                                  const CPlusPlus::LookupContext &context) = 0;

    virtual void renameUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context,
                              const QString &replacement = QString()) = 0;
    virtual void findUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context) = 0;

    virtual void findMacroUsages(const CPlusPlus::Macro &macro) = 0;

Q_SIGNALS:
    void documentUpdated(CPlusPlus::Document::Ptr doc);

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

    static QString licenseTemplate(const QString &file = QString(), const QString &className = QString());

private:
    CppModelManagerInterface *m_modelmanager;
};

} // namespace CppTools

#endif // CPPMODELMANAGERINTERFACE_H
