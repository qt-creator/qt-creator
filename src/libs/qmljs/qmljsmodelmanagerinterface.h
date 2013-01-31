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

#ifndef QMLJSMODELMANAGERINTERFACE_H
#define QMLJSMODELMANAGERINTERFACE_H

#include "qmljs_global.h"
#include "qmljsdocument.h"

#include <utils/environment.h>

#include <QObject>
#include <QStringList>
#include <QPointer>

namespace ProjectExplorer {
    class Project;
}

namespace QmlJS {

class Snapshot;

class QMLJS_EXPORT ModelManagerInterface: public QObject
{
    Q_OBJECT

public:
    class ProjectInfo
    {
    public:
        ProjectInfo()
            : tryQmlDump(false)
        { }

        ProjectInfo(QPointer<ProjectExplorer::Project> project)
            : project(project)
            , tryQmlDump(false)
        { }

        operator bool() const
        { return ! project.isNull(); }

        bool isValid() const
        { return ! project.isNull(); }

        bool isNull() const
        { return project.isNull(); }

    public: // attributes
        QPointer<ProjectExplorer::Project> project;
        QStringList sourceFiles;
        QStringList importPaths;

        // whether trying to run qmldump makes sense
        bool tryQmlDump;
        QString qmlDumpPath;
        ::Utils::Environment qmlDumpEnvironment;

        QString qtImportsPath;
        QString qtQmlPath;
        QString qtVersionString;
    };

    class WorkingCopy
    {
    public:
        typedef QHash<QString, QPair<QString, int> > Table;

        void insert(const QString &fileName, const QString &source, int revision = 0)
        { _elements.insert(fileName, qMakePair(source, revision)); }

        bool contains(const QString &fileName) const
        { return _elements.contains(fileName); }

        QString source(const QString &fileName) const
        { return _elements.value(fileName).first; }

        QPair<QString, int> get(const QString &fileName) const
        { return _elements.value(fileName); }

        Table all() const
        { return _elements; }

    private:
        Table _elements;
    };

    class CppData
    {
    public:
        QList<LanguageUtils::FakeMetaObject::ConstPtr> exportedTypes;
        QHash<QString, QString> contextProperties;
    };

    typedef QHash<QString, CppData> CppDataHash;

public:
    ModelManagerInterface(QObject *parent = 0);
    virtual ~ModelManagerInterface();

    static ModelManagerInterface *instance();

    virtual WorkingCopy workingCopy() const = 0;

    virtual QmlJS::Snapshot snapshot() const = 0;
    virtual QmlJS::Snapshot newestSnapshot() const = 0;

    virtual void updateSourceFiles(const QStringList &files,
                                   bool emitDocumentOnDiskChanged) = 0;
    virtual void fileChangedOnDisk(const QString &path) = 0;
    virtual void removeFiles(const QStringList &files) = 0;

    virtual QList<ProjectInfo> projectInfos() const = 0;
    virtual ProjectInfo projectInfo(ProjectExplorer::Project *project) const = 0;
    virtual void updateProjectInfo(const ProjectInfo &pinfo) = 0;
    Q_SLOT virtual void removeProjectInfo(ProjectExplorer::Project *project) = 0;

    virtual QStringList importPaths() const = 0;

    virtual void loadPluginTypes(const QString &libraryPath, const QString &importPath,
                                 const QString &importUri, const QString &importVersion) = 0;

    virtual CppDataHash cppData() const = 0;

    virtual LibraryInfo builtins(const Document::Ptr &doc) const = 0;

    // Blocks until all parsing threads are done. Used for testing.
    virtual void joinAllThreads() = 0;

public slots:
    virtual void resetCodeModel() = 0;

signals:
    void documentUpdated(QmlJS::Document::Ptr doc);
    void documentChangedOnDisk(QmlJS::Document::Ptr doc);
    void aboutToRemoveFiles(const QStringList &files);
    void libraryInfoUpdated(const QString &path, const QmlJS::LibraryInfo &info);
    void projectInfoUpdated(const ProjectInfo &pinfo);
};

} // namespace QmlJS

#endif // QMLJSMODELMANAGERINTERFACE_H
