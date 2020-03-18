/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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
#include "ui_addnewavddialog.h"
#include "androidconfigurations.h"

#include <QDialog>
#include <QTimer>

namespace Android {
class AndroidConfig;
class SdkPlatform;

namespace Internal {
class AndroidSdkManager;
class AvdDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AvdDialog(int minApiLevel,
                       AndroidSdkManager *sdkManager,
                       const QStringList &abis,
                       const AndroidConfig &config,
                       QWidget *parent = nullptr);

    enum DeviceType { TV, Phone, Wear, Tablet, Automotive, PhoneOrTablet };

    const QMap<DeviceType, QString> DeviceTypeToStringMap{
        {TV,            "TV"},
        {Phone,         "Phone"},
        {Wear,          "Wear"},
        {Tablet,        "Tablet"},
        {Automotive,    "Automotive"}
    };

    struct DeviceDefinitionStruct
    {
        QString name_id;
        QString type_str;
        DeviceType deviceType;
    };

    const SystemImage *systemImage() const;
    QString name() const;
    QString abi() const;
    QString deviceDefinition() const;
    int sdcardSize() const;
    bool isValid() const;
    static AvdDialog::DeviceType tagToDeviceType(const QString &type_tag);
    static CreateAvdInfo gatherCreateAVDInfo(QWidget *parent, AndroidSdkManager *sdkManager,
                                             const AndroidConfig &config,
                                             int minApiLevel = 0, const QStringList &abis = {});

private:
    void parseDeviceDefinitionsList();
    void updateDeviceDefinitionComboBox();
    void updateApiLevelComboBox();
    bool eventFilter(QObject *obj, QEvent *event) override;

    Ui::AddNewAVDDialog m_avdDialog;
    AndroidSdkManager *m_sdkManager;
    int m_minApiLevel;
    QTimer m_hideTipTimer;
    QRegularExpression m_allowedNameChars;
    QList<DeviceDefinitionStruct> m_deviceDefinitionsList;
    AndroidConfig m_androidConfig;
};
}
}
