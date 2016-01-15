/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "vcsplugin.h"

#include "vcsbaseconstants.h"
#include "vcsbasesubmiteditor.h"

#include "commonsettingspage.h"
#include "nicknamedialog.h"
#include "vcsoutputwindow.h"
#include "vcsprojectcache.h"
#include "wizard/vcscommandpage.h"
#include "wizard/vcsconfigurationpage.h"
#include "wizard/vcsjsextension.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/jsexpander.h>
#include <coreplugin/vcsmanager.h>

#include <projectexplorer/jsonwizard/jsonwizardfactory.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projecttree.h>

#include <utils/macroexpander.h>

#include <QtPlugin>
#include <QDebug>

using namespace Core;
using namespace ProjectExplorer;

namespace VcsBase {
namespace Internal {

VcsPlugin *VcsPlugin::m_instance = 0;

VcsPlugin::VcsPlugin() :
    m_settingsPage(0),
    m_nickNameModel(0)
{
    m_instance = this;
}

VcsPlugin::~VcsPlugin()
{
    VcsProjectCache::destroy();
    m_instance = 0;
}

bool VcsPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    EditorManager::addCloseEditorListener([this](IEditor *editor) -> bool {
        bool result = true;
        if (auto se = qobject_cast<VcsBaseSubmitEditor *>(editor))
            emit submitEditorAboutToClose(se, &result);
        return result;
    });

    m_settingsPage = new CommonOptionsPage;
    addAutoReleasedObject(m_settingsPage);
    addAutoReleasedObject(VcsOutputWindow::instance());
    connect(m_settingsPage, &CommonOptionsPage::settingsChanged,
            this, &VcsPlugin::settingsChanged);
    connect(m_settingsPage, &CommonOptionsPage::settingsChanged,
            this, &VcsPlugin::slotSettingsChanged);
    slotSettingsChanged();

    JsonWizardFactory::registerPageFactory(new Internal::VcsConfigurationPageFactory);
    JsonWizardFactory::registerPageFactory(new Internal::VcsCommandPageFactory);

    JsExpander::registerQObjectForJs(QLatin1String("Vcs"), new VcsJsExtension);

    Utils::MacroExpander *expander = Utils::globalMacroExpander();
    expander->registerVariable(Constants::VAR_VCS_NAME,
        tr("Name of the version control system in use by the current project."),
        []() -> QString {
            IVersionControl *vc = 0;
            if (Project *project = ProjectTree::currentProject())
                vc = VcsManager::findVersionControlForDirectory(project->projectDirectory().toString());
            return vc ? vc->displayName() : QString();
        });

    expander->registerVariable(Constants::VAR_VCS_TOPIC,
        tr("The current version control topic (branch or tag) identification of the current project."),
        []() -> QString {
            IVersionControl *vc = 0;
            QString topLevel;
            if (Project *project = ProjectTree::currentProject())
                vc = VcsManager::findVersionControlForDirectory(project->projectDirectory().toString(), &topLevel);
            return vc ? vc->vcsTopic(topLevel) : QString();
        });

    expander->registerVariable(Constants::VAR_VCS_TOPLEVELPATH,
        tr("The top level path to the repository the current project is in."),
        []() -> QString {
            if (Project *project = ProjectTree::currentProject())
                return VcsManager::findTopLevelForDirectory(project->projectDirectory().toString());
            return QString();
        });

    return true;
}

void VcsPlugin::extensionsInitialized()
{
    VcsProjectCache::create();
}

VcsPlugin *VcsPlugin::instance()
{
    return m_instance;
}

CommonVcsSettings VcsPlugin::settings() const
{
    return m_settingsPage->settings();
}

/* Delayed creation/update of the nick name model. */
QStandardItemModel *VcsPlugin::nickNameModel()
{
    if (!m_nickNameModel) {
        m_nickNameModel = NickNameDialog::createModel(this);
        populateNickNameModel();
    }
    return m_nickNameModel;
}

void VcsPlugin::populateNickNameModel()
{
    QString errorMessage;
    if (!NickNameDialog::populateModelFromMailCapFile(settings().nickNameMailMap,
                                                      m_nickNameModel,
                                                      &errorMessage)) {
        qWarning("%s", qPrintable(errorMessage));
    }
}

void VcsPlugin::slotSettingsChanged()
{
    if (m_nickNameModel)
        populateNickNameModel();
}

} // namespace Internal
} // namespace VcsBase
