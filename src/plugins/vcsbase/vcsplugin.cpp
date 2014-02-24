/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "vcsplugin.h"

#include "vcsbaseconstants.h"

#include "commonsettingspage.h"
#include "nicknamedialog.h"
#include "vcsbaseoutputwindow.h"
#include "corelistener.h"

#include <coreplugin/iversioncontrol.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/variablemanager.h>
#include <coreplugin/vcsmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>

#include <QtPlugin>
#include <QDebug>

namespace VcsBase {
namespace Internal {

VcsPlugin *VcsPlugin::m_instance = 0;

VcsPlugin::VcsPlugin() :
    m_settingsPage(0),
    m_nickNameModel(0),
    m_coreListener(0)
{
    m_instance = this;
}

VcsPlugin::~VcsPlugin()
{
    m_instance = 0;
}

bool VcsPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)

    if (!Core::MimeDatabase::addMimeTypes(QLatin1String(":/vcsbase/VcsBase.mimetypes.xml"), errorMessage))
        return false;

    m_coreListener = new CoreListener;
    addAutoReleasedObject(m_coreListener);

    m_settingsPage = new CommonOptionsPage;
    addAutoReleasedObject(m_settingsPage);
    addAutoReleasedObject(VcsBaseOutputWindow::instance());
    connect(m_settingsPage, SIGNAL(settingsChanged(VcsBase::Internal::CommonVcsSettings)),
            this, SIGNAL(settingsChanged(VcsBase::Internal::CommonVcsSettings)));
    connect(m_settingsPage, SIGNAL(settingsChanged(VcsBase::Internal::CommonVcsSettings)),
            this, SLOT(slotSettingsChanged()));
    slotSettingsChanged();

    connect(Core::VariableManager::instance(), SIGNAL(variableUpdateRequested(QByteArray)),
            this, SLOT(updateVariable(QByteArray)));

    Core::VariableManager::registerVariable(Constants::VAR_VCS_NAME,
        tr("Name of the version control system in use by the current project."));
    Core::VariableManager::registerVariable(Constants::VAR_VCS_TOPIC,
        tr("The current version control topic (branch or tag) identification of the current project."));

    return true;
}

void VcsPlugin::extensionsInitialized()
{
}

VcsPlugin *VcsPlugin::instance()
{
    return m_instance;
}

CoreListener *VcsPlugin::coreListener() const
{
    return m_coreListener;
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

void VcsPlugin::updateVariable(const QByteArray &variable)
{
    static ProjectExplorer::Project *cachedProject = 0;
    static Core::IVersionControl *cachedVc = 0;
    static QString cachedTopLevel;

    ProjectExplorer::Project *project = ProjectExplorer::ProjectExplorerPlugin::currentProject();
    if (cachedProject != project) {
        cachedVc = Core::VcsManager::findVersionControlForDirectory(project->projectDirectory(),
                                                                    &cachedTopLevel);
        cachedProject = project;
    }

    if (variable == Constants::VAR_VCS_NAME) {
        if (cachedVc)
            Core::VariableManager::insert(variable, cachedVc->displayName());
        else
            Core::VariableManager::remove(variable);
    } else if (variable == Constants::VAR_VCS_TOPIC) {
        if (cachedVc)
            Core::VariableManager::insert(variable, cachedVc->vcsTopic(cachedTopLevel));
        else
            Core::VariableManager::remove(variable);
    }
}

} // namespace Internal
} // namespace VcsBase

Q_EXPORT_PLUGIN(VcsBase::Internal::VcsPlugin)
