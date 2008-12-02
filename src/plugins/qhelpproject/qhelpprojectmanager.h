/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
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
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

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
