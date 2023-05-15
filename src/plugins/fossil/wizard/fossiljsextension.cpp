// Copyright (c) 2018 Artur Shepilko
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

FossilJsExtension::FossilJsExtension() = default;

FossilJsExtension::~FossilJsExtension() = default;

bool FossilJsExtension::isConfigured() const
{
    IVersionControl *vc = VcsManager::versionControl(Constants::VCS_ID_FOSSIL);
    return vc && vc->isConfigured();
}

QString FossilJsExtension::displayName() const
{
    IVersionControl *vc = VcsManager::versionControl(Constants::VCS_ID_FOSSIL);
    return vc ? vc->displayName() : QString();
}

QString FossilJsExtension::defaultAdminUser() const
{
    if (!isConfigured())
        return QString();

    return settings().userName.value();
}

QString FossilJsExtension::defaultSslIdentityFile() const
{
    if (!isConfigured())
        return QString();

    return settings().sslIdentityFile.value();
}

QString FossilJsExtension::defaultLocalRepoPath() const
{
    if (!isConfigured())
        return QString();

    return settings().defaultRepoPath.value();
}

bool FossilJsExtension::defaultDisableAutosync() const
{
    if (!isConfigured())
        return false;

    return settings().disableAutosync.value();
}

} // namespace Internal
} // namespace Fossil

