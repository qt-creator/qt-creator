/**************************************************************************
**
** Copyright (c) 2013 Hugues Delorme
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
#ifndef BAZAARPLUGIN_H
#define BAZAARPLUGIN_H

#include "bazaarsettings.h"

#include <vcsbase/vcsbaseclientsettings.h>
#include <vcsbase/vcsbaseclient.h>
#include <vcsbase/vcsbaseplugin.h>
#include <coreplugin/icontext.h>

#include <QFileInfo>
#include <QHash>
#include <qglobal.h>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Core {
class ActionManager;
class ActionContainer;
class Id;
class IVersionControl;
class IEditorFactory;
class IEditor;
} // namespace Core

namespace Utils {
class ParameterAction;
} //namespace Utils

namespace VcsBase {
class VcsBaseSubmitEditor;
}

namespace Locator {
class CommandLocator;
}

namespace Bazaar {
namespace Internal {

class OptionsPage;
class BazaarClient;
class BazaarControl;
class BazaarEditor;

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

    const BazaarSettings &settings() const;
    void setSettings(const BazaarSettings &settings);

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
    void diffFromEditorSelected(const QStringList &files);
#ifdef WITH_TESTS
    void testDiffFileResolving_data();
    void testDiffFileResolving();
    void testLogResolving();
#endif

protected:
    void updateActions(VcsBase::VcsBasePlugin::ActionState);
    bool submitEditorAboutToClose(VcsBase::VcsBaseSubmitEditor *submitEditor);

private:
    // Methods
    void createMenu();
    void createSubmitEditorActions();
    void createFileActions(const Core::Context &context);
    void createDirectoryActions(const Core::Context &context);
    void createRepositoryActions(const Core::Context &context);

    // Variables
    static BazaarPlugin *m_instance;
    BazaarSettings m_bazaarSettings;
    OptionsPage *m_optionsPage;
    BazaarClient *m_client;

    Locator::CommandLocator *m_commandLocator;
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
