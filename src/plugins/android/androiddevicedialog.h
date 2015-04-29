/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef ANDROIDDEVICEDIALOG_H
#define ANDROIDDEVICEDIALOG_H

#include "androidconfigurations.h"

#include <QVector>
#include <QDialog>
#include <QFutureWatcher>
#include <QTime>

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

namespace Utils { class ProgressIndicator; }

namespace Android {
namespace Internal {

class AndroidDeviceModel;
namespace Ui { class AndroidDeviceDialog; }

class AndroidDeviceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AndroidDeviceDialog(int apiLevel, const QString &abi, AndroidConfigurations::Options opts,
                                 const QString &serialNumber, QWidget *parent = 0);
    ~AndroidDeviceDialog();

    AndroidDeviceInfo device();
    void accept();

    bool saveDeviceSelection();

private slots:
    void refreshDeviceList();
    void createAvd();
    void clickedOnView(const QModelIndex &idx);
    void showHelp();
    void avdAdded();
private:
    static QVector<AndroidDeviceInfo> refreshDevices(const QString &adbToolPath,
                                                     const QString &androidToolPath,
                                                     const Utils::Environment &environment);
    void devicesRefreshed();
    void enableOkayButton();
    void useDefaultDevice();
    void defaultDeviceClear();

    AndroidDeviceModel *m_model;
    Ui::AndroidDeviceDialog *m_ui;
    Utils::ProgressIndicator *m_progressIndicator;
    int m_apiLevel;
    QString m_abi;
    QString m_avdNameFromAdd;
    QString m_defaultDevice;
    QTime m_defaultDeviceTimer;
    QFutureWatcher<AndroidConfig::CreateAvdInfo> m_futureWatcherAddDevice;
    QFutureWatcher<QVector<AndroidDeviceInfo>> m_futureWatcherRefreshDevices;
};

}
}

#endif // ANDROIDDEVICEDIALOG_H
