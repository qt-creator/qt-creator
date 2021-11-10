/****************************************************************************
**
** Copyright (C) 2016 Brian McGillion and Hugues Delorme
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

#include "vcsbaseclientsettings.h"

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcsettings.h>
#include <utils/stringutils.h>

#include <QSettings>
#include <QVariant>

using namespace Utils;

namespace VcsBase {

VcsBaseSettings::VcsBaseSettings()
{
    setAutoApply(false);

    registerAspect(&binaryPath);
    binaryPath.setSettingsKey("BinaryPath");

    registerAspect(&userName);
    userName.setSettingsKey("Username");

    registerAspect(&userEmail);
    userEmail.setSettingsKey("UserEmail");

    registerAspect(&logCount);
    logCount.setSettingsKey("LogCount");
    logCount.setRange(0, 1000 * 1000);
    logCount.setDefaultValue(100);
    logCount.setLabelText(tr("Log count:"));

    registerAspect(&path);
    path.setSettingsKey("Path");

    registerAspect(&promptOnSubmit);
    promptOnSubmit.setSettingsKey("PromptOnSubmit");
    promptOnSubmit.setDefaultValue(true);
    promptOnSubmit.setLabelText(tr("Prompt on submit"));

    registerAspect(&timeout);
    timeout.setSettingsKey("Timeout");
    timeout.setRange(0, 3600 * 24 * 365);
    timeout.setDefaultValue(30);
    timeout.setLabelText(tr("Timeout:"));
    timeout.setSuffix(tr("s"));
}

FilePaths VcsBaseSettings::searchPathList() const
{
    return Utils::transform(path.value().split(HostOsInfo::pathListSeparator(), Qt::SkipEmptyParts),
                            &FilePath::fromUserInput);
}

} // namespace VcsBase
