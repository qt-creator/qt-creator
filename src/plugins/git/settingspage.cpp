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
#include "gitsettings.h"
#include "gitplugin.h"
#include "gitclient.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>
#include <vcsbase/vcsbaseconstants.h>

#include <utils/environment.h>
#include <utils/hostosinfo.h>

#include <QDir>
#include <QDebug>
#include <QTextStream>

using namespace VcsBase;

namespace Git {
namespace Internal {

SettingsPageWidget::SettingsPageWidget(QWidget *parent) : VcsClientOptionsPageWidget(parent)
{
    m_ui.setupUi(this);
    if (Utils::HostOsInfo::isWindowsHost()) {
        const QByteArray currentHome = qgetenv("HOME");
        const QString toolTip
                = tr("Set the environment variable HOME to \"%1\"\n(%2).\n"
                     "This causes Git to look for the SSH-keys in that location\n"
                     "instead of its installation directory when run outside git bash.").
                arg(QDir::homePath(),
                    currentHome.isEmpty() ? tr("not currently set") :
                            tr("currently set to \"%1\"").arg(QString::fromLocal8Bit(currentHome)));
        m_ui.winHomeCheckBox->setToolTip(toolTip);
    } else {
        m_ui.winHomeCheckBox->setVisible(false);
    }
    updateNoteField();

    m_ui.repBrowserCommandPathChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_ui.repBrowserCommandPathChooser->setHistoryCompleter("Git.RepoCommand.History");
    m_ui.repBrowserCommandPathChooser->setPromptDialogTitle(tr("Git Repository Browser Command"));

    connect(m_ui.pathLineEdit, &QLineEdit::textChanged, this, &SettingsPageWidget::updateNoteField);
}

VcsBaseClientSettings SettingsPageWidget::settings() const
{
    GitSettings rc;
    rc.setValue(GitSettings::pathKey, m_ui.pathLineEdit->text());
    rc.setValue(GitSettings::logCountKey, m_ui.logCountSpinBox->value());
    rc.setValue(GitSettings::timeoutKey, m_ui.timeoutSpinBox->value());
    rc.setValue(GitSettings::pullRebaseKey, m_ui.pullRebaseCheckBox->isChecked());
    rc.setValue(GitSettings::winSetHomeEnvironmentKey, m_ui.winHomeCheckBox->isChecked());
    rc.setValue(GitSettings::gitkOptionsKey, m_ui.gitkOptionsLineEdit->text().trimmed());
    rc.setValue(GitSettings::repositoryBrowserCmd, m_ui.repBrowserCommandPathChooser->path().trimmed());

    return rc;
}

void SettingsPageWidget::setSettings(const VcsBaseClientSettings &s)
{
    m_ui.pathLineEdit->setText(s.stringValue(GitSettings::pathKey));
    m_ui.logCountSpinBox->setValue(s.intValue(GitSettings::logCountKey));
    m_ui.timeoutSpinBox->setValue(s.intValue(GitSettings::timeoutKey));
    m_ui.pullRebaseCheckBox->setChecked(s.boolValue(GitSettings::pullRebaseKey));
    m_ui.winHomeCheckBox->setChecked(s.boolValue(GitSettings::winSetHomeEnvironmentKey));
    m_ui.gitkOptionsLineEdit->setText(s.stringValue(GitSettings::gitkOptionsKey));
    m_ui.repBrowserCommandPathChooser->setPath(s.stringValue(GitSettings::repositoryBrowserCmd));
}

void SettingsPageWidget::updateNoteField()
{
    Utils::Environment env = Utils::Environment::systemEnvironment();
    env.prependOrSetPath(m_ui.pathLineEdit->text());

    bool showNote = env.searchInPath("perl").isEmpty();

    m_ui.noteFieldlabel->setVisible(showNote);
    m_ui.noteLabel->setVisible(showNote);
}

// -------- SettingsPage
SettingsPage::SettingsPage(Core::IVersionControl *control) :
    VcsClientOptionsPage(control, GitPlugin::client())
{
    setId(VcsBase::Constants::VCS_ID_GIT);
    setDisplayName(tr("Git"));
    setWidgetFactory([]() { return new SettingsPageWidget; });
}

void SettingsPage::apply()
{
    VcsClientOptionsPage::apply();

    if (widget()->isVisible()) {
        const VcsBaseClientSettings settings = widget()->settings();
        const GitSettings *rc = static_cast<const GitSettings *>(&settings);
        bool gitFoundOk;
        QString errorMessage;
        rc->gitExecutable(&gitFoundOk, &errorMessage);
        if (!gitFoundOk)
            Core::AsynchronousMessageBox::warning(tr("Git Settings"), errorMessage);
    }
}

} // namespace Internal
} // namespace Git
