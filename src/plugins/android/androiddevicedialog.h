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

#pragma once

#include "androidconfigurations.h"

#include <QVector>
#include <QDialog>
#include <QFutureWatcher>
#include <QTime>

#include <memory>

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

namespace Utils { class ProgressIndicator; }

namespace Android {
namespace Internal {

class AndroidAvdManager;
class AndroidDeviceModel;
namespace Ui { class AndroidDeviceDialog; }

class AndroidDeviceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AndroidDeviceDialog(int apiLevel, const QString &abi,
                                 const QString &serialNumber, QWidget *parent = 0);
    ~AndroidDeviceDialog();

    AndroidDeviceInfo device();

    bool saveDeviceSelection() const;

private:
    void refreshDeviceList();
    void createAvd();
    void clickedOnView(const QModelIndex &idx);
    void showHelp();
    void avdAdded();
    void devicesRefreshed();
    void enableOkayButton();
    void defaultDeviceClear();

    AndroidDeviceModel *m_model;
    Ui::AndroidDeviceDialog *m_ui;
    Utils::ProgressIndicator *m_progressIndicator;
    int m_apiLevel;
    QString m_abi;
    QString m_avdNameFromAdd;
    QString m_defaultDevice;
    std::unique_ptr<AndroidAvdManager> m_avdManager;
    QVector<AndroidDeviceInfo> m_connectedDevices;
    QFutureWatcher<CreateAvdInfo> m_futureWatcherAddDevice;
    QFutureWatcher<AndroidDeviceInfoList> m_futureWatcherRefreshDevices;
};

}
}
