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

#include "avddialog.h"
#include "androidsdkmanager.h"
#include "androidavdmanager.h"

#include <utils/algorithm.h>
#include <utils/tooltip/tooltip.h>
#include <utils/utilsicons.h>
#include <utils/qtcassert.h>

#include <QKeyEvent>
#include <QMessageBox>
#include <QToolTip>
#include <QLoggingCategory>

using namespace Android;
using namespace Android::Internal;

namespace {
static Q_LOGGING_CATEGORY(avdDialogLog, "qtc.android.avdDialog", QtWarningMsg)
}

AvdDialog::AvdDialog(int minApiLevel, AndroidSdkManager *sdkManager, const QStringList &abis,
                     const AndroidConfig &config, QWidget *parent) :
    QDialog(parent),
    m_sdkManager(sdkManager),
    m_minApiLevel(minApiLevel),
    m_allowedNameChars(QLatin1String("[a-z|A-Z|0-9|._-]*")),
    m_androidConfig(config)
{
    QTC_CHECK(m_sdkManager);
    m_avdDialog.setupUi(this);
    m_hideTipTimer.setInterval(2000);
    m_hideTipTimer.setSingleShot(true);

    if (abis.isEmpty()) {
        m_avdDialog.abiComboBox->addItems(QStringList({"x86", "x86_64", "armeabi-v7a",
                                                       "armeabi", "arm64-v8a"}));
    } else {
        m_avdDialog.abiComboBox->addItems(abis);
    }

    auto v = new QRegularExpressionValidator(m_allowedNameChars, this);
    m_avdDialog.nameLineEdit->setValidator(v);
    m_avdDialog.nameLineEdit->installEventFilter(this);

    m_avdDialog.warningText->setType(Utils::InfoLabel::Warning);

    connect(&m_hideTipTimer, &QTimer::timeout, this, []() { Utils::ToolTip::hide(); });

    parseDeviceDefinitionsList();
    for (const QString &type : DeviceTypeToStringMap.values())
        m_avdDialog.deviceDefinitionTypeComboBox->addItem(type);

    connect(m_avdDialog.deviceDefinitionTypeComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &AvdDialog::updateDeviceDefinitionComboBox);
    connect(m_avdDialog.abiComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AvdDialog::updateApiLevelComboBox);

    m_avdDialog.deviceDefinitionTypeComboBox->setCurrentIndex(1); // Set Phone type as default index
    updateApiLevelComboBox();
}

bool AvdDialog::isValid() const
{
    return !name().isEmpty() && systemImage() && systemImage()->isValid() && !abi().isEmpty();
}

CreateAvdInfo AvdDialog::gatherCreateAVDInfo(QWidget *parent, AndroidSdkManager *sdkManager,
                                             const AndroidConfig &config, int minApiLevel, const QStringList &abis)
{
    CreateAvdInfo result;
    AvdDialog d(minApiLevel, sdkManager, abis, config, parent);
    if (d.exec() != QDialog::Accepted || !d.isValid())
        return result;

    result.systemImage = d.systemImage();
    result.name = d.name();
    result.abi = d.abi();
    result.deviceDefinition = d.deviceDefinition();
    result.sdcardSize = d.sdcardSize();
    result.overwrite = d.m_avdDialog.overwriteCheckBox->isChecked();

    return result;
}

AvdDialog::DeviceType AvdDialog::tagToDeviceType(const QString &type_tag)
{
    if (type_tag.contains("android-wear"))
        return AvdDialog::Wear;
    else if (type_tag.contains("android-tv"))
        return AvdDialog::TV;
    else if (type_tag.contains("android-automotive"))
        return AvdDialog::Automotive;
    else
        return AvdDialog::PhoneOrTablet;
}

void AvdDialog::parseDeviceDefinitionsList()
{
    QString output;

    if (!AndroidAvdManager::avdManagerCommand(m_androidConfig, {"list", "device"}, &output)) {
        qCDebug(avdDialogLog) << "Avd list command failed" << output
                              << m_androidConfig.sdkToolsVersion();
        return;
    }

    QStringList avdDeviceInfo;

    for (const QString &line : output.split('\n')) {
        if (line.startsWith("---------") || line.isEmpty()) {
            DeviceDefinitionStruct deviceDefinition;
            for (const QString &line : avdDeviceInfo) {
                QString value;
                if (line.contains("id:")) {
                    deviceDefinition.name_id = line.split("or").at(1);
                    deviceDefinition.name_id = deviceDefinition.name_id.remove(0, 1).remove('"');
                } else if (line.contains("Tag :")) {
                    deviceDefinition.type_str = line.split(':').at(1);
                    deviceDefinition.type_str = deviceDefinition.type_str.remove(0, 1);
                }
            }

            DeviceType deviceType = tagToDeviceType(deviceDefinition.type_str);
            if (deviceType == PhoneOrTablet) {
                if (deviceDefinition.name_id.contains("Tablet"))
                    deviceType = Tablet;
                else
                    deviceType = Phone;
            }
            deviceDefinition.deviceType = deviceType;
            m_deviceDefinitionsList.append(deviceDefinition);
            avdDeviceInfo.clear();
        } else {
            avdDeviceInfo << line;
        }
    }
}

void AvdDialog::updateDeviceDefinitionComboBox()
{
    DeviceType curDeviceType = DeviceTypeToStringMap.key(
        m_avdDialog.deviceDefinitionTypeComboBox->currentText());

    m_avdDialog.deviceDefinitionComboBox->clear();
    for (const DeviceDefinitionStruct &item : m_deviceDefinitionsList) {
        if (item.deviceType == curDeviceType)
            m_avdDialog.deviceDefinitionComboBox->addItem(item.name_id);
    }
    m_avdDialog.deviceDefinitionComboBox->addItem("Custom");

    updateApiLevelComboBox();
}

const SystemImage* AvdDialog::systemImage() const
{
    return m_avdDialog.targetApiComboBox->currentData().value<SystemImage*>();
}

QString AvdDialog::name() const
{
    return m_avdDialog.nameLineEdit->text();
}

QString AvdDialog::abi() const
{
    return m_avdDialog.abiComboBox->currentText();
}

QString AvdDialog::deviceDefinition() const
{
    return m_avdDialog.deviceDefinitionComboBox->currentText();
}

int AvdDialog::sdcardSize() const
{
    return m_avdDialog.sdcardSizeSpinBox->value();
}

void AvdDialog::updateApiLevelComboBox()
{
    SystemImageList installedSystemImages = m_sdkManager->installedSystemImages();
    DeviceType curDeviceType = DeviceTypeToStringMap.key(
        m_avdDialog.deviceDefinitionTypeComboBox->currentText());

    QString selectedAbi = abi();
    auto hasAbi = [selectedAbi](const SystemImage *image) {
        return image && image->isValid() && (image->abiName() == selectedAbi);
    };

    SystemImageList filteredList;
    filteredList = Utils::filtered(installedSystemImages, [hasAbi, &curDeviceType](const SystemImage *image) {
        DeviceType deviceType = tagToDeviceType(image->sdkStylePath().split(';').at(2));
        if (deviceType == PhoneOrTablet && (curDeviceType == Phone || curDeviceType == Tablet))
            curDeviceType = PhoneOrTablet;
        return image && deviceType == curDeviceType && hasAbi(image);
    });

    m_avdDialog.targetApiComboBox->clear();
    for (SystemImage *image : filteredList) {
            QString imageString = "android-" % QString::number(image->apiLevel());
            if (image->sdkStylePath().contains("playstore"))
                imageString += " (Google PlayStore)";
            m_avdDialog.targetApiComboBox->addItem(imageString,
                                                   QVariant::fromValue<SystemImage *>(image));
            m_avdDialog.targetApiComboBox->setItemData(m_avdDialog.targetApiComboBox->count() - 1,
                                                       image->descriptionText(),
                                                       Qt::ToolTipRole);
    }

    if (installedSystemImages.isEmpty()) {
        m_avdDialog.targetApiComboBox->setEnabled(false);
        m_avdDialog.warningText->setVisible(true);
        m_avdDialog.warningText->setText(tr("Cannot create a new AVD. No sufficiently recent Android SDK available.\n"
                                            "Install an SDK of at least API version %1.")
                                         .arg(m_minApiLevel));
    } else if (filteredList.isEmpty()) {
        m_avdDialog.targetApiComboBox->setEnabled(false);
        m_avdDialog.warningText->setVisible(true);
        m_avdDialog.warningText->setText(tr("Cannot create a AVD for ABI %1. Install an image for it.")
                                         .arg(abi()));
    } else {
        m_avdDialog.warningText->setVisible(false);
        m_avdDialog.targetApiComboBox->setEnabled(true);
    }
}

bool AvdDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_avdDialog.nameLineEdit && event->type() == QEvent::KeyPress) {
        auto ke = static_cast<QKeyEvent *>(event);
        const QString key = ke->text();
        if (!key.isEmpty() && !m_allowedNameChars.match(key).hasMatch()) {
            QPoint position = m_avdDialog.nameLineEdit->parentWidget()->mapToGlobal(m_avdDialog.nameLineEdit->geometry().bottomLeft());
            position -= Utils::ToolTip::offsetFromPosition();
            Utils::ToolTip::show(position, tr("Allowed characters are: a-z A-Z 0-9 and . _ -"), m_avdDialog.nameLineEdit);
            m_hideTipTimer.start();
        } else {
            m_hideTipTimer.stop();
            Utils::ToolTip::hide();
        }
    }
    return QDialog::eventFilter(obj, event);
}
