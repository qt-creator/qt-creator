// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "androidconfigurations.h"
#include "androidsdkmanager.h"

#include <QDialog>
#include <QFutureWatcher>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPlainTextEdit;
QT_END_NAMESPACE

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QWidget>
#include <utils/fancylineedit.h>

namespace Utils { class OutputFormatter; }

namespace Android::Internal {

class AndroidSdkManager;
class AndroidSdkModel;

class OptionsDialog : public QDialog
{
public:
    OptionsDialog(AndroidSdkManager *sdkManager, const QStringList &args,
                  QWidget *parent = nullptr);
    ~OptionsDialog() override;

    QStringList sdkManagerArguments() const;

private:
    QPlainTextEdit *m_argumentDetailsEdit;
    QLineEdit *m_argumentsEdit;
    QFuture<QString> m_optionsFuture;
};

class AndroidSdkManagerWidget : public QDialog
{
    Q_OBJECT

    enum View {
        PackageListing,
        Operations,
        LicenseWorkflow
    };

public:
    AndroidSdkManagerWidget(AndroidConfig &config, AndroidSdkManager *sdkManager,
                            QWidget *parent = nullptr);
    ~AndroidSdkManagerWidget() override;

    void installEssentials();

signals:
    void updatingSdk();
    void updatingSdkFinished();
    void licenseWorkflowStarted();

private:
    void onApplyButton(const QString &extraMessage = {});
    void onUpdatePackages();
    void onCancel();
    void onOperationResult(int index);
    void onLicenseCheckResult(const AndroidSdkManager::OperationOutput &output);
    void onSdkManagerOptions();
    void addPackageFuture(const QFuture<AndroidSdkManager::OperationOutput> &future);
    void beginLicenseCheck();
    void beginExecution();
    void beginUpdate();
    void beginLicenseWorkflow();
    void notifyOperationFinished();
    void packageFutureFinished();
    void cancelPendingOperations();
    void switchView(View view);
    void runPendingCommand();

    AndroidConfig &m_androidConfig;
    AndroidSdkManager::CommandType m_pendingCommand = AndroidSdkManager::None;
    View m_currentView = PackageListing;
    AndroidSdkManager *m_sdkManager = nullptr;
    AndroidSdkModel *m_sdkModel = nullptr;
    Utils::OutputFormatter *m_formatter = nullptr;
    QFutureWatcher<AndroidSdkManager::OperationOutput> *m_currentOperation = nullptr;

    QStackedWidget *m_viewStack;
    QWidget *m_packagesStack;
    QWidget *m_outputStack;
    QProgressBar *m_operationProgress;
    QPlainTextEdit *m_outputEdit;
    QLabel *m_sdkLicenseLabel;
    QDialogButtonBox *m_sdkLicenseButtonBox;
    QDialogButtonBox *m_buttonBox;
};

} // Android::Internal
