// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "startremotedialog.h"

#include "valgrindtr.h"

#include <coreplugin/icore.h>

#include <debugger/analyzer/analyzerutils.h>
#include <debugger/debuggerconstants.h>
#include <debugger/debuggermainwindow.h>

#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/sshparameters.h>
#include <projectexplorer/kitchooser.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/taskhub.h>

#include <utils/fancylineedit.h>
#include <utils/filepath.h>
#include <utils/pathchooser.h>
#include <utils/processinterface.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>

using namespace ProjectExplorer;
using namespace Utils;

namespace Valgrind::Internal {

class StartRemoteDialog : public QDialog
{
public:
    StartRemoteDialog();

    CommandLine commandLine() const;
    FilePath workingDirectory() const;

private:
    void validate();
    void accept() override;

    KitChooser *m_kitChooser;
    PathChooser *m_executable;
    FancyLineEdit *m_arguments;
    PathChooser *m_workingDirectory;
    QDialogButtonBox *m_buttonBox;
};

StartRemoteDialog::StartRemoteDialog()
    : QDialog(Core::ICore::dialogParent())
{
    setWindowTitle(Tr::tr("Start Remote Analysis"));

    m_kitChooser = new KitChooser(this);
    m_kitChooser->setKitPredicate([](const Kit *kit) {
        const IDevice::ConstPtr device = RunDeviceKitAspect::device(kit);
        return kit->isValid() && device && !device->sshParameters().host().isEmpty();
    });
    m_executable = new PathChooser(this);
    m_executable->setExpectedKind(PathChooser::ExistingCommand);
    m_arguments = new FancyLineEdit(this);
    m_arguments->setClearButtonEnabled(true);
    m_arguments->setMinimumWidth(500);
    m_workingDirectory = new PathChooser(this);

    m_buttonBox = new QDialogButtonBox(this);
    m_buttonBox->setOrientation(Qt::Horizontal);
    m_buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    auto formLayout = new QFormLayout;
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    formLayout->addRow(Tr::tr("Kit:"), m_kitChooser);
    formLayout->addRow(Tr::tr("Executable:"), m_executable);
    formLayout->addRow(Tr::tr("Arguments:"), m_arguments);
    formLayout->addRow(Tr::tr("Working directory:"), m_workingDirectory);

    auto verticalLayout = new QVBoxLayout(this);
    verticalLayout->addLayout(formLayout);
    verticalLayout->addWidget(m_buttonBox);

    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup("AnalyzerStartRemoteDialog");
    m_kitChooser->populate();
    m_kitChooser->setCurrentKitId(Id::fromSetting(settings->value("profile")));
    const QString executable = settings->value("executable").toString();
    m_executable->setFilePath(FilePath::fromString(executable));
    const QString workingDirectory = settings->value("workingDirectory").toString();
    m_workingDirectory->setFilePath(FilePath::fromString(workingDirectory));
    m_arguments->setText(settings->value("arguments").toString());
    settings->endGroup();

    connect(m_kitChooser, &KitChooser::activated, this, &StartRemoteDialog::validate);
    connect(m_executable, &PathChooser::validChanged, this, &StartRemoteDialog::validate);
    connect(m_workingDirectory, &PathChooser::validChanged, this, &StartRemoteDialog::validate);
    connect(m_arguments, &QLineEdit::textChanged, this, &StartRemoteDialog::validate);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &StartRemoteDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &StartRemoteDialog::reject);

    validate();
}

void StartRemoteDialog::accept()
{
    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup("AnalyzerStartRemoteDialog");
    settings->setValue("profile", m_kitChooser->currentKitId().toString());
    settings->setValue("executable", m_executable->filePath().toFSPathString());
    settings->setValue("workingDirectory", m_workingDirectory->filePath().toFSPathString());
    settings->setValue("arguments", m_arguments->text());
    settings->endGroup();

    QDialog::accept();
}

void StartRemoteDialog::validate()
{
    const bool valid = m_executable->isValid();
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
}

CommandLine StartRemoteDialog::commandLine() const
{
    const Kit *kit = m_kitChooser->currentKit();
    const QString executable = m_executable->filePath().toFSPathString();
    const FilePath filePath = RunDeviceKitAspect::deviceFilePath(kit, executable);
    return {filePath, m_arguments->text(), CommandLine::Raw};
}

FilePath StartRemoteDialog::workingDirectory() const
{
    return m_workingDirectory->filePath();
}

void setupExternalAnalyzer(QAction *action, Perspective *perspective, Id runMode)
{
    QObject::connect(action, &QAction::triggered, perspective, [action, perspective, runMode] {
        RunConfiguration *runConfig = activeRunConfigForActiveProject();
        if (!runConfig) {
            Debugger::showCannotStartDialog(action->text());
            return;
        }
        StartRemoteDialog dlg;
        if (dlg.exec() != QDialog::Accepted)
            return;

        TaskHub::clearTasks(Debugger::Constants::ANALYZERTASK_ID);
        perspective->select();
        RunControl *runControl = new RunControl(runMode);
        runControl->copyDataFromRunConfiguration(runConfig);
        runControl->createMainRecipe();
        runControl->setCommandLine(dlg.commandLine());
        runControl->setWorkingDirectory(dlg.workingDirectory());
        runControl->start();
    });
}

} // Valgrind::Internal
