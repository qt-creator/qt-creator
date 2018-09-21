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

#include "environmentaspect.h"
#include "project.h"
#include "projectexplorer.h"
#include "projectexplorersettings.h"
#include "runconfiguration.h"
#include "target.h"

#include <utils/utilsicons.h>
#include <utils/fancylineedit.h>
#include <utils/pathchooser.h>
#include <utils/qtcprocess.h>

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QFormLayout>
#include <QToolButton>

using namespace Utils;

namespace ProjectExplorer {

/*!
    \class ProjectExplorer::TerminalAspect
*/

TerminalAspect::TerminalAspect()
{
    setDisplayName(tr("Terminal"));
    setId("TerminalAspect");
    setSettingsKey("RunConfiguration.UseTerminal");
}

void TerminalAspect::addToConfigurationLayout(QFormLayout *layout)
{
    QTC_CHECK(!m_checkBox);
    m_checkBox = new QCheckBox(tr("Run in terminal"), layout->parentWidget());
    m_checkBox->setChecked(m_useTerminal);
    layout->addRow(QString(), m_checkBox);
    connect(m_checkBox.data(), &QAbstractButton::clicked, this, [this] {
        m_userSet = true;
        m_useTerminal = m_checkBox->isChecked();
        emit changed();
    });
}

void TerminalAspect::fromMap(const QVariantMap &map)
{
    if (map.contains(settingsKey())) {
        m_useTerminal = map.value(settingsKey()).toBool();
        m_userSet = true;
    } else {
        m_userSet = false;
    }

    if (m_checkBox)
        m_checkBox->setChecked(m_useTerminal);
}

void TerminalAspect::toMap(QVariantMap &data) const
{
    if (m_userSet)
        data.insert(settingsKey(), m_useTerminal);
}

bool TerminalAspect::useTerminal() const
{
    return m_useTerminal;
}

void TerminalAspect::setUseTerminal(bool useTerminal)
{
    if (m_useTerminal != useTerminal) {
        m_useTerminal = useTerminal;
        emit changed();
    }
    if (m_checkBox)
        m_checkBox->setChecked(m_useTerminal);
}

bool TerminalAspect::isUserSet() const
{
    return m_userSet;
}

/*!
    \class ProjectExplorer::WorkingDirectoryAspect
*/

WorkingDirectoryAspect::WorkingDirectoryAspect(EnvironmentAspect *envAspect)
    : m_envAspect(envAspect)
{
    setDisplayName(tr("Working Directory"));
    setId("WorkingDirectoryAspect");
    setSettingsKey("RunConfiguration.WorkingDirectory");
}

void WorkingDirectoryAspect::addToConfigurationLayout(QFormLayout *layout)
{
    QTC_CHECK(!m_chooser);
    m_resetButton = new QToolButton(layout->parentWidget());
    m_resetButton->setToolTip(tr("Reset to Default"));
    m_resetButton->setIcon(Utils::Icons::RESET.icon());
    connect(m_resetButton.data(), &QAbstractButton::clicked, this, &WorkingDirectoryAspect::resetPath);

    m_chooser = new PathChooser(layout->parentWidget());
    m_chooser->setHistoryCompleter(settingsKey());
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

    if (m_envAspect) {
        connect(m_envAspect, &EnvironmentAspect::environmentChanged, m_chooser.data(), [this] {
            m_chooser->setEnvironment(m_envAspect->environment());
        });
        m_chooser->setEnvironment(m_envAspect->environment());
    }

    auto hbox = new QHBoxLayout;
    hbox->addWidget(m_chooser);
    hbox->addWidget(m_resetButton);
    layout->addRow(tr("Working directory:"), hbox);
}

QString WorkingDirectoryAspect::keyForDefaultWd() const
{
    return settingsKey() + ".default";
}

void WorkingDirectoryAspect::resetPath()
{
    m_chooser->setFileName(m_defaultWorkingDirectory);
}

void WorkingDirectoryAspect::fromMap(const QVariantMap &map)
{
    m_workingDirectory = FileName::fromString(map.value(settingsKey()).toString());
    m_defaultWorkingDirectory = FileName::fromString(map.value(keyForDefaultWd()).toString());

    if (m_workingDirectory.isEmpty())
        m_workingDirectory = m_defaultWorkingDirectory;

    if (m_chooser)
        m_chooser->setFileName(m_workingDirectory.isEmpty() ? m_defaultWorkingDirectory : m_workingDirectory);
}

void WorkingDirectoryAspect::toMap(QVariantMap &data) const
{
    const QString wd = m_workingDirectory == m_defaultWorkingDirectory
        ? QString() : m_workingDirectory.toString();
    data.insert(settingsKey(), wd);
    data.insert(keyForDefaultWd(), m_defaultWorkingDirectory.toString());
}

FileName WorkingDirectoryAspect::workingDirectory(const MacroExpander *expander) const
{
    const Utils::Environment env = m_envAspect ? m_envAspect->environment()
                                               : Utils::Environment::systemEnvironment();
    QString workingDir = m_workingDirectory.toUserOutput();
    if (expander)
        workingDir = expander->expandProcessArgs(workingDir);
    return FileName::fromString(PathChooser::expandedDirectory(workingDir, env, QString()));
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

ArgumentsAspect::ArgumentsAspect()
{
    setDisplayName(tr("Arguments"));
    setId("ArgumentsAspect");
    setSettingsKey("RunConfiguration.Arguments");
}

QString ArgumentsAspect::arguments(const MacroExpander *expander) const
{
    QTC_ASSERT(expander, return m_arguments);
    return expander->expandProcessArgs(m_arguments);
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
    QVariant args = map.value(settingsKey());
    // Until 3.7 a QStringList was stored for Remote Linux
    if (args.type() == QVariant::StringList)
        m_arguments = QtcProcess::joinArgs(args.toStringList(), OsTypeLinux);
    else
        m_arguments = args.toString();

    if (m_chooser)
        m_chooser->setText(m_arguments);
}

void ArgumentsAspect::toMap(QVariantMap &map) const
{
    map.insert(settingsKey(), m_arguments);
}

void ArgumentsAspect::addToConfigurationLayout(QFormLayout *layout)
{
    QTC_CHECK(!m_chooser);
    m_chooser = new FancyLineEdit(layout->parentWidget());
    m_chooser->setHistoryCompleter(settingsKey());
    m_chooser->setText(m_arguments);

    connect(m_chooser.data(), &QLineEdit::textChanged, this, &ArgumentsAspect::setArguments);

    layout->addRow(tr("Command line arguments:"), m_chooser);
}

/*!
    \class ProjectExplorer::ExecutableAspect
*/

ExecutableAspect::ExecutableAspect()
{
    setDisplayName(tr("Executable"));
    setId("ExecutableAspect");
    setExecutablePathStyle(HostOsInfo::hostOs());
    m_executable.setPlaceHolderText(tr("<unknown>"));
    m_executable.setLabelText(tr("Executable:"));
    m_executable.setDisplayStyle(BaseStringAspect::LabelDisplay);

    connect(&m_executable, &BaseStringAspect::changed,
            this, &ExecutableAspect::changed);
}

ExecutableAspect::~ExecutableAspect()
{
    delete m_alternativeExecutable;
    m_alternativeExecutable = nullptr;
}

void ExecutableAspect::setExecutablePathStyle(OsType osType)
{
    m_executable.setDisplayFilter([osType](const QString &pathName) {
        return OsSpecificAspects::pathWithNativeSeparators(osType, pathName);
    });
}

void ExecutableAspect::setHistoryCompleter(const QString &historyCompleterKey)
{
    m_executable.setHistoryCompleter(historyCompleterKey);
    if (m_alternativeExecutable)
        m_alternativeExecutable->setHistoryCompleter(historyCompleterKey);
}

void ExecutableAspect::setExpectedKind(const PathChooser::Kind expectedKind)
{
    m_executable.setExpectedKind(expectedKind);
    if (m_alternativeExecutable)
        m_alternativeExecutable->setExpectedKind(expectedKind);
}

void ExecutableAspect::setEnvironment(const Environment &env)
{
    m_executable.setEnvironment(env);
    if (m_alternativeExecutable)
        m_alternativeExecutable->setEnvironment(env);
}

void ExecutableAspect::setDisplayStyle(BaseStringAspect::DisplayStyle style)
{
    m_executable.setDisplayStyle(style);
}

void ExecutableAspect::makeOverridable(const QString &overridingKey, const QString &useOverridableKey)
{
    QTC_ASSERT(!m_alternativeExecutable, return);
    m_alternativeExecutable = new BaseStringAspect;
    m_alternativeExecutable->setDisplayStyle(BaseStringAspect::LineEditDisplay);
    m_alternativeExecutable->setLabelText(tr("Alternate executable on device:"));
    m_alternativeExecutable->setSettingsKey(overridingKey);
    m_alternativeExecutable->makeCheckable(tr("Use this command instead"), useOverridableKey);
    connect(m_alternativeExecutable, &BaseStringAspect::changed,
            this, &ExecutableAspect::changed);
}

FileName ExecutableAspect::executable() const
{
    if (m_alternativeExecutable && m_alternativeExecutable->isChecked())
        return m_alternativeExecutable->fileName();

    return m_executable.fileName();
}

void ExecutableAspect::addToConfigurationLayout(QFormLayout *layout)
{
    m_executable.addToConfigurationLayout(layout);
    if (m_alternativeExecutable)
        m_alternativeExecutable->addToConfigurationLayout(layout);
}

void ExecutableAspect::setLabelText(const QString &labelText)
{
    m_executable.setLabelText(labelText);
}

void ExecutableAspect::setPlaceHolderText(const QString &placeHolderText)
{
    m_executable.setPlaceHolderText(placeHolderText);
}

void ExecutableAspect::setExecutable(const FileName &executable)
{
   m_executable.setValue(executable.toString());
}

void ExecutableAspect::setSettingsKey(const QString &key)
{
    ProjectConfigurationAspect::setSettingsKey(key);
    m_executable.setSettingsKey(key);
}

void ExecutableAspect::fromMap(const QVariantMap &map)
{
    m_executable.fromMap(map);
    if (m_alternativeExecutable)
        m_alternativeExecutable->fromMap(map);
}

void ExecutableAspect::toMap(QVariantMap &map) const
{
    m_executable.toMap(map);
    if (m_alternativeExecutable)
        m_alternativeExecutable->toMap(map);
}


/*!
    \class ProjectExplorer::UseLibraryPathsAspect
*/

UseLibraryPathsAspect::UseLibraryPathsAspect()
{
    setId("UseLibraryPath");
    setSettingsKey("RunConfiguration.UseLibrarySearchPath");
    if (HostOsInfo::isMacHost())
        setLabel(tr("Add build library search path to DYLD_LIBRARY_PATH and DYLD_FRAMEWORK_PATH"));
    else if (HostOsInfo::isWindowsHost())
        setLabel(tr("Add build library search path to PATH"));
    else
        setLabel(tr("Add build library search path to LD_LIBRARY_PATH"));
    setValue(ProjectExplorerPlugin::projectExplorerSettings().addLibraryPathsToRunEnv);
}

/*!
    \class ProjectExplorer::UseDyldSuffixAspect
*/

UseDyldSuffixAspect::UseDyldSuffixAspect()
{
    setId("UseDyldSuffix");
    setSettingsKey("RunConfiguration.UseDyldImageSuffix");
    setLabel(tr("Use debug version of frameworks (DYLD_IMAGE_SUFFIX=_debug)"));
}

} // namespace ProjectExplorer
