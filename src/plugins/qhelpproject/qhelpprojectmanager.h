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

#ifndef QHELPPROJECTMANAGER_H
#define QHELPPROJECTMANAGER_H

#include <QtCore/QObject>
#include <QtCore/QMap>

#include <extensionsystem/plugininterface.h>
#include <projectexplorer/ProjectExplorerInterfaces>

namespace Core {
class ICore;
}

namespace QHelpProjectPlugin {
namespace Internal {

class QHelpProjectManager : public QObject,
    public ExtensionSystem::PluginInterface,
    public ProjectExplorer::IProjectManager
{
    Q_OBJECT
    Q_CLASSINFO("RequiredPlugin", "ProjectExplorer")
    Q_INTERFACES(ExtensionSystem::PluginInterface
        ProjectExplorer::IProjectManager)

public:
    QHelpProjectManager();
    ~QHelpProjectManager();

    bool init(ExtensionSystem::PluginManager *pm, QString *error_message = 0);
    void extensionsInitialized();
    void cleanup();

    int projectContext() const;
    int projectLanguage() const;

    bool canOpen(const QString &fileName);
    QList<ProjectExplorer::ProjectInterface*> open(const QString &fileName);
    QString fileFilter() const;

    QVariant editorProperty(const QString &key) const;

    ProjectExplorer::ProjectExplorerInterface *projectExplorer() const;

private:
    ExtensionSystem::PluginManager *m_pm;
    QWorkbench::ICore *m_core;
    ProjectExplorer::ProjectExplorerInterface *m_projectExplorer;

    int m_projectContext;
    int m_languageId;
};

} // namespace Internal
} // namespace QHelpProjectPlugin

#endif // QHELPPROJECTMANAGER_H
