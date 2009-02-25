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

#ifndef QT4PROJECTMANAGERPLUGIN_H
#define QT4PROJECTMANAGERPLUGIN_H

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>

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
