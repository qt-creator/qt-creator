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

#ifndef QMLPROJECT_H
#define QMLPROJECT_H

#include "qmlprojectmanager.h"
#include "qmlprojectmanager_global.h"
#include "qmlprojectnodes.h"
#include "qmlprojecttarget.h"

#include <projectexplorer/project.h>

#include <QtDeclarative/QmlEngine>

namespace QmlJSEditor {
class ModelManagerInterface;
}

namespace ProjectExplorer {
class FileWatcher;
}

namespace QmlProjectManager {

class QmlProjectItem;

namespace Internal {

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
    QString id() const;
    Core::IFile *file() const;
    Internal::Manager *projectManager() const;
    Internal::QmlProjectTargetFactory *targetFactory() const;
    Internal::QmlProjectTarget *activeTarget() const;

    QList<ProjectExplorer::Project *> dependsOn();

    bool isApplication() const;

    ProjectExplorer::BuildConfigWidget *createConfigWidget();
    QList<ProjectExplorer::BuildConfigWidget*> subConfigWidgets();

    Internal::QmlProjectNode *rootProjectNode() const;
    QStringList files(FilesMode fileMode) const;

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
    QStringList libraryPaths() const;

private slots:
    void refreshProjectFile();
    void refreshFiles();

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
    QmlJSEditor::ModelManagerInterface *m_modelManager;

    // plain format
    QStringList m_files;

    // qml based, new format
    QmlEngine m_engine;
    QWeakPointer<QmlProjectItem> m_projectItem;
    ProjectExplorer::FileWatcher *m_fileWatcher;

    Internal::QmlProjectNode *m_rootNode;
    Internal::QmlProjectTargetFactory *m_targetFactory;
};

} // namespace QmlProjectManager

Q_DECLARE_OPERATORS_FOR_FLAGS(QmlProjectManager::QmlProject::RefreshOptions)

#endif // QMLPROJECT_H
