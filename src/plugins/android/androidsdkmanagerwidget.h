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

namespace Utils { class OutputFormatter; }

namespace Android {
namespace Internal {

class AndroidSdkManager;
namespace Ui {
    class AndroidSdkManagerWidget;
}

class AndroidSdkModel;

class OptionsDialog : public QDialog
{
    Q_OBJECT

public:
    OptionsDialog(AndroidSdkManager *sdkManager, const QStringList &args,
                  QWidget *parent = nullptr);
    ~OptionsDialog();

    QStringList sdkManagerArguments() const;

private:
    QPlainTextEdit *argumentDetailsEdit;
    QLineEdit *argumentsEdit;
    QFuture<QString> m_optionsFuture;
};

class AndroidSdkManagerWidget : public QWidget
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
    ~AndroidSdkManagerWidget();

    void setSdkManagerControlsEnabled(bool enable);
    void installEssentials();

signals:
    void updatingSdk();
    void updatingSdkFinished();

private:
    void onApplyButton();
    void onUpdatePackages();
    void onCancel();
    void onNativeSdkManager();
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
    Ui::AndroidSdkManagerWidget *m_ui = nullptr;
    Utils::OutputFormatter *m_formatter = nullptr;
    QFutureWatcher<AndroidSdkManager::OperationOutput> *m_currentOperation = nullptr;
};

} // namespace Internal
} // namespace Android
