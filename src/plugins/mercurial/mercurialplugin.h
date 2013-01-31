/**************************************************************************
**
** Copyright (c) 2013 Brian McGillion
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

#ifndef MERCURIALPLUGIN_H
#define MERCURIALPLUGIN_H

#include "mercurialsettings.h"

#include <vcsbase/vcsbaseclient.h>
#include <vcsbase/vcsbaseplugin.h>
#include <coreplugin/icontext.h>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Core {
class ActionContainer;
class ActionManager;
class ICore;
class Id;
class IEditor;
} // namespace Core

namespace Utils { class ParameterAction; }
namespace VcsBase { class VcsBaseSubmitEditor; }
namespace Locator { class CommandLocator; }

namespace Mercurial {
namespace Internal {

class OptionsPage;
class MercurialClient;
class MercurialControl;
class MercurialEditor;
class MercurialSettings;

class MercurialPlugin : public VcsBase::VcsBasePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Mercurial.json")

public:
    MercurialPlugin();
    ~MercurialPlugin();
    bool initialize(const QStringList &arguments, QString *errorMessage);

    static MercurialPlugin *instance() { return m_instance; }
    MercurialClient *client() const { return m_client; }

    const MercurialSettings &settings() const;
    void setSettings(const MercurialSettings &settings);

private slots:
    // File menu action slots
    void addCurrentFile();
    void annotateCurrentFile();
    void diffCurrentFile();
    void logCurrentFile();
    void revertCurrentFile();
    void statusCurrentFile();

    // Directory menu action slots
    void diffRepository();
    void logRepository();
    void revertMulti();
    void statusMulti();

    // Repository menu action slots
    void pull();
    void push();
    void update();
    void import();
    void incoming();
    void outgoing();
    void commit();
    void showCommitWidget(const QList<VcsBase::VcsBaseClient::StatusItem> &status);
    void commitFromEditor();
    void diffFromEditorSelected(const QStringList &files);
#ifdef WITH_TESTS
    void testDiffFileResolving_data();
    void testDiffFileResolving();
    void testLogResolving();
#endif

    //TODO implement
   /* //repository management action slots
    void merge();
    void branch();
    void heads();
    void parents();
    void tags();
    void tip();
    void paths();

    //less used repository action
    void init();
    void serve();*/

protected:
    void updateActions(VcsBase::VcsBasePlugin::ActionState);
    bool submitEditorAboutToClose(VcsBase::VcsBaseSubmitEditor *submitEditor);

private:
    void createMenu();
    void createSubmitEditorActions();
    void createFileActions(const Core::Context &context);
    void createDirectoryActions(const Core::Context &context);
    void createRepositoryActions(const Core::Context &context);
    void createRepositoryManagementActions(const Core::Context &context);
    void createLessUsedActions(const Core::Context &context);

    // Variables
    static MercurialPlugin *m_instance;
    MercurialSettings mercurialSettings;
    OptionsPage *optionsPage;
    MercurialClient *m_client;

    Core::ICore *core;
    Locator::CommandLocator *m_commandLocator;
    Core::ActionContainer *mercurialContainer;

    QList<QAction *> m_repositoryActionList;

    // Menu items (file actions)
    Utils::ParameterAction *m_addAction;
    Utils::ParameterAction *m_deleteAction;
    Utils::ParameterAction *annotateFile;
    Utils::ParameterAction *diffFile;
    Utils::ParameterAction *logFile;
    Utils::ParameterAction *renameFile;
    Utils::ParameterAction *revertFile;
    Utils::ParameterAction *statusFile;

    QAction *m_createRepositoryAction;
    // Submit editor actions
    QAction *editorCommit;
    QAction *editorDiff;
    QAction *editorUndo;
    QAction *editorRedo;
    QAction *m_menuAction;

    QString m_submitRepository;

    bool m_submitActionTriggered;
};

} // namespace Internal
} // namespace Mercurial

#endif // MERCURIALPLUGIN_H
