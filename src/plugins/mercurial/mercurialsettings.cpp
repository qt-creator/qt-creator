/****************************************************************************
**
** Copyright (C) 2016 Brian McGillion
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

#include "mercurialsettings.h"

#include "constants.h"

#include <utils/layoutbuilder.h>

#include <vcsbase/vcsbaseconstants.h>

using namespace Utils;

namespace Mercurial {
namespace Internal {

MercurialSettings::MercurialSettings()
{
    setSettingsGroup("Mercurial");
    setAutoApply(false);

    registerAspect(&binaryPath);
    binaryPath.setDisplayStyle(StringAspect::PathChooserDisplay);
    binaryPath.setExpectedKind(PathChooser::ExistingCommand);
    binaryPath.setDefaultValue(Constants::MERCURIALDEFAULT);
    binaryPath.setDisplayName(tr("Mercurial Command"));
    binaryPath.setHistoryCompleter("Bazaar.Command.History");
    binaryPath.setLabelText(tr("Command:"));

    registerAspect(&userName);
    userName.setDisplayStyle(StringAspect::LineEditDisplay);
    userName.setLabelText(tr("Default username:"));
    userName.setToolTip(tr("Username to use by default on commit."));

    registerAspect(&userEmail);
    userEmail.setDisplayStyle(StringAspect::LineEditDisplay);
    userEmail.setLabelText(tr("Default email:"));
    userEmail.setToolTip(tr("Email to use by default on commit."));

    registerAspect(&diffIgnoreWhiteSpace);
    diffIgnoreWhiteSpace.setSettingsKey("diffIgnoreWhiteSpace");

    registerAspect(&diffIgnoreBlankLines);
    diffIgnoreBlankLines.setSettingsKey("diffIgnoreBlankLines");
}

// Optionpage

class OptionsPageWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Mercurial::Internal::OptionsPageWidget)

public:
    OptionsPageWidget(const std::function<void()> &onApply, MercurialSettings *settings);
    void apply() final;

private:
    std::function<void()> m_onApply;
    MercurialSettings *m_settings;
};

OptionsPageWidget::OptionsPageWidget(const std::function<void()> &onApply, MercurialSettings *settings)
    : m_onApply(onApply), m_settings(settings)
{
    MercurialSettings &s = *settings;

    using namespace Layouting;
    const Break nl;

    Column {
        Group {
            Title(tr("Configuration")),
            Row { s.binaryPath }
        },

        Group {
            Title(tr("User")),
            Form {
                s.userName, nl,
                s.userEmail
            }
        },

        Group {
            Title(tr("Miscellaneous")),
            Row {
                s.logCount,
                s.timeout,
                Stretch()
            }
        },
        Stretch()

    }.attachTo(this);
}

void OptionsPageWidget::apply()
{
    if (!m_settings->isDirty())
        return;
    m_settings->apply();
    m_onApply();
}

OptionsPage::OptionsPage(const std::function<void()> &onApply, MercurialSettings *settings)
{
    setId(VcsBase::Constants::VCS_ID_MERCURIAL);
    setDisplayName(OptionsPageWidget::tr("Mercurial"));
    setCategory(VcsBase::Constants::VCS_SETTINGS_CATEGORY);
    setWidgetCreator([onApply, settings] { return new OptionsPageWidget(onApply, settings); });
}

} // namespace Internal
} // namespace Mercurial
