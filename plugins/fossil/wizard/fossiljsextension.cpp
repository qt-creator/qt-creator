/****************************************************************************
**
** Copyright (c) 2018 Artur Shepilko
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

#include "fossiljsextension.h"
#include "../constants.h"
#include "../fossilclient.h"
#include "../fossilplugin.h"

#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

#include <vcsbase/vcsbaseclientsettings.h>
#include <vcsbase/vcsbaseconstants.h>

using namespace Core;

namespace Fossil {
namespace Internal {


class FossilJsExtensionPrivate {

public:
    FossilJsExtensionPrivate() :
        m_vscId(Constants::VCS_ID_FOSSIL) { }

    FossilClient *client() const {
        return FossilPlugin::instance()->client();
    }

    Core::Id m_vscId;
};


void FossilJsExtension::parseArgOptions(const QStringList &args, QMap<QString, QString> &options)
{
    options.clear();

    foreach (const QString &arg, args) {
        if (arg.isEmpty()) continue;

        QStringList opt = arg.split('|', QString::KeepEmptyParts);
        options.insert(opt[0], opt.size() > 1 ? opt[1] : QString());
    }
}

FossilJsExtension::FossilJsExtension() :
    d(new FossilJsExtensionPrivate)
{ }

FossilJsExtension::~FossilJsExtension()
{
    delete d;
}

bool FossilJsExtension::isConfigured() const
{
    IVersionControl *vc = VcsManager::versionControl(d->m_vscId);
    return vc && vc->isConfigured();
}

QString FossilJsExtension::displayName() const
{
    IVersionControl *vc = VcsManager::versionControl(d->m_vscId);
    return vc ? vc->displayName() : QString();
}

QString FossilJsExtension::defaultAdminUser() const
{
    if (!isConfigured())
        return QString();

    VcsBase::VcsBaseClientSettings &settings = d->client()->settings();
    return settings.stringValue(FossilSettings::userNameKey);
}

QString FossilJsExtension::defaultSslIdentityFile() const
{
    if (!isConfigured())
        return QString();

    VcsBase::VcsBaseClientSettings &settings = d->client()->settings();
    return settings.stringValue(FossilSettings::sslIdentityFileKey);
}

QString FossilJsExtension::defaultLocalRepoPath() const
{
    if (!isConfigured())
        return QString();

    VcsBase::VcsBaseClientSettings &settings = d->client()->settings();
    return settings.stringValue(FossilSettings::defaultRepoPathKey);
}

bool FossilJsExtension::defaultDisableAutosync() const
{
    if (!isConfigured())
        return false;

    VcsBase::VcsBaseClientSettings &settings = d->client()->settings();
    return settings.boolValue(FossilSettings::disableAutosyncKey);
}

} // namespace Internal
} // namespace Fossil

