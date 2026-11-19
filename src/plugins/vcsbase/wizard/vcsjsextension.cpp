// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "vcsjsextension.h"

#include <coreplugin/iversioncontrol.h>
#include <coreplugin/jsbind.h>
#include <coreplugin/vcsmanager.h>

using namespace Core;
using namespace Utils;

namespace VcsBase::Internal {

void registerVcsFunctions(JsBind::Registrar &registrar)
{
    registrar.set("isConfigured", [](QString vcsId) {
        IVersionControl *vc = VcsManager::versionControl(Id::fromString(vcsId));
        return vc && vc->isConfigured();
    });
    registrar.set("displayName", [](QString vcsId) -> QString {
        IVersionControl *vc = VcsManager::versionControl(Id::fromString(vcsId));
        return vc ? vc->displayName() : QString();
    });
    registrar.set("isValidRepoUrl", [](QString vcsId, QString location) {
        const IVersionControl *vc = VcsManager::versionControl(Id::fromString(vcsId));
        return vc && vc->getRepoUrl(location).isValid;
    });
}

} // namespace VcsBase::Internal
