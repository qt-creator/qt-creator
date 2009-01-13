/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef QT4PROJECTMANAGERPLUGIN_H
#define QT4PROJECTMANAGERPLUGIN_H

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <coreplugin/icore.h>

namespace Qt4ProjectManager {

class Qt4Manager;

namespace Internal {

class ProFileEditorFactory;
class ConsoleAppWizard;
class GuiAppWizard;
class QMakeBuildStepFactory;
class MakeBuildStepFactory;
class GccParserFactory;
class MsvcParserFactory;
class QtVersionManager;
class EmbeddedPropertiesPage;

class Qt4ProjectManagerPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    ~Qt4ProjectManagerPlugin();
    bool initialize(const QStringList &arguments, QString *error_message);
    void extensionsInitialized();

    int projectContext() const { return m_projectContext; }
    QtVersionManager *versionManager() const;


private slots:
    void updateContextMenu(ProjectExplorer::Project *project,
                           ProjectExplorer::Node *node);
    void currentProjectChanged();
    void buildStateChanged(ProjectExplorer::Project *pro);

#ifdef WITH_TESTS
    void testBasicProjectLoading();
    void testNotYetImplemented();
#endif

private:
    Core::ICore *m_core;
    ProjectExplorer::ProjectExplorerPlugin *m_projectExplorer;
    ProFileEditorFactory *m_proFileEditorFactory;
    Qt4Manager *m_qt4ProjectManager;
    QtVersionManager *m_qtVersionManager;
    EmbeddedPropertiesPage *m_embeddedPropertiesPage;

    int m_projectContext;

    QAction *m_runQMakeAction;
    QAction *m_runQMakeActionContextMenu;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // QT4PROJECTMANAGERPLUGIN_H
