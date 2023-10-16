// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidconfigurations.h"
#include "androidsdkmanager.h"
#include "androidsdkmanagerwidget.h"
#include "androidsdkmodel.h"
#include "androidtr.h"

#include <utils/async.h>
#include <utils/layoutbuilder.h>
#include <utils/outputformatter.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QAbstractButton>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGuiApplication>
#include <QLabel>
#include <QLineEdit>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QSortFilterProxyModel>

using namespace Utils;
using namespace std::placeholders;

namespace Android::Internal {

static Q_LOGGING_CATEGORY(androidSdkMgrUiLog, "qtc.android.sdkManagerUi", QtWarningMsg)

class PackageFilterModel : public QSortFilterProxyModel
{
public:
    PackageFilterModel(AndroidSdkModel *sdkModel);

    void setAcceptedPackageState(AndroidSdkPackage::PackageState state);
    void setAcceptedSearchPackage(const QString &text);
    bool filterAcceptsRow(int source_row, const QModelIndex &sourceParent) const override;

private:
    AndroidSdkPackage::PackageState m_packageState = AndroidSdkPackage::AnyValidState;
    QString m_searchText;
};

AndroidSdkManagerWidget::AndroidSdkManagerWidget(AndroidConfig &config,
                                                 AndroidSdkManager *sdkManager, QWidget *parent) :
    QDialog(parent),
    m_androidConfig(config),
    m_sdkManager(sdkManager),
    m_sdkModel(new AndroidSdkModel(m_androidConfig, m_sdkManager, this))
{
    QTC_CHECK(sdkManager);

    setWindowTitle(Tr::tr("Android SDK Manager"));
    resize(664, 396);
    setModal(true);

    m_packagesStack = new QWidget;

    auto packagesView = new QTreeView(m_packagesStack);
    packagesView->setIndentation(20);
    packagesView->header()->setCascadingSectionResizes(false);

    auto updateInstalledButton = new QPushButton(Tr::tr("Update Installed"));

    auto channelCheckbox = new QComboBox;
    channelCheckbox->addItem(Tr::tr("Default"));
    channelCheckbox->addItem(Tr::tr("Stable"));
    channelCheckbox->addItem(Tr::tr("Beta"));
    channelCheckbox->addItem(Tr::tr("Dev"));
    channelCheckbox->addItem(Tr::tr("Canary"));

    auto obsoleteCheckBox = new QCheckBox(Tr::tr("Include obsolete"));

    auto showAvailableRadio = new QRadioButton(Tr::tr("Available"));
    auto showInstalledRadio = new QRadioButton(Tr::tr("Installed"));
    auto showAllRadio = new QRadioButton(Tr::tr("All"));
    showAllRadio->setChecked(true);

    auto optionsButton = new QPushButton(Tr::tr("Advanced Options..."));

    auto searchField = new FancyLineEdit(m_packagesStack);
    searchField->setPlaceholderText("Filter");

    auto expandCheck = new QCheckBox(Tr::tr("Expand All"));

    m_outputStack = new QWidget;
    m_operationProgress = new QProgressBar(m_outputStack);

    m_outputEdit = new QPlainTextEdit(m_outputStack);
    m_outputEdit->setReadOnly(true);

    m_sdkLicenseLabel = new QLabel(Tr::tr("Do you want to accept the Android SDK license?"));
    m_sdkLicenseLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    m_sdkLicenseLabel->hide();

    m_sdkLicenseButtonBox = new QDialogButtonBox(m_outputStack);
    m_sdkLicenseButtonBox->setEnabled(false);
    m_sdkLicenseButtonBox->setStandardButtons(QDialogButtonBox::No|QDialogButtonBox::Yes);
    m_sdkLicenseButtonBox->hide();

    m_buttonBox = new QDialogButtonBox(this);
    m_buttonBox->setStandardButtons(QDialogButtonBox::Apply | QDialogButtonBox::Cancel);
    m_buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);

    m_viewStack = new QStackedWidget(this);
    m_viewStack->addWidget(m_packagesStack);
    m_viewStack->addWidget(m_outputStack);
    m_viewStack->setCurrentWidget(m_packagesStack);

    m_formatter = new OutputFormatter;
    m_formatter->setPlainTextEdit(m_outputEdit);

    auto proxyModel = new PackageFilterModel(m_sdkModel);
    packagesView->setModel(proxyModel);
    packagesView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    packagesView->header()->setSectionResizeMode(AndroidSdkModel::packageNameColumn,
                                                       QHeaderView::Stretch);
    packagesView->header()->setStretchLastSection(false);

    using namespace Layouting;
    Grid {
        searchField, expandCheck, br,

        Span(2, packagesView),
        Column {
            updateInstalledButton,
            st,
            Group {
                title(Tr::tr("Show Packages")),
                Column {
                    Row { Tr::tr("Channel:"), channelCheckbox },
                    obsoleteCheckBox,
                    hr,
                    showAvailableRadio,
                    showInstalledRadio,
                    showAllRadio,
                }
            },
            optionsButton
        },
        noMargin
    }.attachTo(m_packagesStack);

    Column {
        m_outputEdit,
        Row { m_sdkLicenseLabel, m_sdkLicenseButtonBox },
        m_operationProgress,
        noMargin
    }.attachTo(m_outputStack);

    Column {
        m_viewStack,
        m_buttonBox
    }.attachTo(this);

    connect(m_sdkModel, &AndroidSdkModel::dataChanged, this, [this] {
        if (m_viewStack->currentWidget() == m_packagesStack)
            m_buttonBox->button(QDialogButtonBox::Apply)
                ->setEnabled(!m_sdkModel->userSelection().isEmpty());
    });

    connect(m_sdkModel, &AndroidSdkModel::modelAboutToBeReset, this,
            [this, expandCheck] {
        m_buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
        expandCheck->setChecked(false);
        cancelPendingOperations();
        switchView(PackageListing);
    });

    connect(expandCheck, &QCheckBox::stateChanged, this, [packagesView](int state) {
        if (state == Qt::Checked)
            packagesView->expandAll();
        else
            packagesView->collapseAll();
    });
    connect(updateInstalledButton, &QPushButton::clicked,
            this, &AndroidSdkManagerWidget::onUpdatePackages);
    connect(showAllRadio, &QRadioButton::toggled, this, [this, proxyModel](bool checked) {
        if (checked) {
            proxyModel->setAcceptedPackageState(AndroidSdkPackage::AnyValidState);
            m_sdkModel->resetSelection();
        }
    });
    connect(showInstalledRadio, &QRadioButton::toggled,
            this, [this, proxyModel](bool checked) {
        if (checked) {
            proxyModel->setAcceptedPackageState(AndroidSdkPackage::Installed);
            m_sdkModel->resetSelection();
        }
    });
    connect(showAvailableRadio, &QRadioButton::toggled, this, [this, proxyModel](bool checked) {
        if (checked) {
            proxyModel->setAcceptedPackageState(AndroidSdkPackage::Available);
            m_sdkModel->resetSelection();
        }
    });

    connect(searchField, &QLineEdit::textChanged,
            this, [this, proxyModel, expandCheck](const QString &text) {
        proxyModel->setAcceptedSearchPackage(text);
        m_sdkModel->resetSelection();
        // It is more convenient to expand the view with the results
        expandCheck->setChecked(!text.isEmpty());
    });

    connect(m_buttonBox->button(QDialogButtonBox::Apply), &QAbstractButton::clicked, this, [this] {
        onApplyButton();
    });
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &AndroidSdkManagerWidget::onCancel);

    connect(optionsButton, &QPushButton::clicked,
            this, &AndroidSdkManagerWidget::onSdkManagerOptions);
    connect(m_sdkLicenseButtonBox, &QDialogButtonBox::accepted, this, [this] {
        m_sdkManager->acceptSdkLicense(true);
        m_sdkLicenseButtonBox->setEnabled(false); // Wait for next license to enable controls
    });
    connect(m_sdkLicenseButtonBox, &QDialogButtonBox::rejected, this, [this] {
        m_sdkManager->acceptSdkLicense(false);
        m_sdkLicenseButtonBox->setEnabled(false); // Wait for next license to enable controls
    });

    connect(obsoleteCheckBox, &QCheckBox::stateChanged, this, [this](int state) {
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

    connect(channelCheckbox, &QComboBox::currentIndexChanged, this, [this](int index) {
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
}

void AndroidSdkManagerWidget::installEssentials()
{
    m_sdkModel->selectMissingEssentials();
    if (!m_sdkModel->missingEssentials().isEmpty()) {
        QMessageBox::warning(this,
                             Tr::tr("Android SDK Changes"),
                             Tr::tr("%1 cannot find the following essential packages: \"%2\".\n"
                                    "Install them manually after the current operation is done.\n")
                                 .arg(QGuiApplication::applicationDisplayName(),
                                      m_sdkModel->missingEssentials().join("\", \"")));
    }
    onApplyButton(Tr::tr("Android SDK installation is missing necessary packages. "
                     "Do you want to install the missing packages?"));
}

void AndroidSdkManagerWidget::beginLicenseCheck()
{
    m_formatter->appendMessage(Tr::tr("Checking pending licenses...") + "\n", NormalMessageFormat);
    m_formatter->appendMessage(Tr::tr("The installation of Android SDK packages may fail if the "
                                      "respective licenses are not accepted.")
                                   + "\n",
                               LogMessageFormat);
    addPackageFuture(m_sdkManager->checkPendingLicenses());
}

void AndroidSdkManagerWidget::onApplyButton(const QString &extraMessage)
{
    QTC_ASSERT(m_currentView == PackageListing, return);

    if (m_sdkManager->isBusy()) {
        m_formatter->appendMessage("\n" + Tr::tr("SDK Manager is busy."), StdErrFormat);
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

    QString message = Tr::tr("%n Android SDK packages shall be updated.", "", packagesToUpdate.count());
    if (!extraMessage.isEmpty())
        message.prepend(extraMessage + "\n\n");
    QMessageBox messageDlg(QMessageBox::Information, Tr::tr("Android SDK Changes"),
                           message, QMessageBox::Ok | QMessageBox::Cancel, this);

    QString details;
    if (!uninstallPackages.isEmpty())
        details = Tr::tr("[Packages to be uninstalled:]\n").append(uninstallPackages.join("\n"));

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
        m_formatter->appendMessage("\n" + Tr::tr("SDK Manager is busy."), StdErrFormat);
        return;
    }
    switchView(Operations);
    m_pendingCommand = AndroidSdkManager::UpdateAll;
    beginLicenseCheck();
}

void AndroidSdkManagerWidget::onCancel()
{
    cancelPendingOperations();
    close();
}

void AndroidSdkManagerWidget::onOperationResult(int index)
{
    QTC_ASSERT(m_currentOperation, return);
    AndroidSdkManager::OperationOutput result = m_currentOperation->resultAt(index);
    if (result.type == AndroidSdkManager::LicenseWorkflow) {
        // Show license controls and enable to user input.
        m_sdkLicenseLabel->setVisible(true);
        m_sdkLicenseButtonBox->setVisible(true);
        m_sdkLicenseButtonBox->setEnabled(true);
        m_sdkLicenseButtonBox->button(QDialogButtonBox::No)->setDefault(true);
    }
    auto breakLine = [](const QString &line) { return line.endsWith("\n") ? line : line + "\n";};
    if (!result.stdError.isEmpty() && result.type != AndroidSdkManager::LicenseCheck)
        m_formatter->appendMessage(breakLine(result.stdError), StdErrFormat);
    if (!result.stdOutput.isEmpty() && result.type != AndroidSdkManager::LicenseCheck)
        m_formatter->appendMessage(breakLine(result.stdOutput), StdOutFormat);
    m_outputEdit->ensureCursorVisible();
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
        connect(m_currentOperation, &QFutureWatcherBase::resultReadyAt,
                this, &AndroidSdkManagerWidget::onOperationResult);
        connect(m_currentOperation, &QFutureWatcherBase::finished,
                this, &AndroidSdkManagerWidget::packageFutureFinished);
        connect(m_currentOperation, &QFutureWatcherBase::progressValueChanged,
                this, [this](int value) {
            m_operationProgress->setValue(value);
        });
        m_currentOperation->setFuture(future);
    } else {
        qCDebug(androidSdkMgrUiLog) << "Operation canceled/finished before adding to the queue";
        if (m_sdkManager->isBusy()) {
            m_formatter->appendMessage(Tr::tr("SDK Manager is busy. Operation cancelled."),
                                       StdErrFormat);
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
    m_formatter->appendMessage(Tr::tr("Installing/Uninstalling selected packages...\n"),
                               NormalMessageFormat);
    m_formatter->appendMessage(Tr::tr("Closing the %1 dialog will cancel the running and scheduled SDK "
                                  "operations.\n").arg(HostOsInfo::isMacHost() ?
                                                           Tr::tr("preferences") : Tr::tr("options")),
                               LogMessageFormat);

    addPackageFuture(m_sdkManager->update(installSdkPaths, uninstallSdkPaths));
}

void AndroidSdkManagerWidget::beginUpdate()
{
    m_formatter->appendMessage(Tr::tr("Updating installed packages...\n"), NormalMessageFormat);
    m_formatter->appendMessage(Tr::tr("Closing the %1 dialog will cancel the running and scheduled SDK "
                                  "operations.\n").arg(HostOsInfo::isMacHost() ?
                                                           Tr::tr("preferences") : Tr::tr("options")),
                               LogMessageFormat);
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
        QMessageBox::information(this, Tr::tr("Android SDK Changes"),
                                 Tr::tr("Android SDK operations finished."), QMessageBox::Ok);
        m_operationProgress->setValue(0);
        // Once the update/install is done, let's hide the dialog.
        hide();
    }
}

void AndroidSdkManagerWidget::packageFutureFinished()
{
    QTC_ASSERT (m_currentOperation, return);

    bool continueWorkflow = true;
    if (m_currentOperation->isCanceled()) {
        m_formatter->appendMessage(Tr::tr("Operation cancelled.\n"), StdErrFormat);
        continueWorkflow = false;
    }
    m_operationProgress->setValue(100);
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
            m_sdkLicenseButtonBox->hide();
            m_sdkLicenseLabel->hide();
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
        m_formatter->appendMessage(Tr::tr("\nNo pending operations to cancel...\n"),
                                   NormalMessageFormat);
        switchView(PackageListing);
        return;
    }
    m_formatter->appendMessage(Tr::tr("\nCancelling pending operations...\n"),
                               NormalMessageFormat);
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
        m_buttonBox->button(QDialogButtonBox::Apply)->setVisible(true);
        emit updatingSdkFinished();
    } else {
        m_buttonBox->button(QDialogButtonBox::Apply)->setVisible(false);
        emit updatingSdk();
    }

    if (m_currentView == LicenseWorkflow)
        emit licenseWorkflowStarted();

    m_operationProgress->setValue(0);
    m_viewStack->setCurrentWidget(m_currentView == PackageListing ?
                                          m_packagesStack : m_outputStack);
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
    setWindowTitle(Tr::tr("SDK Manager Arguments"));

    m_argumentDetailsEdit = new QPlainTextEdit(this);
    m_argumentDetailsEdit->setReadOnly(true);

    auto populateOptions = [this](const QString& options) {
        if (options.isEmpty()) {
            m_argumentDetailsEdit->setPlainText(Tr::tr("Cannot load available arguments for "
                                                       "\"sdkmanager\" command."));
        } else {
            m_argumentDetailsEdit->setPlainText(options);
        }
    };
    m_optionsFuture = sdkManager->availableArguments();
    Utils::onResultReady(m_optionsFuture, this, populateOptions);

    auto dialogButtons = new QDialogButtonBox(this);
    dialogButtons->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    connect(dialogButtons, &QDialogButtonBox::accepted, this, &OptionsDialog::accept);
    connect(dialogButtons, &QDialogButtonBox::rejected, this, &OptionsDialog::reject);

    m_argumentsEdit = new QLineEdit(this);
    m_argumentsEdit->setText(args.join(" "));

    auto gridLayout = new QGridLayout(this);
    gridLayout->addWidget(new QLabel(Tr::tr("SDK manager arguments:"), this), 0, 0, 1, 1);
    gridLayout->addWidget(m_argumentsEdit, 0, 1, 1, 1);
    gridLayout->addWidget(new QLabel(Tr::tr("Available arguments:"), this), 1, 0, 1, 2);
    gridLayout->addWidget(m_argumentDetailsEdit, 2, 0, 1, 2);
    gridLayout->addWidget(dialogButtons, 3, 0, 1, 2);
}

OptionsDialog::~OptionsDialog()
{
    m_optionsFuture.cancel();
    m_optionsFuture.waitForFinished();
}

QStringList OptionsDialog::sdkManagerArguments() const
{
    QString userInput = m_argumentsEdit->text().simplified();
    return userInput.isEmpty() ? QStringList() : userInput.split(' ');
}

} // Android::Internal
