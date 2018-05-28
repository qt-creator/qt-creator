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

#include "customexecutablerunconfiguration.h"

#include "abi.h"
#include "buildconfiguration.h"
#include "devicesupport/devicemanager.h"
#include "environmentaspect.h"
#include "localenvironmentaspect.h"
#include "project.h"
#include "runconfigurationaspects.h"
#include "target.h"

#include <coreplugin/icore.h>
#include <coreplugin/variablechooser.h>

#include <utils/detailswidget.h>
#include <utils/pathchooser.h>
#include <utils/stringutils.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFormLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

using namespace Utils;

namespace ProjectExplorer {

const char CUSTOM_EXECUTABLE_ID[] = "ProjectExplorer.CustomExecutableRunConfiguration";

// Dialog prompting the user to complete the configuration.

class CustomExecutableDialog : public QDialog
{
public:
    explicit CustomExecutableDialog(RunConfiguration *rc);

    void accept() override;
    bool event(QEvent *event) override;

private:
    void changed() {
        bool isValid = !m_executableChooser->rawPath().isEmpty();
        m_dialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(isValid);
    }

private:
    void environmentWasChanged();

    QDialogButtonBox *m_dialogButtonBox = nullptr;
    RunConfiguration *m_rc = nullptr;
    ArgumentsAspect m_arguments;
    WorkingDirectoryAspect m_workingDirectory;
    TerminalAspect m_terminal;
    PathChooser *m_executableChooser = nullptr;
};

CustomExecutableDialog::CustomExecutableDialog(RunConfiguration *rc)
    : QDialog(Core::ICore::dialogParent()),
      m_rc(rc),
      m_arguments(rc, rc->extraAspect<ArgumentsAspect>()->settingsKey()),
      m_workingDirectory(rc, rc->extraAspect<WorkingDirectoryAspect>()->settingsKey()),
      m_terminal(rc, rc->extraAspect<TerminalAspect>()->settingsKey())
{
    auto vbox = new QVBoxLayout(this);
    vbox->addWidget(new QLabel(tr("Could not find the executable, please specify one.")));

    auto layout = new QFormLayout;
    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    layout->setMargin(0);

    auto detailsContainer = new DetailsWidget(this);
    detailsContainer->setState(DetailsWidget::NoSummary);
    vbox->addWidget(detailsContainer);

    m_dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_dialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    connect(m_dialogButtonBox, &QDialogButtonBox::accepted, this, &CustomExecutableDialog::accept);
    connect(m_dialogButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    vbox->addWidget(m_dialogButtonBox);
    vbox->setSizeConstraint(QLayout::SetMinAndMaxSize);

    auto detailsWidget = new QWidget(detailsContainer);
    detailsContainer->setWidget(detailsWidget);
    detailsWidget->setLayout(layout);

    m_executableChooser = new PathChooser(this);
    m_executableChooser->setHistoryCompleter("Qt.CustomExecutable.History");
    m_executableChooser->setExpectedKind(PathChooser::ExistingCommand);
    m_executableChooser->setPath(rc->extraAspect<ExecutableAspect>()->executable().toString());
    layout->addRow(tr("Executable:"), m_executableChooser);
    connect(m_executableChooser, &PathChooser::rawPathChanged,
            this, &CustomExecutableDialog::changed);

    m_arguments.copyFrom(rc->extraAspect<ArgumentsAspect>());
    m_arguments.addToConfigurationLayout(layout);

    m_workingDirectory.copyFrom(rc->extraAspect<WorkingDirectoryAspect>());
    m_workingDirectory.addToConfigurationLayout(layout);

    m_terminal.copyFrom(rc->extraAspect<TerminalAspect>());
    m_terminal.addToConfigurationLayout(layout);

    auto enviromentAspect = rc->extraAspect<EnvironmentAspect>();
    connect(enviromentAspect, &EnvironmentAspect::environmentChanged,
            this, &CustomExecutableDialog::environmentWasChanged);
    environmentWasChanged();

    Core::VariableChooser::addSupportForChildWidgets(this, m_rc->macroExpander());
}

void CustomExecutableDialog::accept()
{
    auto executable = FileName::fromString(m_executableChooser->path());
    m_rc->extraAspect<ExecutableAspect>()->setExecutable(executable);
    m_rc->extraAspect<WorkingDirectoryAspect>()->copyFrom(&m_workingDirectory);
    m_rc->extraAspect<ArgumentsAspect>()->copyFrom(&m_arguments);
    m_rc->extraAspect<TerminalAspect>()->copyFrom(&m_terminal);

    QDialog::accept();
}

bool CustomExecutableDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Escape && !ke->modifiers()) {
            ke->accept();
            return true;
        }
    }
    return QDialog::event(event);
}

void CustomExecutableDialog::environmentWasChanged()
{
    auto aspect = m_rc->extraAspect<EnvironmentAspect>();
    QTC_ASSERT(aspect, return);
    m_executableChooser->setEnvironment(aspect->environment());
}


// CustomExecutableRunConfiguration

CustomExecutableRunConfiguration::CustomExecutableRunConfiguration(Target *target)
    : CustomExecutableRunConfiguration(target, CUSTOM_EXECUTABLE_ID)
{}

CustomExecutableRunConfiguration::CustomExecutableRunConfiguration(Target *target, Core::Id id)
    : RunConfiguration(target, id)
{
    auto envAspect = new LocalEnvironmentAspect(this, LocalEnvironmentAspect::BaseEnvironmentModifier());
    addExtraAspect(envAspect);

    auto exeAspect = new ExecutableAspect(this);
    exeAspect->setSettingsKey("ProjectExplorer.CustomExecutableRunConfiguration.Executable");
    exeAspect->setDisplayStyle(BaseStringAspect::PathChooserDisplay);
    exeAspect->setHistoryCompleter("Qt.CustomExecutable.History");
    exeAspect->setExpectedKind(PathChooser::ExistingCommand);
    exeAspect->setEnvironment(envAspect->environment());
    addExtraAspect(exeAspect);

    addExtraAspect(new ArgumentsAspect(this, "ProjectExplorer.CustomExecutableRunConfiguration.Arguments"));
    addExtraAspect(new TerminalAspect(this, "ProjectExplorer.CustomExecutableRunConfiguration.UseTerminal"));
    addExtraAspect(new WorkingDirectoryAspect(this, "ProjectExplorer.CustomExecutableRunConfiguration.WorkingDirectory"));

    connect(envAspect, &EnvironmentAspect::environmentChanged,
            this, [exeAspect, envAspect] { exeAspect->setEnvironment(envAspect->environment()); });

    setDefaultDisplayName(defaultDisplayName());
}

// Note: Qt4Project deletes all empty customexecrunconfigs for which isConfigured() == false.
CustomExecutableRunConfiguration::~CustomExecutableRunConfiguration()
{
    if (m_dialog) {
        emit configurationFinished();
        disconnect(m_dialog, &QDialog::finished,
                   this, &CustomExecutableRunConfiguration::configurationDialogFinished);
    }
    delete m_dialog;
}

RunConfiguration::ConfigurationState CustomExecutableRunConfiguration::ensureConfigured(QString *errorMessage)
{
    if (m_dialog) {// uhm already shown
        errorMessage->clear(); // no error dialog
        m_dialog->activateWindow();
        m_dialog->raise();
        return UnConfigured;
    }

    m_dialog = new CustomExecutableDialog(this);
    connect(m_dialog, &QDialog::finished,
            this, &CustomExecutableRunConfiguration::configurationDialogFinished);
    m_dialog->setWindowTitle(displayName()); // pretty pointless
    m_dialog->show();
    return Waiting;
}

void CustomExecutableRunConfiguration::configurationDialogFinished()
{
    disconnect(m_dialog, &QDialog::finished,
               this, &CustomExecutableRunConfiguration::configurationDialogFinished);
    m_dialog->deleteLater();
    m_dialog = nullptr;
    emit configurationFinished();
}

QString CustomExecutableRunConfiguration::rawExecutable() const
{
    return extraAspect<ExecutableAspect>()->executable().toString();
}

bool CustomExecutableRunConfiguration::isConfigured() const
{
    return !rawExecutable().isEmpty();
}

Runnable CustomExecutableRunConfiguration::runnable() const
{
    FileName workingDirectory = extraAspect<WorkingDirectoryAspect>()->workingDirectory();

    Runnable r;
    r.executable = extraAspect<ExecutableAspect>()->executable().toString();
    r.commandLineArguments = extraAspect<ArgumentsAspect>()->arguments();
    r.environment = extraAspect<EnvironmentAspect>()->environment();
    r.runMode = extraAspect<TerminalAspect>()->runMode();
    r.workingDirectory = workingDirectory.toString();
    r.device = DeviceManager::instance()->defaultDevice(Constants::DESKTOP_DEVICE_TYPE);

    if (!r.executable.isEmpty()) {
        QString expanded = macroExpander()->expand(r.executable);
        r.executable = r.environment.searchInPath(expanded, {workingDirectory}).toString();
    }

    return r;
}

QString CustomExecutableRunConfiguration::defaultDisplayName() const
{
    if (rawExecutable().isEmpty())
        return tr("Custom Executable");
    else
        return tr("Run %1").arg(QDir::toNativeSeparators(rawExecutable()));
}

Abi CustomExecutableRunConfiguration::abi() const
{
    return Abi(); // return an invalid ABI: We do not know what we will end up running!
}

// Factory

CustomExecutableRunConfigurationFactory::CustomExecutableRunConfigurationFactory() :
    FixedRunConfigurationFactory(CustomExecutableRunConfiguration::tr("Custom Executable"))
{
    registerRunConfiguration<CustomExecutableRunConfiguration>(CUSTOM_EXECUTABLE_ID);
}

} // namespace ProjectExplorer
