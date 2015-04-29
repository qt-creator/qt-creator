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

TerminalAspect::TerminalAspect(RunConfiguration *runConfig, const QString &key, bool useTerminal, bool isForced)
    : IRunConfigurationAspect(runConfig), m_useTerminal(useTerminal),
      m_isForced(isForced), m_checkBox(0), m_key(key)
{
    setDisplayName(tr("Terminal"));
    setId("TerminalAspect");
}

IRunConfigurationAspect *TerminalAspect::create(RunConfiguration *runConfig) const
{
    return new TerminalAspect(runConfig, m_key, false, false);
}

IRunConfigurationAspect *TerminalAspect::clone(RunConfiguration *runConfig) const
{
    return new TerminalAspect(runConfig, m_key, m_useTerminal, m_isForced);
}

void TerminalAspect::addToMainConfigurationWidget(QWidget *parent, QFormLayout *layout)
{
    QTC_CHECK(!m_checkBox);
    m_checkBox = new QCheckBox(tr("Run in terminal"), parent);
    m_checkBox->setChecked(m_useTerminal);
    layout->addRow(QString(), m_checkBox);
    connect(m_checkBox.data(), &QAbstractButton::clicked, this, [this] {
        m_isForced = true;
        setUseTerminal(true);
    });
}

void TerminalAspect::fromMap(const QVariantMap &map)
{
    if (map.contains(m_key)) {
        m_useTerminal = map.value(m_key).toBool();
        m_isForced = true;
    } else {
        m_isForced = false;
    }
}

void TerminalAspect::toMap(QVariantMap &data) const
{
    if (m_isForced)
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

IRunConfigurationAspect *WorkingDirectoryAspect::create(RunConfiguration *runConfig) const
{
    return new WorkingDirectoryAspect(runConfig, m_key);
}

IRunConfigurationAspect *WorkingDirectoryAspect::clone(RunConfiguration *runConfig) const
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
    connect(m_chooser, &PathChooser::pathChanged, this, &WorkingDirectoryAspect::setWorkingDirectory);

    auto resetButton = new QToolButton(parent);
    resetButton->setToolTip(tr("Reset to default"));
    resetButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_RESET)));
    connect(resetButton, &QAbstractButton::clicked, this, [this] { m_chooser->setPath(QString()); });

    if (auto envAspect = runConfiguration()->extraAspect<EnvironmentAspect>()) {
        connect(envAspect, &EnvironmentAspect::environmentChanged, this, [this, envAspect] {
            m_chooser->setEnvironment(envAspect->environment());
        });
        m_chooser->setEnvironment(envAspect->environment());
    }

    auto hbox = new QHBoxLayout;
    hbox->addWidget(m_chooser);
    hbox->addWidget(resetButton);
    layout->addRow(tr("Working directory:"), hbox);
}

void WorkingDirectoryAspect::fromMap(const QVariantMap &map)
{
    m_workingDirectory = map.value(m_key).toBool();
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
    m_arguments = arguments;
    if (m_chooser)
        m_chooser->setText(m_arguments);
}

void ArgumentsAspect::fromMap(const QVariantMap &map)
{
    m_arguments = map.value(m_key).toBool();
}

void ArgumentsAspect::toMap(QVariantMap &map) const
{
    map.insert(m_key, m_arguments);
}

IRunConfigurationAspect *ArgumentsAspect::create(RunConfiguration *runConfig) const
{
    return new ArgumentsAspect(runConfig, m_key);
}

IRunConfigurationAspect *ArgumentsAspect::clone(RunConfiguration *runConfig) const
{
    return new ArgumentsAspect(runConfig, m_key, m_arguments);
}

void ArgumentsAspect::addToMainConfigurationWidget(QWidget *parent, QFormLayout *layout)
{
    QTC_CHECK(!m_chooser);
    m_chooser = new FancyLineEdit(parent);
    m_chooser->setHistoryCompleter(m_key);

    connect(m_chooser, &QLineEdit::textChanged, this, &ArgumentsAspect::setArguments);

    layout->addRow(tr("Command line arguments:"), m_chooser);
}


/*!
    \class ProjectExplorer::ExecutableAspect
*/

ExecutableAspect::ExecutableAspect(RunConfiguration *runConfig, const QString &key, const QString &executable)
    : IRunConfigurationAspect(runConfig), m_executable(executable), m_chooser(0), m_key(key)
{
    setDisplayName(tr("Executable"));
    setId("ExecutableAspect");
}

QString ExecutableAspect::executable() const
{
    return runConfiguration()->macroExpander()->expandProcessArgs(m_executable);
}

QString ExecutableAspect::unexpandedExecutable() const
{
    return m_executable;
}

void ExecutableAspect::setExectuable(const QString &executable)
{
    m_executable = executable;
    if (m_chooser)
        m_chooser->setText(m_executable);
}

void ExecutableAspect::fromMap(const QVariantMap &map)
{
    m_executable = map.value(m_key).toBool();
}

void ExecutableAspect::toMap(QVariantMap &map) const
{
    map.insert(m_key, m_executable);
}

IRunConfigurationAspect *ExecutableAspect::create(RunConfiguration *runConfig) const
{
    return new ExecutableAspect(runConfig, m_key);
}

IRunConfigurationAspect *ExecutableAspect::clone(RunConfiguration *runConfig) const
{
    return new ExecutableAspect(runConfig, m_key, m_executable);
}

void ExecutableAspect::addToMainConfigurationWidget(QWidget *parent, QFormLayout *layout)
{
    QTC_CHECK(!m_chooser);
    m_chooser = new FancyLineEdit(parent);
    m_chooser->setHistoryCompleter(m_key);

    connect(m_chooser, &QLineEdit::textChanged, this, &ExecutableAspect::setExectuable);

    layout->addRow(tr("Command line arguments:"), m_chooser);
}

} // namespace ProjectExplorer
