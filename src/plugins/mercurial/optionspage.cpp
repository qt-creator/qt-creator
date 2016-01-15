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

#include "optionspage.h"

#include "mercurialclient.h"
#include "mercurialsettings.h"
#include "mercurialplugin.h"

#include <coreplugin/icore.h>
#include <utils/pathchooser.h>
#include <vcsbase/vcsbaseconstants.h>

#include <QTextStream>

using namespace VcsBase;

namespace Mercurial {
namespace Internal  {

OptionsPageWidget::OptionsPageWidget(QWidget *parent) : VcsClientOptionsPageWidget(parent)
{
    m_ui.setupUi(this);
    m_ui.commandChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_ui.commandChooser->setHistoryCompleter(QLatin1String("Mercurial.Command.History"));
    m_ui.commandChooser->setPromptDialogTitle(tr("Mercurial Command"));
}

VcsBaseClientSettings OptionsPageWidget::settings() const
{
    MercurialSettings s;
    s.setValue(MercurialSettings::binaryPathKey, m_ui.commandChooser->rawPath());
    s.setValue(MercurialSettings::userNameKey, m_ui.defaultUsernameLineEdit->text().trimmed());
    s.setValue(MercurialSettings::userEmailKey, m_ui.defaultEmailLineEdit->text().trimmed());
    s.setValue(MercurialSettings::logCountKey, m_ui.logEntriesCount->value());
    s.setValue(MercurialSettings::timeoutKey, m_ui.timeout->value());
    return s;
}

void OptionsPageWidget::setSettings(const VcsBaseClientSettings &s)
{
    m_ui.commandChooser->setPath(s.stringValue(MercurialSettings::binaryPathKey));
    m_ui.defaultUsernameLineEdit->setText(s.stringValue(MercurialSettings::userNameKey));
    m_ui.defaultEmailLineEdit->setText(s.stringValue(MercurialSettings::userEmailKey));
    m_ui.logEntriesCount->setValue(s.intValue(MercurialSettings::logCountKey));
    m_ui.timeout->setValue(s.intValue(MercurialSettings::timeoutKey));
}

OptionsPage::OptionsPage(Core::IVersionControl *control) :
    VcsClientOptionsPage(control, MercurialPlugin::client())
{
    setId(VcsBase::Constants::VCS_ID_MERCURIAL);
    setDisplayName(tr("Mercurial"));
    setWidgetFactory([]() { return new OptionsPageWidget; });
}

} // namespace Internal
} // namespace Mercurial
