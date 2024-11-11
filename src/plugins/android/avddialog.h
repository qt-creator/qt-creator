// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "androidconfigurations.h"

#include <solutions/tasking/tasktreerunner.h>

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
class SdkPlatform;

namespace Internal {

class AvdDialog : public QDialog
{
public:
    explicit AvdDialog(QWidget *parent = nullptr);
    CreateAvdInfo avdInfo() const;

private:
    enum DeviceType { Phone, Tablet, Automotive, TV, Wear, Desktop, PhoneOrTablet };

    const SystemImage *systemImage() const;
    QString name() const;
    QString abi() const;
    QString deviceDefinition() const;
    int sdcardSize() const;
    bool isValid() const;

    void updateDeviceDefinitionComboBox();
    void updateApiLevelComboBox();
    bool eventFilter(QObject *obj, QEvent *event) override;

    static AvdDialog::DeviceType tagToDeviceType(const QString &type_tag);

    void collectInitialData();
    void createAvd();

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
    QMap<AvdDialog::DeviceType, QString> m_deviceTypeToStringMap;

    QWidget *m_gui;
    QComboBox *m_abiComboBox;
    QSpinBox *m_sdcardSizeSpinBox;
    QLineEdit *m_nameLineEdit;
    QComboBox *m_targetApiComboBox;
    QComboBox *m_deviceDefinitionComboBox;
    Utils::InfoLabel *m_warningText;
    QComboBox *m_deviceDefinitionTypeComboBox;
    QCheckBox *m_overwriteCheckBox;
    QDialogButtonBox *m_buttonBox;
    Tasking::TaskTreeRunner m_taskTreeRunner;
};

} // Internal
} // Android
