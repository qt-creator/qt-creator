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

#include <coreplugin/icore.h>

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

class FileShareProtocolSettingsWidget final : public Core::IOptionsPageWidget
{
public:
    FileShareProtocolSettingsWidget(FileShareProtocolSettings *settings)
        : m_settings(settings)
    {
        FileShareProtocolSettings &s = *settings;
        using namespace Layouting;

        auto label = new QLabel(tr("The fileshare-based paster protocol allows for sharing code"
                                   "snippets using simple files on a shared network drive. "
                                   "Files are never deleted."));
        label->setWordWrap(true);

        Column {
            Form {
            label, Break(),
                s.path,
                s.displayCount
            },
            Stretch()
        }.attachTo(this);
    }

    void apply() final
    {
        if (m_settings->isDirty()) {
            m_settings->apply();
            m_settings->writeSettings(Core::ICore::settings());
        }
    }

private:
    FileShareProtocolSettings *m_settings;
};

FileShareProtocolSettingsPage::FileShareProtocolSettingsPage(FileShareProtocolSettings *s)
{
    setId("X.CodePaster.FileSharePaster");
    setDisplayName(FileShareProtocolSettingsWidget::tr("Fileshare"));
    setCategory(Constants::CPASTER_SETTINGS_CATEGORY);
    setWidgetCreator([s] { return new FileShareProtocolSettingsWidget(s); });
}

} // namespace CodePaster
