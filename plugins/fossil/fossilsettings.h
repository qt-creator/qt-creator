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

#pragma once

#include <vcsbase/vcsbaseclientsettings.h>

namespace Fossil {
namespace Internal {

class FossilSettings : public VcsBase::VcsBaseClientSettings
{
public:
    static const QString defaultRepoPathKey;
    static const QString sslIdentityFileKey;
    static const QString diffIgnoreAllWhiteSpaceKey;
    static const QString diffStripTrailingCRKey;
    static const QString annotateShowCommittersKey;
    static const QString annotateListVersionsKey;
    static const QString timelineWidthKey;
    static const QString timelineLineageFilterKey;
    static const QString timelineVerboseKey;
    static const QString timelineItemTypeKey;
    static const QString disableAutosyncKey;

    FossilSettings();
};

struct RepositorySettings
{
    enum AutosyncMode {AutosyncOff = 0, AutosyncOn = 1, AutosyncPullOnly};

    QString user;
    AutosyncMode autosync;
    QString sslIdentityFile;

    RepositorySettings();
};

inline bool operator== (const RepositorySettings &lh, const RepositorySettings &rh)
{
    return (lh.user == rh.user
            && lh.autosync == rh.autosync
            && lh.sslIdentityFile == rh.sslIdentityFile);
}

} // namespace Internal
} // namespace Fossil
