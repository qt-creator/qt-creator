/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
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

#ifndef MERCURIALPLUGIN_H
#define MERCURIALPLUGIN_H

#include "mercurialsettings.h"

#include <vcsbase/vcsbaseclient.h>
#include <vcsbase/vcsbaseplugin.h>
#include <coreplugin/icontext.h>

QT_BEGIN_NAMESPACE
class QAction;
class QTemporaryFile;
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
    virtual ~MercurialPlugin();
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
    virtual void updateActions(VcsBase::VcsBasePlugin::ActionState);
    virtual bool submitEditorAboutToClose(VcsBase::VcsBaseSubmitEditor *submitEditor);

private:
    void createMenu();
    void createSubmitEditorActions();
    void createFileActions(const Core::Context &context);
    void createDirectoryActions(const Core::Context &context);
    void createRepositoryActions(const Core::Context &context);
    void createRepositoryManagementActions(const Core::Context &context);
    void createLessUsedActions(const Core::Context &context);
    void deleteCommitLog();

    // Variables
    static MercurialPlugin *m_instance;
    MercurialSettings mercurialSettings;
    OptionsPage *optionsPage;
    MercurialClient *m_client;

    Core::ICore *core;
    Locator::CommandLocator *m_commandLocator;
    Core::ActionContainer *mercurialContainer;

    QList<QAction *> m_repositoryActionList;
    QTemporaryFile *changeLog;

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
};

} // namespace Internal
} // namespace Mercurial

#endif // MERCURIALPLUGIN_H
