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

#ifndef QMLPROJECT_H
#define QMLPROJECT_H

#include "qmlprojectmanager_global.h"

#include <projectexplorer/project.h>

#include <QDeclarativeEngine>
#include <QPointer>

namespace ProjectExplorer { class RunConfiguration; }
namespace QmlJS { class ModelManagerInterface; }
namespace Utils { class FileSystemWatcher; }

namespace QmlProjectManager {

class QmlProjectItem;

namespace Internal {
class Manager;
class QmlProjectFile;
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
    Core::Id id() const;
    Core::IDocument *document() const;
    ProjectExplorer::IProjectManager *projectManager() const;

    bool supportsKit(ProjectExplorer::Kit *k, QString *errorMessage) const;

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
    QStringList customImportPaths() const;

    bool addFiles(const QStringList &filePaths);

    void refreshProjectFile();

    enum QmlImport { UnknownImport, QtQuick1Import, QtQuick2Import };
    QmlImport defaultImport() const;

private slots:
    void refreshFiles(const QSet<QString> &added, const QSet<QString> &removed);
    void addedTarget(ProjectExplorer::Target *target);
    void addedRunConfiguration(ProjectExplorer::RunConfiguration *);

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
    QmlImport m_defaultImport;
    QmlJS::ModelManagerInterface *m_modelManager;

    // plain format
    QStringList m_files;

    // qml based, new format
    QDeclarativeEngine m_engine;
    QPointer<QmlProjectItem> m_projectItem;

    Internal::QmlProjectNode *m_rootNode;
};

} // namespace QmlProjectManager

Q_DECLARE_OPERATORS_FOR_FLAGS(QmlProjectManager::QmlProject::RefreshOptions)

#endif // QMLPROJECT_H
