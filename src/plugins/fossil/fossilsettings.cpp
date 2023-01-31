// Copyright (c) 2018 Artur Shepilko
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fossilsettings.h"

#include "constants.h"

#include <coreplugin/icore.h>

#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <vcsbase/vcsbaseconstants.h>

using namespace Utils;

namespace Fossil {
namespace Internal {

FossilSettings::FossilSettings()
{
    setSettingsGroup(Constants::FOSSIL);
    setAutoApply(false);

    registerAspect(&binaryPath);
    binaryPath.setDisplayStyle(StringAspect::PathChooserDisplay);
    binaryPath.setExpectedKind(PathChooser::ExistingCommand);
    binaryPath.setDefaultValue(Constants::FOSSILDEFAULT);
    binaryPath.setDisplayName(tr("Fossil Command"));
    binaryPath.setHistoryCompleter("Fossil.Command.History");
    binaryPath.setLabelText(tr("Command:"));

    registerAspect(&defaultRepoPath);
    defaultRepoPath.setSettingsKey("defaultRepoPath");
    defaultRepoPath.setDisplayStyle(StringAspect::PathChooserDisplay);
    defaultRepoPath.setExpectedKind(PathChooser::Directory);
    defaultRepoPath.setDisplayName(tr("Fossil Repositories"));
    defaultRepoPath.setLabelText(tr("Default path:"));
    defaultRepoPath.setToolTip(tr("Directory to store local repositories by default."));

    registerAspect(&userName);
    userName.setDisplayStyle(StringAspect::LineEditDisplay);
    userName.setLabelText(tr("Default user:"));
    userName.setToolTip(tr("Existing user to become an author of changes made to the repository."));

    registerAspect(&sslIdentityFile);
    sslIdentityFile.setSettingsKey("sslIdentityFile");
    sslIdentityFile.setDisplayStyle(StringAspect::PathChooserDisplay);
    sslIdentityFile.setExpectedKind(PathChooser::File);
    sslIdentityFile.setDisplayName(tr("SSL/TLS Identity Key"));
    sslIdentityFile.setLabelText(tr("SSL/TLS identity:"));
    sslIdentityFile.setToolTip(tr("SSL/TLS client identity key to use if requested by the server."));

    registerAspect(&diffIgnoreAllWhiteSpace);
    diffIgnoreAllWhiteSpace.setSettingsKey("diffIgnoreAllWhiteSpace");

    registerAspect(&diffStripTrailingCR);
    diffStripTrailingCR.setSettingsKey("diffStripTrailingCR");

    registerAspect(&annotateShowCommitters);
    annotateShowCommitters.setSettingsKey("annotateShowCommitters");

    registerAspect(&annotateListVersions);
    annotateListVersions.setSettingsKey("annotateListVersions");

    registerAspect(&timelineWidth);
    timelineWidth.setSettingsKey("timelineWidth");
    timelineWidth.setLabelText(tr("Log width:"));
    timelineWidth.setToolTip(tr("The width of log entry line (>20). "
        "Choose 0 to see a single line per entry."));

    registerAspect(&timelineLineageFilter);
    timelineLineageFilter.setSettingsKey("timelineLineageFilter");

    registerAspect(&timelineVerbose);
    timelineVerbose.setSettingsKey("timelineVerbose");

    registerAspect(&timelineItemType);
    timelineItemType.setDefaultValue("all");
    timelineItemType.setSettingsKey("timelineItemType");

    registerAspect(&disableAutosync);
    disableAutosync.setSettingsKey("disableAutosync");
    disableAutosync.setDefaultValue(true);
    disableAutosync.setLabelText(tr("Disable auto-sync"));
    disableAutosync.setToolTip(tr("Disable automatic pull prior to commit or update and "
        "automatic push after commit or tag or branch creation."));

    registerAspect(&timeout);
    timeout.setLabelText(tr("Timeout:"));
    timeout.setSuffix(tr("s"));

    registerAspect(&logCount);
    logCount.setLabelText(tr("Log count:"));
    logCount.setToolTip(tr("The number of recent commit log entries to show. "
        "Choose 0 to see all entries."));
};

// OptionsPage

class OptionsPageWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Fossil::Internal::OptionsPageWidget)

public:
    OptionsPageWidget(const std::function<void()> &onApply, FossilSettings *settings);
    void apply() final;

private:
    const std::function<void()> m_onApply;
    FossilSettings *m_settings;
};

void OptionsPageWidget::apply()
{
    if (!m_settings->isDirty())
        return;

    m_settings->apply();
    m_onApply();
}

OptionsPageWidget::OptionsPageWidget(const std::function<void()> &onApply, FossilSettings *settings) :
    m_onApply(onApply),
    m_settings(settings)
{
    FossilSettings &s = *m_settings;

    using namespace Layouting;

    Column {
        Group {
            title(tr("Configuration")),
            Row { s.binaryPath }
        },

        Group {
            title(tr("Local Repositories")),
            Row { s.defaultRepoPath }
        },
        Group {
            title(tr("User")),
            Form {
                s.userName, br,
                s.sslIdentityFile
            }
        },

        Group {
            title(tr("Miscellaneous")),
            Column {
                Row {
                s.logCount,
                s.timelineWidth,
                s.timeout,
                st
                },
                s.disableAutosync
            },
        },
        st

    }.attachTo(this);
}

OptionsPage::OptionsPage(const std::function<void()> &onApply, FossilSettings *settings)
{
    setId(Constants::VCS_ID_FOSSIL);
    setDisplayName(OptionsPageWidget::tr("Fossil"));
    setWidgetCreator([onApply, settings]() { return new OptionsPageWidget(onApply, settings); });
    setCategory(VcsBase::Constants::VCS_SETTINGS_CATEGORY);
}

} // Internal
} // Fossil
