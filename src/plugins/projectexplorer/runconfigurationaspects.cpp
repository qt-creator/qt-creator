/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include "runconfigurationaspects.h"

#include "project.h"
#include "runconfiguration.h"
#include "environmentaspect.h"

#include <coreplugin/coreconstants.h>

#include <utils/fancylineedit.h>
#include <utils/pathchooser.h>

#include <QCheckBox>
#include <QLineEdit>
#include <QDebug>
#include <QFormLayout>
#include <QLabel>
#include <QToolButton>

using namespace Utils;

namespace ProjectExplorer {

/*!
    \class ProjectExplorer::TerminalAspect
*/

TerminalAspect::TerminalAspect(RunConfiguration *runConfig, const QString &key, bool useTerminal, bool userSet)
    : IRunConfigurationAspect(runConfig), m_useTerminal(useTerminal),
      m_userSet(userSet), m_checkBox(0), m_key(key)
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

WorkingDirectoryAspect::WorkingDirectoryAspect(RunConfiguration *runConfig, const QString &key, const QString &dir)
    : IRunConfigurationAspect(runConfig), m_workingDirectory(dir), m_chooser(0), m_key(key)
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
    return new WorkingDirectoryAspect(runConfig, m_key, m_workingDirectory);
}

void WorkingDirectoryAspect::addToMainConfigurationWidget(QWidget *parent, QFormLayout *layout)
{
    QTC_CHECK(!m_chooser);
    m_chooser = new PathChooser(parent);
    m_chooser->setHistoryCompleter(m_key);
    m_chooser->setExpectedKind(Utils::PathChooser::Directory);
    m_chooser->setPromptDialogTitle(tr("Select Working Directory"));
    connect(m_chooser.data(), &PathChooser::pathChanged,
            this, &WorkingDirectoryAspect::setWorkingDirectory);

    auto resetButton = new QToolButton(parent);
    resetButton->setToolTip(tr("Reset to Default"));
    resetButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_RESET)));
    connect(resetButton, &QAbstractButton::clicked, this, &WorkingDirectoryAspect::resetPath);

    if (auto envAspect = runConfiguration()->extraAspect<EnvironmentAspect>()) {
        connect(envAspect, &EnvironmentAspect::environmentChanged,
                this, &WorkingDirectoryAspect::updateEnvironment);
        updateEnvironment();
    }

    auto hbox = new QHBoxLayout;
    hbox->addWidget(m_chooser);
    hbox->addWidget(resetButton);
    layout->addRow(tr("Working directory:"), hbox);
}

void WorkingDirectoryAspect::updateEnvironment()
{
    auto envAspect = runConfiguration()->extraAspect<EnvironmentAspect>();
    QTC_ASSERT(envAspect, return);
    QTC_ASSERT(m_chooser, return);
    m_chooser->setEnvironment(envAspect->environment());
}

void WorkingDirectoryAspect::resetPath()
{
    QTC_ASSERT(m_chooser, return);
    m_chooser->setPath(QString());
}

void WorkingDirectoryAspect::fromMap(const QVariantMap &map)
{
    m_workingDirectory = map.value(m_key).toString();
}

void WorkingDirectoryAspect::toMap(QVariantMap &data) const
{
    data.insert(m_key, m_workingDirectory);
}

QString WorkingDirectoryAspect::workingDirectory() const
{
    return runConfiguration()->macroExpander()->expandProcessArgs(m_workingDirectory);
}

QString WorkingDirectoryAspect::unexpandedWorkingDirectory() const
{
    return m_workingDirectory;
}

void WorkingDirectoryAspect::setWorkingDirectory(const QString &workingDirectory)
{
    m_workingDirectory = workingDirectory;
}

PathChooser *WorkingDirectoryAspect::pathChooser() const
{
    return m_chooser;
}


/*!
    \class ProjectExplorer::ArgumentsAspect
*/

ArgumentsAspect::ArgumentsAspect(RunConfiguration *runConfig, const QString &key, const QString &arguments)
    : IRunConfigurationAspect(runConfig), m_arguments(arguments), m_chooser(0), m_key(key)
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
    if (m_chooser->text() != arguments)
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
