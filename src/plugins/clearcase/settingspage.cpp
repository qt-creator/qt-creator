/**************************************************************************
**
** Copyright (C) 2015 AudioCodes Ltd.
** Author: Orgad Shaneh <orgad.shaneh@audiocodes.com>
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "settingspage.h"
#include "clearcaseconstants.h"
#include "clearcasesettings.h"
#include "clearcaseplugin.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/pathchooser.h>

#include <QCoreApplication>
#include <QFileDialog>
#include <QTextStream>

using namespace ClearCase::Internal;
using namespace Utils;

SettingsPageWidget::SettingsPageWidget(QWidget *parent) :
    QWidget(parent)
{
    m_ui.setupUi(this);
    m_ui.commandPathChooser->setPromptDialogTitle(tr("ClearCase Command"));
    m_ui.commandPathChooser->setExpectedKind(PathChooser::ExistingCommand);
    m_ui.commandPathChooser->setHistoryCompleter(QLatin1String("ClearCase.Command.History"));
}

ClearCaseSettings SettingsPageWidget::settings() const
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
    return rc;
}

void SettingsPageWidget::setSettings(const ClearCaseSettings &s)
{
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
            diffWarning.append(tr("DiffUtils is available for free download at"
                                  "http://gnuwin32.sourceforge.net/packages/diffutils.htm."
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

SettingsPage::SettingsPage() :
    m_widget(0)
{
    setId(ClearCase::Constants::VCS_ID_CLEARCASE);
    setDisplayName(tr("ClearCase"));
}

QWidget *SettingsPage::widget()
{
    if (!m_widget)
        m_widget = new SettingsPageWidget;
    m_widget->setSettings(ClearCasePlugin::instance()->settings());
    return m_widget;
}

void SettingsPage::apply()
{
    ClearCasePlugin::instance()->setSettings(m_widget->settings());
}
