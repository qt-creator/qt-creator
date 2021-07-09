/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "fileshareprotocolsettingspage.h"
#include "cpasterconstants.h"

#include <utils/layoutbuilder.h>
#include <utils/temporarydirectory.h>

using namespace Utils;

namespace CodePaster {

FileShareProtocolSettings::FileShareProtocolSettings()
{
    setSettingsGroup("FileSharePasterSettings");
    setAutoApply(false);

    registerAspect(&path);
    path.setSettingsKey("Path");
    path.setDisplayStyle(StringAspect::PathChooserDisplay);
    path.setExpectedKind(PathChooser::ExistingDirectory);
    path.setDefaultValue(TemporaryDirectory::masterDirectoryPath());
    path.setLabelText(tr("&Path:"));

    registerAspect(&displayCount);
    displayCount.setSettingsKey("DisplayCount");
    displayCount.setDefaultValue(10);
    displayCount.setSuffix(' ' + tr("entries"));
    displayCount.setLabelText(tr("&Display:"));
}

// Settings page

FileShareProtocolSettingsPage::FileShareProtocolSettingsPage(FileShareProtocolSettings *settings)
{
    setId("X.CodePaster.FileSharePaster");
    setDisplayName(FileShareProtocolSettings::tr("Fileshare"));
    setCategory(Constants::CPASTER_SETTINGS_CATEGORY);
    setSettings(settings);

    setLayouter([&s = *settings](QWidget *widget) {
        using namespace Layouting;

        auto label = new QLabel(FileShareProtocolSettings::tr(
            "The fileshare-based paster protocol allows for sharing code snippets using "
            "simple files on a shared network drive. Files are never deleted."));
        label->setWordWrap(true);

        Column {
            Form {
                label, Break(),
                s.path,
                s.displayCount
            },
            Stretch()
        }.attachTo(widget);
    });
}

} // namespace CodePaster
