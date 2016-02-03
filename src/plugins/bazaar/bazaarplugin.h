/****************************************************************************
**
** Copyright (C) 2016 Hugues Delorme
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

#pragma once

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

protected:
    void updateActions(VcsBase::VcsBasePlugin::ActionState);
    bool submitEditorAboutToClose();

#ifdef WITH_TESTS
private slots:
    void testDiffFileResolving_data();
    void testDiffFileResolving();
    void testLogResolving();
#endif

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

    // Functions
    void createMenu(const Core::Context &context);
    void createSubmitEditorActions();
    void createFileActions(const Core::Context &context);
    void createDirectoryActions(const Core::Context &context);
    void createRepositoryActions(const Core::Context &context);

    // Variables
    static BazaarPlugin *m_instance;

    BazaarSettings m_bazaarSettings;
    BazaarClient *m_client = nullptr;

    Core::CommandLocator *m_commandLocator = nullptr;
    Core::ActionContainer *m_bazaarContainer = nullptr;

    QList<QAction *> m_repositoryActionList;

    // Menu Items (file actions)
    Utils::ParameterAction *m_addAction = nullptr;
    Utils::ParameterAction *m_deleteAction = nullptr;
    Utils::ParameterAction *m_annotateFile = nullptr;
    Utils::ParameterAction *m_diffFile = nullptr;
    Utils::ParameterAction *m_logFile = nullptr;
    Utils::ParameterAction *m_revertFile = nullptr;
    Utils::ParameterAction *m_statusFile = nullptr;

    // Submit editor actions
    QAction *m_editorCommit = nullptr;
    QAction *m_editorDiff = nullptr;
    QAction *m_editorUndo = nullptr;
    QAction *m_editorRedo = nullptr;
    QAction *m_menuAction = nullptr;

    QString m_submitRepository;
    bool m_submitActionTriggered = false;
};

} // namespace Internal
} // namespace Bazaar
