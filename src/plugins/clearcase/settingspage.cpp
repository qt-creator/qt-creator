/****************************************************************************
**
** Copyright (C) 2016 AudioCodes Ltd.
** Author: Orgad Shaneh <orgad.shaneh@audiocodes.com>
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

#include "settingspage.h"
#include "clearcaseconstants.h"
#include "clearcasesettings.h"
#include "clearcaseplugin.h"
#include "ui_settingspage.h"

#include <coreplugin/icore.h>

#include <vcsbase/vcsbaseconstants.h>

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/pathchooser.h>

#include <QCoreApplication>

using namespace Utils;

namespace ClearCase {
namespace Internal {

class SettingsPageWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(ClearCase::Internal::SettingsPageWidget)

public:
    SettingsPageWidget();

private:
    void apply() final;

    Ui::SettingsPage m_ui;
};

SettingsPageWidget::SettingsPageWidget()
{
    m_ui.setupUi(this);
    m_ui.commandPathChooser->setPromptDialogTitle(tr("ClearCase Command"));
    m_ui.commandPathChooser->setExpectedKind(PathChooser::ExistingCommand);
    m_ui.commandPathChooser->setHistoryCompleter(QLatin1String("ClearCase.Command.History"));

    const ClearCaseSettings &s = ClearCasePlugin::settings();

    m_ui.commandPathChooser->setPath(s.ccCommand);
    m_ui.timeOutSpinBox->setValue(s.timeOutS);
    m_ui.autoCheckOutCheckBox->setChecked(s.autoCheckOut);
    m_ui.noCommentCheckBox->setChecked(s.noComment);
    bool extDiffAvailable = !Environment::systemEnvironment().searchInPath(QLatin1String("diff")).isEmpty();
    if (extDiffAvailable) {
        m_ui.diffWarningLabel->setVisible(false);
    } else {
        QString diffWarning = tr("In order to use External diff, \"diff\" command needs to be accessible.");
        if (HostOsInfo::isWindowsHost()) {
            diffWarning += QLatin1Char(' ');
            diffWarning.append(tr("DiffUtils is available for free download at "
                                  "http://gnuwin32.sourceforge.net/packages/diffutils.htm. "
                                  "Extract it to a directory in your PATH."));
        }
        m_ui.diffWarningLabel->setText(diffWarning);
        m_ui.externalDiffRadioButton->setEnabled(false);
    }
    if (extDiffAvailable && s.diffType == ExternalDiff)
        m_ui.externalDiffRadioButton->setChecked(true);
    else
        m_ui.graphicalDiffRadioButton->setChecked(true);
    m_ui.autoAssignActivityCheckBox->setChecked(s.autoAssignActivityName);
    m_ui.historyCountSpinBox->setValue(s.historyCount);
    m_ui.promptCheckBox->setChecked(s.promptToCheckIn);
    m_ui.disableIndexerCheckBox->setChecked(s.disableIndexer);
    m_ui.diffArgsEdit->setText(s.diffArgs);
    m_ui.indexOnlyVOBsEdit->setText(s.indexOnlyVOBs);
}

void SettingsPageWidget::apply()
{
    ClearCaseSettings rc;
    rc.ccCommand = m_ui.commandPathChooser->rawPath();
    rc.ccBinaryPath = m_ui.commandPathChooser->path();
    rc.timeOutS = m_ui.timeOutSpinBox->value();
    rc.autoCheckOut = m_ui.autoCheckOutCheckBox->isChecked();
    rc.noComment = m_ui.noCommentCheckBox->isChecked();
    if (m_ui.graphicalDiffRadioButton->isChecked())
        rc.diffType = GraphicalDiff;
    else if (m_ui.externalDiffRadioButton->isChecked())
        rc.diffType = ExternalDiff;
    rc.autoAssignActivityName = m_ui.autoAssignActivityCheckBox->isChecked();
    rc.historyCount = m_ui.historyCountSpinBox->value();
    rc.promptToCheckIn = m_ui.promptCheckBox->isChecked();
    rc.disableIndexer = m_ui.disableIndexerCheckBox->isChecked();
    rc.diffArgs = m_ui.diffArgsEdit->text();
    rc.indexOnlyVOBs = m_ui.indexOnlyVOBsEdit->text();
    rc.extDiffAvailable = m_ui.externalDiffRadioButton->isEnabled();

    ClearCasePlugin::setSettings(rc);
}

ClearCaseSettingsPage::ClearCaseSettingsPage()
{
    setId(ClearCase::Constants::VCS_ID_CLEARCASE);
    setDisplayName(SettingsPageWidget::tr("ClearCase"));
    setCategory(VcsBase::Constants::VCS_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new SettingsPageWidget; });
}

} // Internal
} // ClearCase
