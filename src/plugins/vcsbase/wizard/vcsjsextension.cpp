// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "vcsjsextension.h"

#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

using namespace Core;
using namespace Utils;

namespace VcsBase {
namespace Internal {

bool VcsJsExtension::isConfigured(const QString &vcsId) const
{
    IVersionControl *vc = VcsManager::versionControl(Id::fromString(vcsId));
    return vc && vc->isConfigured();
}

QString VcsJsExtension::displayName(const QString &vcsId) const
{
    IVersionControl *vc = VcsManager::versionControl(Id::fromString(vcsId));
    return vc ? vc->displayName() : QString();
}

bool VcsJsExtension::isValidRepoUrl(const QString &vcsId, const QString &location) const
{
    const IVersionControl * const vc = VcsManager::versionControl(Id::fromString(vcsId));
    return vc && vc->getRepoUrl(location).isValid;
}

} // namespace Internal
} // namespace VcsBase
