/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QMLPROJECT_H
#define QMLPROJECT_H

#include "qmlprojectmanager_global.h"

#include <projectexplorer/project.h>

#include <QtDeclarative/QDeclarativeEngine>

namespace QmlJS {
class ModelManagerInterface;
}

namespace Utils {
class FileSystemWatcher;
}

namespace QmlProjectManager {

class QmlProjectItem;

namespace Internal {
class Manager;
class QmlProjectFile;
class QmlProjectTarget;
class QmlProjectNode;
} // namespace Internal

class QMLPROJECTMANAGER_EXPORT QmlProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    QmlProject(Internal::Manager *manager, const QString &filename);
    virtual ~QmlProject();

    QString filesFileName() const;

    QString displayName() const;
    QString id() const;
    Core::IFile *file() const;
    ProjectExplorer::IProjectManager *projectManager() const;
    Internal::QmlProjectTarget *activeTarget() const;

    QList<ProjectExplorer::Project *> dependsOn();

    QList<ProjectExplorer::BuildConfigWidget*> subConfigWidgets();

    ProjectExplorer::ProjectNode *rootProjectNode() const;
    QStringList files(FilesMode fileMode) const;

    bool validProjectFile() const;

    enum RefreshOption {
        ProjectFile   = 0x01,
        Files         = 0x02,
        Configuration = 0x04,
        Everything    = ProjectFile | Files | Configuration
    };
    Q_DECLARE_FLAGS(RefreshOptions,RefreshOption)

    void refresh(RefreshOptions options);

    QDir projectDir() const;
    QStringList files() const;
    QString mainFile() const;
    QStringList importPaths() const;

    bool addFiles(const QStringList &filePaths);

    void refreshProjectFile();

private slots:
    void refreshFiles(const QSet<QString> &added, const QSet<QString> &removed);

protected:
    bool fromMap(const QVariantMap &map);

private:
    // plain format
    void parseProject(RefreshOptions options);
    QStringList convertToAbsoluteFiles(const QStringList &paths) const;

    Internal::Manager *m_manager;
    QString m_fileName;
    Internal::QmlProjectFile *m_file;
    QString m_projectName;
    QmlJS::ModelManagerInterface *m_modelManager;

    // plain format
    QStringList m_files;

    // qml based, new format
    QDeclarativeEngine m_engine;
    QWeakPointer<QmlProjectItem> m_projectItem;

    Internal::QmlProjectNode *m_rootNode;
};

} // namespace QmlProjectManager

Q_DECLARE_OPERATORS_FOR_FLAGS(QmlProjectManager::QmlProject::RefreshOptions)

#endif // QMLPROJECT_H
