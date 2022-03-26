/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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
#include "androidsdkmanagerwidget.h"

#include "ui_androidsdkmanagerwidget.h"
#include "androidconfigurations.h"
#include "androidsdkmanager.h"
#include "androidsdkmodel.h"

#include <app/app_version.h>
#include <utils/runextensions.h>
#include <utils/outputformatter.h>
#include <utils/runextensions.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QSortFilterProxyModel>

namespace {
static Q_LOGGING_CATEGORY(androidSdkMgrUiLog, "qtc.android.sdkManagerUi", QtWarningMsg)
}

namespace Android {
namespace Internal {

using namespace std::placeholders;

class PackageFilterModel : public QSortFilterProxyModel
{
public:
    PackageFilterModel(AndroidSdkModel* sdkModel);

    void setAcceptedPackageState(AndroidSdkPackage::PackageState state);
    void setAcceptedSearchPackage(const QString &text);
    bool filterAcceptsRow(int source_row, const QModelIndex &sourceParent) const override;

private:
    AndroidSdkPackage::PackageState m_packageState =  AndroidSdkPackage::AnyValidState;
    QString m_searchText;
};

AndroidSdkManagerWidget::AndroidSdkManagerWidget(AndroidConfig &config,
                                                 AndroidSdkManager *sdkManager, QWidget *parent) :
    QDialog(parent),
    m_androidConfig(config),
    m_sdkManager(sdkManager),
    m_sdkModel(new AndroidSdkModel(m_androidConfig, m_sdkManager, this)),
    m_ui(new Ui::AndroidSdkManagerWidget)
{
    QTC_CHECK(sdkManager);
    m_ui->setupUi(this);
    m_ui->sdkLicensebuttonBox->hide();
    m_ui->sdkLicenseLabel->hide();
    m_ui->viewStack->setCurrentWidget(m_ui->packagesStack);
    setModal(true);

    m_formatter = new Utils::OutputFormatter;
    m_formatter->setPlainTextEdit(m_ui->outputEdit);

    connect(m_sdkModel, &AndroidSdkModel::dataChanged, [this]() {
        if (m_ui->viewStack->currentWidget() == m_ui->packagesStack)
            m_ui->applySelectionButton->setEnabled(!m_sdkModel->userSelection().isEmpty());
    });

    connect(m_sdkModel, &AndroidSdkModel::modelAboutToBeReset, [this]() {
        m_ui->applySelectionButton->setEnabled(false);
        m_ui->expandCheck->setChecked(false);
        cancelPendingOperations();
        switchView(PackageListing);
    });

    auto proxyModel = new PackageFilterModel(m_sdkModel);
    m_ui->packagesView->setModel(proxyModel);
    m_ui->packagesView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_ui->packagesView->header()->setSectionResizeMode(AndroidSdkModel::packageNameColumn,
                                                       QHeaderView::Stretch);
    m_ui->packagesView->header()->setStretchLastSection(false);
    connect(m_ui->expandCheck, &QCheckBox::stateChanged, [this](int state) {
       if (state == Qt::Checked)
           m_ui->packagesView->expandAll();
       else
           m_ui->packagesView->collapseAll();
    });
    connect(m_ui->updateInstalledButton, &QPushButton::clicked,
            this, &AndroidSdkManagerWidget::onUpdatePackages);
    connect(m_ui->showAllRadio, &QRadioButton::toggled, [this, proxyModel](bool checked) {
        if (checked) {
            proxyModel->setAcceptedPackageState(AndroidSdkPackage::AnyValidState);
            m_sdkModel->resetSelection();
        }
    });
    connect(m_ui->showInstalledRadio, &QRadioButton::toggled, [this, proxyModel](bool checked) {
        if (checked) {
            proxyModel->setAcceptedPackageState(AndroidSdkPackage::Installed);
            m_sdkModel->resetSelection();
        }
    });
    connect(m_ui->showAvailableRadio, &QRadioButton::toggled, [this, proxyModel](bool checked) {
        if (checked) {
            proxyModel->setAcceptedPackageState(AndroidSdkPackage::Available);
            m_sdkModel->resetSelection();
        }
    });

    m_ui->searchField->setPlaceholderText("Filter");
    connect(m_ui->searchField, &QLineEdit::textChanged, [this, proxyModel](const QString &text) {
        proxyModel->setAcceptedSearchPackage(text);
        m_sdkModel->resetSelection();
        // It is more convenient to expand the view with the results
        m_ui->expandCheck->setChecked(!text.isEmpty());
    });

    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &AndroidSdkManagerWidget::close);
    connect(m_ui->applySelectionButton, &QPushButton::clicked,
            this, [this]() { onApplyButton(); });
    connect(m_ui->cancelButton, &QPushButton::clicked, this,
            &AndroidSdkManagerWidget::onCancel);
    connect(m_ui->optionsButton, &QPushButton::clicked,
            this, &AndroidSdkManagerWidget::onSdkManagerOptions);
    connect(m_ui->sdkLicensebuttonBox, &QDialogButtonBox::accepted, [this]() {
        m_sdkManager->acceptSdkLicense(true);
        m_ui->sdkLicensebuttonBox->setEnabled(false); // Wait for next license to enable controls
    });
    connect(m_ui->sdkLicensebuttonBox, &QDialogButtonBox::rejected, [this]() {
        m_sdkManager->acceptSdkLicense(false);
        m_ui->sdkLicensebuttonBox->setEnabled(false); // Wait for next license to enable controls
    });

    connect(m_ui->oboleteCheckBox, &QCheckBox::stateChanged, [this](int state) {
        const QString obsoleteArg = "--include_obsolete";
        QStringList args = m_androidConfig.sdkManagerToolArgs();
        if (state == Qt::Checked && !args.contains(obsoleteArg)) {
            args.append(obsoleteArg);
            m_androidConfig.setSdkManagerToolArgs(args);
       } else if (state == Qt::Unchecked && args.contains(obsoleteArg)) {
            args.removeAll(obsoleteArg);
            m_androidConfig.setSdkManagerToolArgs(args);
       }
        m_sdkManager->reloadPackages(true);
    });

    connect(m_ui->channelCheckbox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int index) {
        QStringList args = m_androidConfig.sdkManagerToolArgs();
        QString existingArg;
        for (int i = 0; i < 4; ++i) {
            const QString arg = "--channel=" + QString::number(i);
            if (args.contains(arg)) {
                existingArg = arg;
                break;
            }
        }

        if (index == 0 && !existingArg.isEmpty()) {
            args.removeAll(existingArg);
            m_androidConfig.setSdkManagerToolArgs(args);
        } else if (index > 0) {
            // Add 1 to account for Stable (second item) being channel 0
            const QString channelArg = "--channel=" + QString::number(index - 1);
            if (existingArg != channelArg) {
                if (!existingArg.isEmpty()) {
                    args.removeAll(existingArg);
                    m_androidConfig.setSdkManagerToolArgs(args);
                }
                args.append(channelArg);
                m_androidConfig.setSdkManagerToolArgs(args);
            }
       }
        m_sdkManager->reloadPackages(true);
    });
}

AndroidSdkManagerWidget::~AndroidSdkManagerWidget()
{
    if (m_currentOperation)
        delete m_currentOperation;
    cancelPendingOperations();
    delete m_formatter;
    delete m_ui;
}

void AndroidSdkManagerWidget::installEssentials()
{
    m_sdkModel->selectMissingEssentials();
    if (!m_sdkModel->missingEssentials().isEmpty()) {
        QMessageBox::warning(this,
                             tr("Android SDK Changes"),
                             tr("%1 cannot find the following essential packages: \"%2\".\n"
                                "Install them manually after the current operation is done.\n")
                                 .arg(Core::Constants::IDE_DISPLAY_NAME)
                                 .arg(m_sdkModel->missingEssentials().join("\", \"")));
    }
    onApplyButton(tr("Android SDK installation is missing necessary packages. "
                     "Do you want to install the missing packages?"));
}

void AndroidSdkManagerWidget::beginLicenseCheck()
{
    m_formatter->appendMessage(tr("Checking pending licenses...\n"), Utils::NormalMessageFormat);
    m_formatter->appendMessage(tr("The installation of Android SDK packages may fail if the "
                                  "respective licenses are not accepted.\n"),
                               Utils::LogMessageFormat);
    addPackageFuture(m_sdkManager->checkPendingLicenses());
}

void AndroidSdkManagerWidget::onApplyButton(const QString &extraMessage)
{
    QTC_ASSERT(m_currentView == PackageListing, return);

    if (m_sdkManager->isBusy()) {
        m_formatter->appendMessage(tr("\nSDK Manager is busy."), Utils::StdErrFormat);
        return;
    }

    const QList<const AndroidSdkPackage *> packagesToUpdate = m_sdkModel->userSelection();
    if (packagesToUpdate.isEmpty())
        return;

    QStringList installPackages, uninstallPackages;
    for (auto package : packagesToUpdate) {
        QString str = QString("   %1").arg(package->descriptionText());
        if (package->state() == AndroidSdkPackage::Installed)
            uninstallPackages << str;
        else
            installPackages << str;
    }

    QString message = tr("%n Android SDK packages shall be updated.", "", packagesToUpdate.count());
    if (!extraMessage.isEmpty())
        message.prepend(extraMessage + "\n\n");
    QMessageBox messageDlg(QMessageBox::Information, tr("Android SDK Changes"),
                           message, QMessageBox::Ok | QMessageBox::Cancel, this);

    QString details;
    if (!uninstallPackages.isEmpty())
        details = tr("[Packages to be uninstalled:]\n").append(uninstallPackages.join("\n"));

    if (!installPackages.isEmpty()) {
        if (!uninstallPackages.isEmpty())
            details.append("\n\n");
        details.append("[Packages to be installed:]\n").append(installPackages.join("\n"));
    }
    messageDlg.setDetailedText(details);
    if (messageDlg.exec() == QMessageBox::Cancel)
        return;

    // Open the SDK Manager dialog after accepting to continue with the installation
    show();

    switchView(Operations);
    m_pendingCommand = AndroidSdkManager::UpdatePackage;
    // User agreed with the selection. Check for licenses.
    if (!installPackages.isEmpty()) {
        // Pending license affects installtion only.
        beginLicenseCheck();
    } else {
        // Uninstall only. Go Ahead.
        beginExecution();
    }
}

void AndroidSdkManagerWidget::onUpdatePackages()
{
    if (m_sdkManager->isBusy()) {
        m_formatter->appendMessage(tr("\nSDK Manager is busy."), Utils::StdErrFormat);
        return;
    }
    switchView(Operations);
    m_pendingCommand = AndroidSdkManager::UpdateAll;
    beginLicenseCheck();
}

void AndroidSdkManagerWidget::onCancel()
{
    cancelPendingOperations();
}

void AndroidSdkManagerWidget::onOperationResult(int index)
{
    QTC_ASSERT(m_currentOperation, return);
    AndroidSdkManager::OperationOutput result = m_currentOperation->resultAt(index);
    if (result.type == AndroidSdkManager::LicenseWorkflow) {
        // Show license controls and enable to user input.
        m_ui->sdkLicenseLabel->setVisible(true);
        m_ui->sdkLicensebuttonBox->setVisible(true);
        m_ui->sdkLicensebuttonBox->setEnabled(true);
        m_ui->sdkLicensebuttonBox->button(QDialogButtonBox::No)->setDefault(true);
    }
    auto breakLine = [](const QString &line) { return line.endsWith("\n") ? line : line + "\n";};
    if (!result.stdError.isEmpty() && result.type != AndroidSdkManager::LicenseCheck)
        m_formatter->appendMessage(breakLine(result.stdError), Utils::StdErrFormat);
    if (!result.stdOutput.isEmpty() && result.type != AndroidSdkManager::LicenseCheck)
        m_formatter->appendMessage(breakLine(result.stdOutput), Utils::StdOutFormat);
    m_ui->outputEdit->ensureCursorVisible();
}

void AndroidSdkManagerWidget::onLicenseCheckResult(const AndroidSdkManager::OperationOutput& output)
{
    if (output.success) {
        // No assertion was found. Looks like all license are accepted. Go Ahead.
        runPendingCommand();
    } else {
        // Run license workflow.
        beginLicenseWorkflow();
    }
}

void AndroidSdkManagerWidget::addPackageFuture(const QFuture<AndroidSdkManager::OperationOutput>
                                               &future)
{
    QTC_ASSERT(!m_currentOperation, return);
    if (!future.isFinished() || !future.isCanceled()) {
        m_currentOperation = new QFutureWatcher<AndroidSdkManager::OperationOutput>;
        connect(m_currentOperation,
                &QFutureWatcher<AndroidSdkManager::OperationOutput>::resultReadyAt,
                this, &AndroidSdkManagerWidget::onOperationResult);
        connect(m_currentOperation, &QFutureWatcher<AndroidSdkManager::OperationOutput>::finished,
                this, &AndroidSdkManagerWidget::packageFutureFinished);
        connect(m_currentOperation,
                &QFutureWatcher<AndroidSdkManager::OperationOutput>::progressValueChanged,
                [this](int value) {
            m_ui->operationProgress->setValue(value);
        });
        m_currentOperation->setFuture(future);
    } else {
        qCDebug(androidSdkMgrUiLog) << "Operation canceled/finished before adding to the queue";
        if (m_sdkManager->isBusy()) {
            m_formatter->appendMessage(tr("SDK Manager is busy. Operation cancelled."),
                                       Utils::StdErrFormat);
        }
        notifyOperationFinished();
        switchView(PackageListing);
    }
}

void AndroidSdkManagerWidget::beginExecution()
{
    const QList<const AndroidSdkPackage *> packagesToUpdate = m_sdkModel->userSelection();
    if (packagesToUpdate.isEmpty()) {
        switchView(PackageListing);
        return;
    }

    QStringList installSdkPaths, uninstallSdkPaths;
    for (auto package : packagesToUpdate) {
        if (package->state() == AndroidSdkPackage::Installed)
            uninstallSdkPaths << package->sdkStylePath();
        else
            installSdkPaths << package->sdkStylePath();
    }
    m_formatter->appendMessage(tr("Installing/Uninstalling selected packages...\n"),
                               Utils::NormalMessageFormat);
    m_formatter->appendMessage(tr("Closing the %1 dialog will cancel the running and scheduled SDK "
                                  "operations.\n").arg(Utils::HostOsInfo::isMacHost() ?
                                                           tr("preferences") : tr("options")),
                               Utils::LogMessageFormat);

    addPackageFuture(m_sdkManager->update(installSdkPaths, uninstallSdkPaths));
}

void AndroidSdkManagerWidget::beginUpdate()
{
    m_formatter->appendMessage(tr("Updating installed packages...\n"), Utils::NormalMessageFormat);
    m_formatter->appendMessage(tr("Closing the %1 dialog will cancel the running and scheduled SDK "
                                  "operations.\n").arg(Utils::HostOsInfo::isMacHost() ?
                                                           tr("preferences") : tr("options")),
                               Utils::LogMessageFormat);
    addPackageFuture(m_sdkManager->updateAll());
}

void AndroidSdkManagerWidget::beginLicenseWorkflow()
{
    switchView(LicenseWorkflow);
    addPackageFuture(m_sdkManager->runLicenseCommand());
}

void AndroidSdkManagerWidget::notifyOperationFinished()
{
    if (!m_currentOperation || m_currentOperation->isFinished()) {
        QMessageBox::information(this, tr("Android SDK Changes"),
                                 tr("Android SDK operations finished."), QMessageBox::Ok);
        m_ui->operationProgress->setValue(0);
        // Once the update/install is done, let's hide the dialog.
        hide();
    }
}

void AndroidSdkManagerWidget::packageFutureFinished()
{
    QTC_ASSERT (m_currentOperation, return);

    bool continueWorkflow = true;
    if (m_currentOperation->isCanceled()) {
        m_formatter->appendMessage(tr("Operation cancelled.\n"), Utils::StdErrFormat);
        continueWorkflow = false;
    }
    m_ui->operationProgress->setValue(100);
    int resultCount = m_currentOperation->future().resultCount();
    if (continueWorkflow && resultCount > 0) {
        AndroidSdkManager::OperationOutput output = m_currentOperation->resultAt(resultCount -1);
        AndroidSdkManager::CommandType type = output.type;
        m_currentOperation->deleteLater();
        m_currentOperation = nullptr;
        switch (type) {
        case AndroidSdkManager::LicenseCheck:
            onLicenseCheckResult(output);
            break;
        case AndroidSdkManager::LicenseWorkflow:
            m_ui->sdkLicensebuttonBox->hide();
            m_ui->sdkLicenseLabel->hide();
            runPendingCommand();
            break;
        case AndroidSdkManager::UpdateAll:
        case AndroidSdkManager::UpdatePackage:
            notifyOperationFinished();
            switchView(PackageListing);
            m_sdkManager->reloadPackages(true);
            break;
        default:
            break;
        }
    } else {
        m_currentOperation->deleteLater();
        m_currentOperation = nullptr;
        switchView(PackageListing);
        m_sdkManager->reloadPackages(true);
    }
}

void AndroidSdkManagerWidget::cancelPendingOperations()
{
    if (!m_sdkManager->isBusy()) {
        m_formatter->appendMessage(tr("\nNo pending operations to cancel...\n"),
                                   Utils::NormalMessageFormat);
        switchView(PackageListing);
        return;
    }
    m_formatter->appendMessage(tr("\nCancelling pending operations...\n"),
                               Utils::NormalMessageFormat);
    m_sdkManager->cancelOperatons();
}

void AndroidSdkManagerWidget::switchView(AndroidSdkManagerWidget::View view)
{
    if (m_currentView == PackageListing)
        m_formatter->clear();
    m_currentView = view;
    if (m_currentView == PackageListing) {
        // We need the buttonBox only in the main listing view, as the license and update
        // views already have a cancel button.
        m_ui->buttonBox->button(QDialogButtonBox::Ok)->setVisible(true);
        emit updatingSdkFinished();
    } else {
        m_ui->buttonBox->button(QDialogButtonBox::Ok)->setVisible(false);
        emit updatingSdk();
    }

    if (m_currentView == LicenseWorkflow)
        emit licenseWorkflowStarted();

    m_ui->operationProgress->setValue(0);
    m_ui->viewStack->setCurrentWidget(m_currentView == PackageListing ?
                                          m_ui->packagesStack : m_ui->outputStack);
}

void AndroidSdkManagerWidget::runPendingCommand()
{
    if (m_pendingCommand == AndroidSdkManager::UpdatePackage)
        beginExecution(); // License workflow can only start when updating packages.
    else if (m_pendingCommand == AndroidSdkManager::UpdateAll)
        beginUpdate();
    else
        QTC_ASSERT(false, qCDebug(androidSdkMgrUiLog) << "Unexpected state: No pending command.");
}

void AndroidSdkManagerWidget::onSdkManagerOptions()
{
    OptionsDialog dlg(m_sdkManager, m_androidConfig.sdkManagerToolArgs(), this);
    if (dlg.exec() == QDialog::Accepted) {
        QStringList arguments = dlg.sdkManagerArguments();
        if (arguments != m_androidConfig.sdkManagerToolArgs()) {
            m_androidConfig.setSdkManagerToolArgs(arguments);
            m_sdkManager->reloadPackages(true);
        }
    }
}

PackageFilterModel::PackageFilterModel(AndroidSdkModel *sdkModel) :
    QSortFilterProxyModel(sdkModel)
{
    setSourceModel(sdkModel);
}

void PackageFilterModel::setAcceptedPackageState(AndroidSdkPackage::PackageState state)
{
    m_packageState = state;
    invalidateFilter();
}

void PackageFilterModel::setAcceptedSearchPackage(const QString &name)
{
    m_searchText = name;
    invalidateFilter();
}

bool PackageFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex srcIndex = sourceModel()->index(sourceRow, 0, sourceParent);
    if (!srcIndex.isValid())
        return false;

    auto packageState = [](const QModelIndex& i) {
      return (AndroidSdkPackage::PackageState)i.data(AndroidSdkModel::PackageStateRole).toInt();
    };

    auto packageFound = [this](const QModelIndex& i) {
        return i.data(AndroidSdkModel::packageNameColumn).toString()
                .contains(m_searchText, Qt::CaseInsensitive);
    };

    bool showTopLevel = false;
    if (!sourceParent.isValid()) {
        // Top Level items
        for (int row = 0; row < sourceModel()->rowCount(srcIndex); ++row) {
            QModelIndex childIndex = sourceModel()->index(row, 0, srcIndex);
            if ((m_packageState & packageState(childIndex) && packageFound(childIndex))) {
                showTopLevel = true;
                break;
            }
        }
    }

    return showTopLevel || ((packageState(srcIndex) & m_packageState) && packageFound(srcIndex));
}

OptionsDialog::OptionsDialog(AndroidSdkManager *sdkManager, const QStringList &args,
                             QWidget *parent) : QDialog(parent)
{
    QTC_CHECK(sdkManager);
    resize(800, 480);
    setWindowTitle(tr("SDK Manager Arguments"));

    argumentDetailsEdit = new QPlainTextEdit(this);
    argumentDetailsEdit->setReadOnly(true);

    auto populateOptions = [this](const QString& options) {
        if (options.isEmpty()) {
            argumentDetailsEdit->setPlainText(tr("Cannot load available arguments for "
                                                 "\"sdkmanager\" command."));
        } else {
            argumentDetailsEdit->setPlainText(options);
        }
    };
    m_optionsFuture = sdkManager->availableArguments();
    Utils::onResultReady(m_optionsFuture, populateOptions);

    auto dialogButtons = new QDialogButtonBox(this);
    dialogButtons->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    connect(dialogButtons, &QDialogButtonBox::accepted, this, &OptionsDialog::accept);
    connect(dialogButtons, &QDialogButtonBox::rejected, this, &OptionsDialog::reject);

    argumentsEdit = new QLineEdit(this);
    argumentsEdit->setText(args.join(" "));

    auto gridLayout = new QGridLayout(this);
    gridLayout->addWidget(new QLabel(tr("SDK manager arguments:"), this), 0, 0, 1, 1);
    gridLayout->addWidget(argumentsEdit, 0, 1, 1, 1);
    gridLayout->addWidget(new QLabel(tr("Available arguments:"), this), 1, 0, 1, 2);
    gridLayout->addWidget(argumentDetailsEdit, 2, 0, 1, 2);
    gridLayout->addWidget(dialogButtons, 3, 0, 1, 2);
}

OptionsDialog::~OptionsDialog()
{
    m_optionsFuture.cancel();
    m_optionsFuture.waitForFinished();
}

QStringList OptionsDialog::sdkManagerArguments() const
{
    QString userInput = argumentsEdit->text().simplified();
    return userInput.isEmpty() ? QStringList() : userInput.split(' ');
}

} // namespace Internal
} // namespace Android
