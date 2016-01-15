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

#include "settingspage.h"

#include "cvsclient.h"
#include "cvssettings.h"
#include "cvsplugin.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <vcsbase/vcsbaseconstants.h>
#include <utils/pathchooser.h>

#include <QCoreApplication>
#include <QTextStream>
#include <QFileDialog>

using namespace Cvs::Internal;
using namespace Utils;
using namespace VcsBase;

SettingsPageWidget::SettingsPageWidget(QWidget *parent) : VcsClientOptionsPageWidget(parent)
{
    m_ui.setupUi(this);
    m_ui.commandPathChooser->setExpectedKind(PathChooser::ExistingCommand);
    m_ui.commandPathChooser->setHistoryCompleter(QLatin1String("Cvs.Command.History"));
    m_ui.commandPathChooser->setPromptDialogTitle(tr("CVS Command"));
}

VcsBaseClientSettings SettingsPageWidget::settings() const
{
    CvsSettings rc;
    rc.setValue(CvsSettings::binaryPathKey, m_ui.commandPathChooser->rawPath());
    rc.setValue(CvsSettings::cvsRootKey, m_ui.rootLineEdit->text());
    rc.setValue(CvsSettings::diffOptionsKey, m_ui.diffOptionsLineEdit->text());
    rc.setValue(CvsSettings::timeoutKey, m_ui.timeOutSpinBox->value());
    rc.setValue(CvsSettings::promptOnSubmitKey, m_ui.promptToSubmitCheckBox->isChecked());
    rc.setValue(CvsSettings::describeByCommitIdKey, m_ui.describeByCommitIdCheckBox->isChecked());
    return rc;
}

void SettingsPageWidget::setSettings(const VcsBaseClientSettings &s)
{
    m_ui.commandPathChooser->setFileName(s.binaryPath());
    m_ui.rootLineEdit->setText(s.stringValue(CvsSettings::cvsRootKey));
    m_ui.diffOptionsLineEdit->setText(s.stringValue(CvsSettings::diffOptionsKey));
    m_ui.timeOutSpinBox->setValue(s.intValue(CvsSettings::timeoutKey));
    m_ui.promptToSubmitCheckBox->setChecked(s.boolValue(CvsSettings::promptOnSubmitKey));
    m_ui.describeByCommitIdCheckBox->setChecked(s.boolValue(CvsSettings::describeByCommitIdKey));
}

SettingsPage::SettingsPage(Core::IVersionControl *control) :
    VcsClientOptionsPage(control, CvsPlugin::instance()->client())
{
    setId(VcsBase::Constants::VCS_ID_CVS);
    setDisplayName(tr("CVS"));
    setWidgetFactory([]() { return new SettingsPageWidget; });
}
