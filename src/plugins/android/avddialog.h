// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "androidconfigurations.h"

#include <QDialog>
#include <QRegularExpression>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QComboBox;
class QCheckBox;
class QDialogButtonBox;
class QLineEdit;
class QSpinBox;
QT_END_NAMESPACE

namespace Utils { class InfoLabel; }

namespace Android {
class AndroidConfig;
class SdkPlatform;

namespace Internal {

class AndroidSdkManager;

class AvdDialog : public QDialog
{
public:
    explicit AvdDialog(const AndroidConfig &config, QWidget *parent = nullptr);
    int exec() override;

    enum DeviceType { Phone, Tablet, Automotive, TV, Wear, PhoneOrTablet };

    ProjectExplorer::IDevice::Ptr device() const;

    const SystemImage *systemImage() const;
    QString name() const;
    QString abi() const;
    QString deviceDefinition() const;
    int sdcardSize() const;
    bool isValid() const;

private:
    void parseDeviceDefinitionsList();
    void updateDeviceDefinitionComboBox();
    void updateApiLevelComboBox();
    bool eventFilter(QObject *obj, QEvent *event) override;

    static AvdDialog::DeviceType tagToDeviceType(const QString &type_tag);

    struct DeviceDefinitionStruct
    {
        QString name_id;
        QString type_str;
        DeviceType deviceType;
    };

    CreateAvdInfo m_createdAvdInfo;
    QTimer m_hideTipTimer;
    QRegularExpression m_allowedNameChars;
    QList<DeviceDefinitionStruct> m_deviceDefinitionsList;
    const AndroidConfig &m_androidConfig;
    AndroidSdkManager m_sdkManager;
    QMap<AvdDialog::DeviceType, QString> m_deviceTypeToStringMap;

    QComboBox *m_abiComboBox;
    QSpinBox *m_sdcardSizeSpinBox;
    QLineEdit *m_nameLineEdit;
    QComboBox *m_targetApiComboBox;
    QComboBox *m_deviceDefinitionComboBox;
    Utils::InfoLabel *m_warningText;
    QComboBox *m_deviceDefinitionTypeComboBox;
    QCheckBox *m_overwriteCheckBox;
    QDialogButtonBox *m_buttonBox;
};

} // Internal
} // Android
