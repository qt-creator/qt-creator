/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "vcsplugin.h"
#include "diffhighlighter.h"
#include "vcsbasesettingspage.h"
#include "nicknamedialog.h"
#include "vcsbaseoutputwindow.h"
#include "corelistener.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/mimedatabase.h>

#include <QtCore/QtPlugin>
#include <QtCore/QDebug>

namespace VCSBase {
namespace Internal {

VCSPlugin *VCSPlugin::m_instance = 0;

VCSPlugin::VCSPlugin() :
    m_settingsPage(0),
    m_nickNameModel(0),
    m_coreListener(0)
{
    m_instance = this;
}

VCSPlugin::~VCSPlugin()
{
    m_instance = 0;
}

bool VCSPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    Core::ICore *core = Core::ICore::instance();
    if (!core->mimeDatabase()->addMimeTypes(QLatin1String(":/vcsbase/VCSBase.mimetypes.xml"), errorMessage))
        return false;

    m_coreListener = new CoreListener;
    addAutoReleasedObject(m_coreListener);

    m_settingsPage = new VCSBaseSettingsPage;
    addAutoReleasedObject(m_settingsPage);
    addAutoReleasedObject(VCSBaseOutputWindow::instance());
    connect(m_settingsPage, SIGNAL(settingsChanged(VCSBase::Internal::VCSBaseSettings)),
            this, SIGNAL(settingsChanged(VCSBase::Internal::VCSBaseSettings)));
    connect(m_settingsPage, SIGNAL(settingsChanged(VCSBase::Internal::VCSBaseSettings)),
            this, SLOT(slotSettingsChanged()));
    slotSettingsChanged();
    return true;
}

void VCSPlugin::extensionsInitialized()
{
}

VCSPlugin *VCSPlugin::instance()
{
    return m_instance;
}

CoreListener *VCSPlugin::coreListener() const
{
    return m_coreListener;
}

VCSBaseSettings VCSPlugin::settings() const
{
    return m_settingsPage->settings();
}

/* Delayed creation/update of the nick name model. */
QStandardItemModel *VCSPlugin::nickNameModel()
{
    if (!m_nickNameModel) {
        m_nickNameModel = NickNameDialog::createModel(this);
        populateNickNameModel();
    }
    return m_nickNameModel;
}

void VCSPlugin::populateNickNameModel()
{
    QString errorMessage;
    if (!NickNameDialog::populateModelFromMailCapFile(settings().nickNameMailMap,
                                                      m_nickNameModel,
                                                      &errorMessage)) {
            qWarning("%s", qPrintable(errorMessage));
        }
}

void VCSPlugin::slotSettingsChanged()
{
    if (m_nickNameModel)
        populateNickNameModel();
}

} // namespace Internal
} // namespace VCSBase

Q_EXPORT_PLUGIN(VCSBase::Internal::VCSPlugin)
