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
    FossilJsExtensionPrivate(FossilSettings *settings) :
        m_vscId(Constants::VCS_ID_FOSSIL),
        m_settings(settings)
    {
    }

    Utils::Id m_vscId;
    FossilSettings *m_settings;
};


QMap<QString, QString> FossilJsExtension::parseArgOptions(const QStringList &args)
{
    QMap<QString, QString> options;
    for (const QString &arg : args) {
        if (arg.isEmpty())
            continue;
        const QStringList opt = arg.split('|', Qt::KeepEmptyParts);
        options.insert(opt[0], opt.size() > 1 ? opt[1] : QString());
    }
    return options;
}

FossilJsExtension::FossilJsExtension(FossilSettings *settings) :
    d(new FossilJsExtensionPrivate(settings))
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

    return d->m_settings->userName.value();
}

QString FossilJsExtension::defaultSslIdentityFile() const
{
    if (!isConfigured())
        return QString();

    return d->m_settings->sslIdentityFile.value();
}

QString FossilJsExtension::defaultLocalRepoPath() const
{
    if (!isConfigured())
        return QString();

    return d->m_settings->defaultRepoPath.value();
}

bool FossilJsExtension::defaultDisableAutosync() const
{
    if (!isConfigured())
        return false;

    return d->m_settings->disableAutosync.value();
}

} // namespace Internal
} // namespace Fossil

