// Copyright (c) 2018 Artur Shepilko
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fossiljsextension.h"
#include "../constants.h"
#include "../fossilclient.h"

#include <coreplugin/iversioncontrol.h>
#include <coreplugin/jsbind.h>
#include <coreplugin/vcsmanager.h>

#include <memory>

using namespace Core;

namespace Fossil::Internal {

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
    return isConfigured() ? settings().userName() : QString();
}

QString FossilJsExtension::defaultSslIdentityFile() const
{
    return isConfigured() ? settings().sslIdentityFile().toFSPathString() : QString();
}

QString FossilJsExtension::defaultLocalRepoPath() const
{
    return isConfigured() ? settings().defaultRepoPath().toFSPathString() : QString();
}

bool FossilJsExtension::defaultDisableAutosync() const
{
    if (!isConfigured())
        return false;

    return settings().disableAutosync();
}

void registerFossilFunctions(JsBind::Registrar &registrar)
{
    const auto ext = std::make_shared<FossilJsExtension>();
    registrar.set("isConfigured", [ext] { return ext->isConfigured(); });
    registrar.set("displayName", [ext] { return ext->displayName(); });
    registrar.set("defaultAdminUser", [ext] { return ext->defaultAdminUser(); });
    registrar.set("defaultSslIdentityFile", [ext] { return ext->defaultSslIdentityFile(); });
    registrar.set("defaultLocalRepoPath", [ext] { return ext->defaultLocalRepoPath(); });
    registrar.set("defaultDisableAutosync", [ext] { return ext->defaultDisableAutosync(); });
}

} // namespace Fossil::Internal

