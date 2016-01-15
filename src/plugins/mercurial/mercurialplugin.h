/****************************************************************************
**
** Copyright (C) 2016 Brian McGillion
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
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
class CommandLocator;
class ICore;
class Id;
class IEditor;
} // namespace Core

namespace Utils { class ParameterAction; }
namespace VcsBase { class VcsBaseSubmitEditor; }

namespace Mercurial {
namespace Internal {

class OptionsPage;
class MercurialClient;
class MercurialControl;
class MercurialEditorWidget;
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
    static MercurialClient *client() { return m_instance->m_client; }

#ifdef WITH_TESTS
private slots:
    void testDiffFileResolving_data();
    void testDiffFileResolving();
    void testLogResolving();
#endif

protected:
    void updateActions(VcsBase::VcsBasePlugin::ActionState);
    bool submitEditorAboutToClose();

private:
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

    void createMenu(const Core::Context &context);
    void createSubmitEditorActions();
    void createFileActions(const Core::Context &context);
    void createDirectoryActions(const Core::Context &context);
    void createRepositoryActions(const Core::Context &context);
    void createRepositoryManagementActions(const Core::Context &context);

    // Variables
    static MercurialPlugin *m_instance;
    OptionsPage *optionsPage;
    MercurialClient *m_client;

    Core::ICore *core;
    Core::CommandLocator *m_commandLocator;
    Core::ActionContainer *m_mercurialContainer;

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
