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

#include "fossilsettings.h"
#include "constants.h"

#include <QSettings>

namespace Fossil {
namespace Internal {

const QString FossilSettings::defaultRepoPathKey("defaultRepoPath");
const QString FossilSettings::sslIdentityFileKey("sslIdentityFile");
const QString FossilSettings::diffIgnoreAllWhiteSpaceKey("diffIgnoreAllWhiteSpace");
const QString FossilSettings::diffStripTrailingCRKey("diffStripTrailingCR");
const QString FossilSettings::annotateShowCommittersKey("annotateShowCommitters");
const QString FossilSettings::annotateListVersionsKey("annotateListVersions");
const QString FossilSettings::timelineWidthKey("timelineWidth");
const QString FossilSettings::timelineLineageFilterKey("timelineLineageFilter");
const QString FossilSettings::timelineVerboseKey("timelineVerbose");
const QString FossilSettings::timelineItemTypeKey("timelineItemType");
const QString FossilSettings::disableAutosyncKey("disableAutosync");

FossilSettings::FossilSettings()
{
    setSettingsGroup(Constants::FOSSIL);
    // Override default binary path
    declareKey(binaryPathKey, Constants::FOSSILDEFAULT);
    declareKey(defaultRepoPathKey, "");
    declareKey(sslIdentityFileKey, "");
    declareKey(diffIgnoreAllWhiteSpaceKey, false);
    declareKey(diffStripTrailingCRKey, false);
    declareKey(annotateShowCommittersKey, false);
    declareKey(annotateListVersionsKey, false);
    declareKey(timelineWidthKey, 0);
    declareKey(timelineLineageFilterKey, "");
    declareKey(timelineVerboseKey, false);
    declareKey(timelineItemTypeKey, "all");
    declareKey(disableAutosyncKey, true);
}

RepositorySettings::RepositorySettings()
    : autosync(AutosyncOn)
{
}

} // namespace Internal
} // namespace Fossil
