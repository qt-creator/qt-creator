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

static void copyAspect(ProjectConfigurationAspect *source, ProjectConfigurationAspect *target)
{
    QVariantMap data;
    source->toMap(data);
    target->fromMap(data);
}

class CustomExecutableDialog : public QDialog
{
    Q_DECLARE_TR_FUNCTIONS(CustomExecutableDialog)
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
      m_workingDirectory(rc->aspect<EnvironmentAspect>())
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
    m_executableChooser->setPath(rc->aspect<ExecutableAspect>()->executable().toString());
    layout->addRow(tr("Executable:"), m_executableChooser);
    connect(m_executableChooser, &PathChooser::rawPathChanged,
            this, &CustomExecutableDialog::changed);

    copyAspect(rc->aspect<ArgumentsAspect>(), &m_arguments);
    m_arguments.addToConfigurationLayout(layout);

    copyAspect(rc->aspect<WorkingDirectoryAspect>(), &m_workingDirectory);
    m_workingDirectory.addToConfigurationLayout(layout);

    copyAspect(rc->aspect<TerminalAspect>(), &m_terminal);
    m_terminal.addToConfigurationLayout(layout);

    auto enviromentAspect = rc->aspect<EnvironmentAspect>();
    connect(enviromentAspect, &EnvironmentAspect::environmentChanged,
            this, &CustomExecutableDialog::environmentWasChanged);
    environmentWasChanged();

    Core::VariableChooser::addSupportForChildWidgets(this, m_rc->macroExpander());
}

void CustomExecutableDialog::accept()
{
    auto executable = FileName::fromString(m_executableChooser->path());
    m_rc->aspect<ExecutableAspect>()->setExecutable(executable);
    copyAspect(&m_arguments, m_rc->aspect<ArgumentsAspect>());
    copyAspect(&m_workingDirectory, m_rc->aspect<WorkingDirectoryAspect>());
    copyAspect(&m_terminal, m_rc->aspect<TerminalAspect>());

    QDialog::accept();
}

bool CustomExecutableDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride) {
        auto *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Escape && !ke->modifiers()) {
            ke->accept();
            return true;
        }
    }
    return QDialog::event(event);
}

void CustomExecutableDialog::environmentWasChanged()
{
    auto aspect = m_rc->aspect<EnvironmentAspect>();
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
    auto envAspect = addAspect<LocalEnvironmentAspect>
            (target, LocalEnvironmentAspect::BaseEnvironmentModifier());

    auto exeAspect = addAspect<ExecutableAspect>();
    exeAspect->setSettingsKey("ProjectExplorer.CustomExecutableRunConfiguration.Executable");
    exeAspect->setDisplayStyle(BaseStringAspect::PathChooserDisplay);
    exeAspect->setHistoryCompleter("Qt.CustomExecutable.History");
    exeAspect->setExpectedKind(PathChooser::ExistingCommand);
    exeAspect->setEnvironment(envAspect->environment());

    addAspect<ArgumentsAspect>();
    addAspect<WorkingDirectoryAspect>(envAspect);
    addAspect<TerminalAspect>();

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
    return aspect<ExecutableAspect>()->executable().toString();
}

bool CustomExecutableRunConfiguration::isConfigured() const
{
    return !rawExecutable().isEmpty();
}

Runnable CustomExecutableRunConfiguration::runnable() const
{
    FileName workingDirectory =
            aspect<WorkingDirectoryAspect>()->workingDirectory(macroExpander());

    Runnable r;
    r.executable = aspect<ExecutableAspect>()->executable().toString();
    r.commandLineArguments = aspect<ArgumentsAspect>()->arguments(macroExpander());
    r.environment = aspect<EnvironmentAspect>()->environment();
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

// Factory

CustomExecutableRunConfigurationFactory::CustomExecutableRunConfigurationFactory() :
    FixedRunConfigurationFactory(CustomExecutableRunConfiguration::tr("Custom Executable"))
{
    registerRunConfiguration<CustomExecutableRunConfiguration>(CUSTOM_EXECUTABLE_ID);

    addRunWorkerFactory<SimpleTargetRunner>(ProjectExplorer::Constants::NORMAL_RUN_MODE);
}

} // namespace ProjectExplorer
