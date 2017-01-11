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

#include "runconfigurationaspects.h"

#include "project.h"
#include "runconfiguration.h"
#include "environmentaspect.h"

#include <utils/utilsicons.h>
#include <utils/fancylineedit.h>
#include <utils/pathchooser.h>

#include <QCheckBox>
#include <QLineEdit>
#include <QFormLayout>
#include <QToolButton>

using namespace Utils;

namespace ProjectExplorer {

/*!
    \class ProjectExplorer::TerminalAspect
*/

TerminalAspect::TerminalAspect(RunConfiguration *runConfig, const QString &key,
                               bool useTerminal, bool userSet) :
    IRunConfigurationAspect(runConfig),
    m_useTerminal(useTerminal), m_userSet(userSet), m_checkBox(nullptr), m_key(key)
{
    setDisplayName(tr("Terminal"));
    setId("TerminalAspect");
}

TerminalAspect *TerminalAspect::create(RunConfiguration *runConfig) const
{
    return new TerminalAspect(runConfig, m_key, false, false);
}

TerminalAspect *TerminalAspect::clone(RunConfiguration *runConfig) const
{
    return new TerminalAspect(runConfig, m_key, m_useTerminal, m_userSet);
}

void TerminalAspect::addToMainConfigurationWidget(QWidget *parent, QFormLayout *layout)
{
    QTC_CHECK(!m_checkBox);
    m_checkBox = new QCheckBox(tr("Run in terminal"), parent);
    m_checkBox->setChecked(m_useTerminal);
    layout->addRow(QString(), m_checkBox);
    connect(m_checkBox.data(), &QAbstractButton::clicked, this, [this] {
        m_userSet = true;
        m_useTerminal = m_checkBox->isChecked();
        emit useTerminalChanged(m_useTerminal);
    });
}

void TerminalAspect::fromMap(const QVariantMap &map)
{
    if (map.contains(m_key)) {
        m_useTerminal = map.value(m_key).toBool();
        m_userSet = true;
    } else {
        m_userSet = false;
    }
}

void TerminalAspect::toMap(QVariantMap &data) const
{
    if (m_userSet)
        data.insert(m_key, m_useTerminal);
}

bool TerminalAspect::useTerminal() const
{
    return m_useTerminal;
}

void TerminalAspect::setUseTerminal(bool useTerminal)
{
    if (m_useTerminal != useTerminal) {
        m_useTerminal = useTerminal;
        emit useTerminalChanged(useTerminal);
    }
    if (m_checkBox)
        m_checkBox->setChecked(m_useTerminal);
}

bool TerminalAspect::isUserSet() const
{
    return m_userSet;
}

ApplicationLauncher::Mode TerminalAspect::runMode() const
{
    return m_useTerminal ? ApplicationLauncher::Console : ApplicationLauncher::Gui;
}

void TerminalAspect::setRunMode(ApplicationLauncher::Mode runMode)
{
    setUseTerminal(runMode == ApplicationLauncher::Console);
}

/*!
    \class ProjectExplorer::WorkingDirectoryAspect
*/

WorkingDirectoryAspect::WorkingDirectoryAspect(RunConfiguration *runConfig, const QString &key)
    : IRunConfigurationAspect(runConfig), m_key(key)
{
    setDisplayName(tr("Working Directory"));
    setId("WorkingDirectoryAspect");
}

WorkingDirectoryAspect *WorkingDirectoryAspect::create(RunConfiguration *runConfig) const
{
    return new WorkingDirectoryAspect(runConfig, m_key);
}

WorkingDirectoryAspect *WorkingDirectoryAspect::clone(RunConfiguration *runConfig) const
{
    auto * const aspect = new WorkingDirectoryAspect(runConfig, m_key);
    aspect->m_defaultWorkingDirectory = m_defaultWorkingDirectory;
    aspect->m_workingDirectory = m_workingDirectory;
    return aspect;
}

void WorkingDirectoryAspect::addToMainConfigurationWidget(QWidget *parent, QFormLayout *layout)
{
    QTC_CHECK(!m_chooser);
    m_resetButton = new QToolButton(parent);
    m_resetButton->setToolTip(tr("Reset to Default"));
    m_resetButton->setIcon(Utils::Icons::RESET.icon());
    connect(m_resetButton.data(), &QAbstractButton::clicked, this, &WorkingDirectoryAspect::resetPath);

    m_chooser = new PathChooser(parent);
    m_chooser->setHistoryCompleter(m_key);
    m_chooser->setExpectedKind(Utils::PathChooser::Directory);
    m_chooser->setPromptDialogTitle(tr("Select Working Directory"));
    m_chooser->setBaseFileName(m_defaultWorkingDirectory);
    m_chooser->setFileName(m_workingDirectory.isEmpty() ? m_defaultWorkingDirectory : m_workingDirectory);
    connect(m_chooser.data(), &PathChooser::pathChanged, this,
            [this]() {
                m_workingDirectory = m_chooser->rawFileName();
                m_resetButton->setEnabled(m_workingDirectory != m_defaultWorkingDirectory);
            });

    m_resetButton->setEnabled(m_workingDirectory != m_defaultWorkingDirectory);

    if (auto envAspect = runConfiguration()->extraAspect<EnvironmentAspect>()) {
        connect(envAspect, &EnvironmentAspect::environmentChanged, m_chooser.data(), [this, envAspect] {
            m_chooser->setEnvironment(envAspect->environment());
        });
        m_chooser->setEnvironment(envAspect->environment());
    }

    auto hbox = new QHBoxLayout;
    hbox->addWidget(m_chooser);
    hbox->addWidget(m_resetButton);
    layout->addRow(tr("Working directory:"), hbox);
}

QString WorkingDirectoryAspect::keyForDefaultWd() const
{
    return m_key + QLatin1String(".default");
}

void WorkingDirectoryAspect::resetPath()
{
    m_chooser->setFileName(m_defaultWorkingDirectory);
}

void WorkingDirectoryAspect::fromMap(const QVariantMap &map)
{
    m_workingDirectory = FileName::fromString(map.value(m_key).toString());
    m_defaultWorkingDirectory = FileName::fromString(map.value(keyForDefaultWd()).toString());

    if (m_workingDirectory.isEmpty())
        m_workingDirectory = m_defaultWorkingDirectory;
}

void WorkingDirectoryAspect::toMap(QVariantMap &data) const
{
    const QString wd
            = (m_workingDirectory == m_defaultWorkingDirectory) ? QString() : m_workingDirectory.toString();
    data.insert(m_key, wd);
    data.insert(keyForDefaultWd(), m_defaultWorkingDirectory.toString());
}

FileName WorkingDirectoryAspect::workingDirectory() const
{
    auto envAspect = runConfiguration()->extraAspect<EnvironmentAspect>();
    const Utils::Environment env = envAspect ? envAspect->environment()
                                             : Utils::Environment::systemEnvironment();
    const QString macroExpanded
            = runConfiguration()->macroExpander()->expandProcessArgs(m_workingDirectory.toUserOutput());
    return FileName::fromString(PathChooser::expandedDirectory(macroExpanded, env, QString()));
}

FileName WorkingDirectoryAspect::defaultWorkingDirectory() const
{
    return m_defaultWorkingDirectory;
}

FileName WorkingDirectoryAspect::unexpandedWorkingDirectory() const
{
    return m_workingDirectory;
}

void WorkingDirectoryAspect::setDefaultWorkingDirectory(const FileName &defaultWorkingDir)
{
    if (defaultWorkingDir == m_defaultWorkingDirectory)
        return;

    Utils::FileName oldDefaultDir = m_defaultWorkingDirectory;
    m_defaultWorkingDirectory = defaultWorkingDir;
    if (m_chooser)
        m_chooser->setBaseFileName(m_defaultWorkingDirectory);

    if (m_workingDirectory.isEmpty() || m_workingDirectory == oldDefaultDir) {
        if (m_chooser)
            m_chooser->setFileName(m_defaultWorkingDirectory);
        m_workingDirectory = defaultWorkingDir;
    }
}

PathChooser *WorkingDirectoryAspect::pathChooser() const
{
    return m_chooser;
}


/*!
    \class ProjectExplorer::ArgumentsAspect
*/

ArgumentsAspect::ArgumentsAspect(RunConfiguration *runConfig, const QString &key, const QString &arguments)
    : IRunConfigurationAspect(runConfig), m_arguments(arguments), m_key(key)
{
    setDisplayName(tr("Arguments"));
    setId("ArgumentsAspect");
}

QString ArgumentsAspect::arguments() const
{
    return runConfiguration()->macroExpander()->expandProcessArgs(m_arguments);
}

QString ArgumentsAspect::unexpandedArguments() const
{
    return m_arguments;
}

void ArgumentsAspect::setArguments(const QString &arguments)
{
    if (arguments != m_arguments) {
        m_arguments = arguments;
        emit argumentsChanged(arguments);
    }
    if (m_chooser && m_chooser->text() != arguments)
        m_chooser->setText(arguments);
}

void ArgumentsAspect::fromMap(const QVariantMap &map)
{
    m_arguments = map.value(m_key).toString();
}

void ArgumentsAspect::toMap(QVariantMap &map) const
{
    map.insert(m_key, m_arguments);
}

ArgumentsAspect *ArgumentsAspect::create(RunConfiguration *runConfig) const
{
    return new ArgumentsAspect(runConfig, m_key);
}

ArgumentsAspect *ArgumentsAspect::clone(RunConfiguration *runConfig) const
{
    return new ArgumentsAspect(runConfig, m_key, m_arguments);
}

void ArgumentsAspect::addToMainConfigurationWidget(QWidget *parent, QFormLayout *layout)
{
    QTC_CHECK(!m_chooser);
    m_chooser = new FancyLineEdit(parent);
    m_chooser->setHistoryCompleter(m_key);
    m_chooser->setText(m_arguments);

    connect(m_chooser.data(), &QLineEdit::textChanged, this, &ArgumentsAspect::setArguments);

    layout->addRow(tr("Command line arguments:"), m_chooser);
}

} // namespace ProjectExplorer
