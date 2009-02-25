/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef CPPMODELMANAGERINTERFACE_H
#define CPPMODELMANAGERINTERFACE_H

#include <cpptools/cpptools_global.h>
#include <cplusplus/CppDocument.h>
#include <QtCore/QObject>
#include <QtCore/QMap>
#include <QtCore/QPointer>

namespace ProjectExplorer {
    class Project;
}

namespace CppTools {

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
    };

public:
    CppModelManagerInterface(QObject *parent = 0) : QObject(parent) {}
    virtual ~CppModelManagerInterface() {}

    virtual void GC() = 0;
    virtual void updateSourceFiles(const QStringList &sourceFiles) = 0;

    virtual CPlusPlus::Snapshot snapshot() const = 0;

    virtual QList<ProjectInfo> projectInfos() const = 0;
    virtual ProjectInfo projectInfo(ProjectExplorer::Project *project) const = 0;
    virtual void updateProjectInfo(const ProjectInfo &pinfo) = 0;
};

} // namespace CppTools

#endif // CPPMODELMANAGERINTERFACE_H
