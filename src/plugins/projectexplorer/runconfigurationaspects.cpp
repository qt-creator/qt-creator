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

TerminalAspect::TerminalAspect(RunConfiguration *runConfig, const QString &key, bool useTerminal) :
    IRunConfigurationAspect(runConfig), m_useTerminal(useTerminal)
{
    setDisplayName(tr("Terminal"));
    setId("TerminalAspect");
    setSettingsKey(key);
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
    : IRunConfigurationAspect(runConfig)
{
    setDisplayName(tr("Working Directory"));
    setId("WorkingDirectoryAspect");
    setSettingsKey(key);
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

ArgumentsAspect::ArgumentsAspect(RunConfiguration *runConfig, const QString &key)
    : IRunConfigurationAspect(runConfig)
{
    setDisplayName(tr("Arguments"));
    setId("ArgumentsAspect");
    setSettingsKey(key);
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
    \class ProjectExplorer::BaseStringAspect
*/

BaseStringAspect::BaseStringAspect(RunConfiguration *rc)
    : IRunConfigurationAspect(rc)
{
}

BaseStringAspect::~BaseStringAspect()
{
    delete m_checker;
    m_checker = nullptr;
}

QString BaseStringAspect::value() const
{
    return m_value;
}

void BaseStringAspect::setValue(const QString &value)
{
    const bool isSame = value == m_value;
    m_value = value;
    update();
    if (!isSame)
        emit changed();
}

void BaseStringAspect::fromMap(const QVariantMap &map)
{
    if (!settingsKey().isEmpty())
        m_value = map.value(settingsKey()).toString();
    if (m_checker)
        m_checker->fromMap(map);
}

void BaseStringAspect::toMap(QVariantMap &map) const
{
    if (!settingsKey().isEmpty())
        map.insert(settingsKey(), m_value);
    if (m_checker)
        m_checker->toMap(map);
}

FileName BaseStringAspect::fileName() const
{
    return FileName::fromString(m_value);
}

void BaseStringAspect::setLabelText(const QString &labelText)
{
    m_labelText = labelText;
    if (m_label)
        m_label->setText(labelText);
}

QString BaseStringAspect::labelText() const
{
    return m_labelText;
}

void BaseStringAspect::setDisplayFilter(const std::function<QString(const QString &)> &displayFilter)
{
    m_displayFilter = displayFilter;
}

bool BaseStringAspect::isChecked() const
{
    return !m_checker || m_checker->value();
}

void BaseStringAspect::setDisplayStyle(DisplayStyle displayStyle)
{
    m_displayStyle = displayStyle;
}

void BaseStringAspect::setPlaceHolderText(const QString &placeHolderText)
{
    m_placeHolderText = placeHolderText;
    if (m_lineEditDisplay)
        m_lineEditDisplay->setPlaceholderText(placeHolderText);
}

void BaseStringAspect::setHistoryCompleter(const QString &historyCompleterKey)
{
    m_historyCompleterKey = historyCompleterKey;
    if (m_lineEditDisplay)
        m_lineEditDisplay->setHistoryCompleter(historyCompleterKey);
    if (m_pathChooserDisplay)
        m_pathChooserDisplay->setHistoryCompleter(historyCompleterKey);
}

void BaseStringAspect::setExpectedKind(const PathChooser::Kind expectedKind)
{
    m_expectedKind = expectedKind;
    if (m_pathChooserDisplay)
        m_pathChooserDisplay->setExpectedKind(expectedKind);
}

void BaseStringAspect::setEnvironment(const Environment &env)
{
    m_environment = env;
    if (m_pathChooserDisplay)
        m_pathChooserDisplay->setEnvironment(env);
}

void BaseStringAspect::addToConfigurationLayout(QFormLayout *layout)
{
    QTC_CHECK(!m_label);
    QWidget *parent = layout->parentWidget();
    m_label = new QLabel(parent);
    m_label->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto hbox = new QHBoxLayout;
    switch (m_displayStyle) {
    case PathChooserDisplay:
        m_pathChooserDisplay = new PathChooser(parent);
        m_pathChooserDisplay->setExpectedKind(m_expectedKind);
        m_pathChooserDisplay->setHistoryCompleter(m_historyCompleterKey);
        m_pathChooserDisplay->setEnvironment(m_environment);
        connect(m_pathChooserDisplay, &PathChooser::pathChanged,
                this, &BaseStringAspect::setValue);
        hbox->addWidget(m_pathChooserDisplay);
        break;
    case LineEditDisplay:
        m_lineEditDisplay = new FancyLineEdit(parent);
        m_lineEditDisplay->setPlaceholderText(m_placeHolderText);
        m_lineEditDisplay->setHistoryCompleter(m_historyCompleterKey);
        connect(m_lineEditDisplay, &FancyLineEdit::textEdited,
                this, &BaseStringAspect::setValue);
        hbox->addWidget(m_lineEditDisplay);
        break;
    case LabelDisplay:
        m_labelDisplay = new QLabel(parent);
        m_labelDisplay->setTextInteractionFlags(Qt::TextSelectableByMouse);
        hbox->addWidget(m_labelDisplay);
        break;
    }

    if (m_checker) {
        auto form = new QFormLayout;
        form->setContentsMargins(0, 0, 0, 0);
        form->setFormAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_checker->addToConfigurationLayout(form);
        hbox->addLayout(form);
    }
    layout->addRow(m_label, hbox);

    update();
}

void BaseStringAspect::update()
{
    const QString displayedString = m_displayFilter ? m_displayFilter(m_value) : m_value;
    const bool enabled = !m_checker || m_checker->value();

    if (m_pathChooserDisplay) {
        m_pathChooserDisplay->setFileName(FileName::fromString(displayedString));
        m_pathChooserDisplay->setEnabled(enabled);
    }

    if (m_lineEditDisplay) {
        m_lineEditDisplay->setText(displayedString);
        m_lineEditDisplay->setEnabled(enabled);
    }

    if (m_labelDisplay)
        m_labelDisplay->setText(displayedString);

    if (m_label)
        m_label->setText(m_labelText);
}

void BaseStringAspect::makeCheckable(const QString &checkerLabel, const QString &checkerKey)
{
    QTC_ASSERT(!m_checker, return);
    m_checker = new BaseBoolAspect(runConfiguration());
    m_checker->setLabel(checkerLabel);
    m_checker->setSettingsKey(checkerKey);

    connect(m_checker, &BaseBoolAspect::changed, this, &BaseStringAspect::update);
    connect(m_checker, &BaseBoolAspect::changed, this, &BaseStringAspect::changed);

    update();
}

/*!
    \class ProjectExplorer::ExecutableAspect
*/

ExecutableAspect::ExecutableAspect(RunConfiguration *rc)
    : IRunConfigurationAspect(rc), m_executable(rc)
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
    m_alternativeExecutable = new BaseStringAspect(runConfiguration());
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
    IRunConfigurationAspect::setSettingsKey(key);
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
    \class ProjectExplorer::BaseBoolAspect
*/

BaseBoolAspect::BaseBoolAspect(RunConfiguration *runConfig, const QString &settingsKey)
    : IRunConfigurationAspect(runConfig)
{
    setSettingsKey(settingsKey);
}

BaseBoolAspect::~BaseBoolAspect() = default;

void BaseBoolAspect::addToConfigurationLayout(QFormLayout *layout)
{
    QTC_CHECK(!m_checkBox);
    m_checkBox = new QCheckBox(m_label, layout->parentWidget());
    m_checkBox->setChecked(m_value);
    layout->addRow(QString(), m_checkBox);
    connect(m_checkBox.data(), &QAbstractButton::clicked, this, [this] {
        m_value = m_checkBox->isChecked();
        emit changed();
    });
}

void BaseBoolAspect::fromMap(const QVariantMap &map)
{
    m_value = map.value(settingsKey(), false).toBool();
}

void BaseBoolAspect::toMap(QVariantMap &data) const
{
    data.insert(settingsKey(), m_value);
}

bool BaseBoolAspect::value() const
{
    return m_value;
}

void BaseBoolAspect::setValue(bool value)
{
    m_value = value;
    if (m_checkBox)
        m_checkBox->setChecked(m_value);
}

void BaseBoolAspect::setLabel(const QString &label)
{
    m_label = label;
}

/*!
    \class ProjectExplorer::UseLibraryPathsAspect
*/

UseLibraryPathsAspect::UseLibraryPathsAspect(RunConfiguration *rc, const QString &settingsKey)
    : BaseBoolAspect(rc, settingsKey)
{
    setId("UseLibraryPath");
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

UseDyldSuffixAspect::UseDyldSuffixAspect(RunConfiguration *rc, const QString &settingsKey)
    : BaseBoolAspect(rc, settingsKey)
{
    setId("UseDyldSuffix");
    setLabel(tr("Use debug version of frameworks (DYLD_IMAGE_SUFFIX=_debug)"));
}

} // namespace ProjectExplorer
