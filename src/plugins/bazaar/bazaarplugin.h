/**************************************************************************
**
** Copyright (C) 2015 Hugues Delorme
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef BAZAARPLUGIN_H
#define BAZAARPLUGIN_H

#include "bazaarsettings.h"

#include <vcsbase/vcsbaseclient.h>
#include <vcsbase/vcsbaseplugin.h>
#include <coreplugin/icontext.h>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Core {
class ActionContainer;
class CommandLocator;
class Id;
} // namespace Core

namespace Utils { class ParameterAction; }

namespace Bazaar {
namespace Internal {

class OptionsPage;
class BazaarClient;
class BazaarControl;
class BazaarEditorWidget;

class BazaarPlugin : public VcsBase::VcsBasePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Bazaar.json")

public:
    BazaarPlugin();
    ~BazaarPlugin();
    bool initialize(const QStringList &arguments, QString *errorMessage);

    static BazaarPlugin *instance();
    BazaarClient *client() const;

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
    void revertAll();
    void statusMulti();

    // Repository menu action slots
    void pull();
    void push();
    void update();
    void commit();
    void showCommitWidget(const QList<VcsBase::VcsBaseClient::StatusItem> &status);
    void commitFromEditor();
    void uncommit();
    void diffFromEditorSelected(const QStringList &files);
#ifdef WITH_TESTS
    void testDiffFileResolving_data();
    void testDiffFileResolving();
    void testLogResolving();
#endif

protected:
    void updateActions(VcsBase::VcsBasePlugin::ActionState);
    bool submitEditorAboutToClose();

private:
    // Functions
    void createMenu(const Core::Context &context);
    void createSubmitEditorActions();
    void createFileActions(const Core::Context &context);
    void createDirectoryActions(const Core::Context &context);
    void createRepositoryActions(const Core::Context &context);

    // Variables
    static BazaarPlugin *m_instance;
    BazaarSettings m_bazaarSettings;
    OptionsPage *m_optionsPage;
    BazaarClient *m_client;

    Core::CommandLocator *m_commandLocator;
    Core::ActionContainer *m_bazaarContainer;

    QList<QAction *> m_repositoryActionList;

    // Menu Items (file actions)
    Utils::ParameterAction *m_addAction;
    Utils::ParameterAction *m_deleteAction;
    Utils::ParameterAction *m_annotateFile;
    Utils::ParameterAction *m_diffFile;
    Utils::ParameterAction *m_logFile;
    Utils::ParameterAction *m_renameFile;
    Utils::ParameterAction *m_revertFile;
    Utils::ParameterAction *m_statusFile;

    // Submit editor actions
    QAction *m_editorCommit;
    QAction *m_editorDiff;
    QAction *m_editorUndo;
    QAction *m_editorRedo;
    QAction *m_menuAction;

    QString m_submitRepository;
    bool m_submitActionTriggered;
};

} // namespace Internal
} // namespace Bazaar

#endif // BAZAARPLUGIN_H
