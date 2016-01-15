/****************************************************************************
**
** Copyright (C) 2016 Hugues Delorme
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

#include "optionspage.h"
#include "bazaarclient.h"
#include "bazaarsettings.h"
#include "bazaarplugin.h"

#include <coreplugin/icore.h>
#include <vcsbase/vcsbaseconstants.h>

#include <QTextStream>

using namespace Bazaar::Internal;
using namespace Bazaar;
using namespace VcsBase;

OptionsPageWidget::OptionsPageWidget(QWidget *parent) : VcsClientOptionsPageWidget(parent)
{
    m_ui.setupUi(this);
    m_ui.commandChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_ui.commandChooser->setPromptDialogTitle(tr("Bazaar Command"));
    m_ui.commandChooser->setHistoryCompleter(QLatin1String("Bazaar.Command.History"));
}

VcsBaseClientSettings OptionsPageWidget::settings() const
{
    VcsBaseClientSettings s = BazaarPlugin::instance()->client()->settings();
    s.setValue(BazaarSettings::binaryPathKey, m_ui.commandChooser->rawPath());
    s.setValue(BazaarSettings::userNameKey, m_ui.defaultUsernameLineEdit->text().trimmed());
    s.setValue(BazaarSettings::userEmailKey, m_ui.defaultEmailLineEdit->text().trimmed());
    s.setValue(BazaarSettings::logCountKey, m_ui.logEntriesCount->value());
    s.setValue(BazaarSettings::timeoutKey, m_ui.timeout->value());
    return s;
}

void OptionsPageWidget::setSettings(const VcsBaseClientSettings &s)
{
    m_ui.commandChooser->setPath(s.stringValue(BazaarSettings::binaryPathKey));
    m_ui.defaultUsernameLineEdit->setText(s.stringValue(BazaarSettings::userNameKey));
    m_ui.defaultEmailLineEdit->setText(s.stringValue(BazaarSettings::userEmailKey));
    m_ui.logEntriesCount->setValue(s.intValue(BazaarSettings::logCountKey));
    m_ui.timeout->setValue(s.intValue(BazaarSettings::timeoutKey));
}

OptionsPage::OptionsPage(Core::IVersionControl *control) :
    VcsClientOptionsPage(control, BazaarPlugin::instance()->client())
{
    setId(VcsBase::Constants::VCS_ID_BAZAAR);
    setDisplayName(tr("Bazaar"));
    setWidgetFactory([]() { return new OptionsPageWidget; });
}
