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

#include "utils/runextensions.h"
#include "utils/outputformatter.h"
#include "utils/runextensions.h"
#include "utils/qtcassert.h"
#include "utils/utilsicons.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QProcess>
#include <QSortFilterProxyModel>

namespace {
Q_LOGGING_CATEGORY(androidSdkMgrUiLog, "qtc.android.sdkManagerUi")
}

namespace Android {
namespace Internal {

using namespace std::placeholders;

class PackageFilterModel : public QSortFilterProxyModel
{
public:
    PackageFilterModel(AndroidSdkModel* sdkModel);

    void setAcceptedPackageState(AndroidSdkPackage::PackageState state);
    bool filterAcceptsRow(int source_row, const QModelIndex &sourceParent) const override;

private:
    AndroidSdkPackage::PackageState m_packageState =  AndroidSdkPackage::AnyValidState;
};

AndroidSdkManagerWidget::AndroidSdkManagerWidget(AndroidConfig &config,
                                                 AndroidSdkManager *sdkManager, QWidget *parent) :
    QWidget(parent),
    m_androidConfig(config),
    m_sdkManager(sdkManager),
    m_sdkModel(new AndroidSdkModel(m_androidConfig, m_sdkManager, this)),
    m_ui(new Ui::AndroidSdkManagerWidget)
{
    QTC_CHECK(sdkManager);
    m_ui->setupUi(this);
    m_ui->sdkLicensebuttonBox->hide();
    m_ui->sdkLicenseLabel->hide();
    m_ui->warningLabel->setElideMode(Qt::ElideRight);
    m_ui->warningIconLabel->setPixmap(Utils::Icons::WARNING.pixmap());
    m_ui->viewStack->setCurrentWidget(m_ui->packagesStack);

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
    m_ui->packagesView->header()->setSectionResizeMode(AndroidSdkModel::packageNameColumn,
                                                       QHeaderView::ResizeToContents);
    m_ui->packagesView->header()->setSectionResizeMode(AndroidSdkModel::apiLevelColumn,
                                                       QHeaderView::ResizeToContents);
    m_ui->packagesView->header()->setSectionResizeMode(AndroidSdkModel::packageRevisionColumn,
                                                       QHeaderView::ResizeToContents);
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

    connect(m_ui->applySelectionButton, &QPushButton::clicked,
            this, &AndroidSdkManagerWidget::onApplyButton);
    connect(m_ui->cancelButton, &QPushButton::clicked, this,
            &AndroidSdkManagerWidget::onCancel);
    connect(m_ui->nativeSdkManagerButton, &QPushButton::clicked,
            this, &AndroidSdkManagerWidget::onNativeSdkManager);
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
}

AndroidSdkManagerWidget::~AndroidSdkManagerWidget()
{
    if (m_currentOperation)
        delete m_currentOperation;
    cancelPendingOperations();
    delete m_formatter;
    delete m_ui;
}

void AndroidSdkManagerWidget::setSdkManagerControlsEnabled(bool enable)
{
    m_ui->packagesTypeGroup->setEnabled(enable);
    m_ui->expandCheck->setVisible(enable);
    m_ui->warningIconLabel->setVisible(!enable);
    m_ui->warningLabel->setVisible(!enable);
    m_ui->packagesView->setEnabled(enable);
    m_ui->updateInstalledButton->setEnabled(enable);
    m_ui->optionsButton->setEnabled(enable);
}

void AndroidSdkManagerWidget::installEssentials()
{
    m_sdkModel->selectMissingEssentials();
    m_ui->applySelectionButton->click();
}

void AndroidSdkManagerWidget::beginLicenseCheck()
{
    m_formatter->appendMessage(tr("Checking pending licenses...\n"), Utils::NormalMessageFormat);
    addPackageFuture(m_sdkManager->checkPendingLicenses());
}

void AndroidSdkManagerWidget::onApplyButton()
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

    QMessageBox messageDlg(QMessageBox::Information, tr("Android SDK Changes"),
                           tr("%n Android SDK packages shall be updated.",
                              "", packagesToUpdate.count()),
                           QMessageBox::Ok | QMessageBox::Cancel, this);

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

void AndroidSdkManagerWidget::onNativeSdkManager()
{
    if (m_androidConfig.useNativeUiTools()) {
        QProcess::startDetached(m_androidConfig.androidToolPath().toString());
    } else {
        QMessageBox::warning(this, tr("Native SDK Manager Not Available"),
                             tr("SDK manager UI tool is not available in the installed SDK tools "
                                "(version %1). Use the command line tool \"sdkmanager\" for "
                                "advanced SDK management.")
                             .arg(m_androidConfig.sdkToolsVersion().toString()));
    }
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
        // Assertion was found. Provide user workflow to accept licenses.
        QString warningMessage = tr("\nPlease note that the installation and use of Android SDK "
                                    "packages may fail if respective licenses are not accepted.");
        int userSelection = QMessageBox::question(this, tr("Android SDK Licenses"),
                                                  output.stdOutput + warningMessage,
                                                  QMessageBox::Yes | QMessageBox::No);
        if (userSelection == QMessageBox::Yes) {
            // Run license workflow.
            beginLicenseWorkflow();
        } else {
            // User decided to go ahead anyways.
            runPendingCommand();
        }
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
    if (m_currentView == PackageListing) {
        m_formatter->clear();
        m_ui->outputEdit->clear();
    }
    m_currentView = view;
    if (m_currentView == PackageListing)
        emit updatingSdkFinished();
    else
        emit updatingSdk();

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

bool PackageFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex srcIndex = sourceModel()->index(sourceRow, 0, sourceParent);
    if (!srcIndex.isValid())
        return false;

    auto packageState = [](const QModelIndex& i) {
      return (AndroidSdkPackage::PackageState)i.data(AndroidSdkModel::PackageStateRole).toInt();
    };

    bool showTopLevel = false;
    if (!sourceParent.isValid()) {
        // Top Level items
        for (int row = 0; row < sourceModel()->rowCount(srcIndex); ++row) {
            QModelIndex childIndex = sourceModel()->index(row, 0, srcIndex);
            if (m_packageState & packageState(childIndex)) {
                showTopLevel = true;
                break;
            }
        }
    }

    return showTopLevel || (packageState(srcIndex) & m_packageState);
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
