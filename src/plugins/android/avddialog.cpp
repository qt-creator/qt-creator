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
#include "androiddevice.h"
#include "androidconstants.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <utils/algorithm.h>
#include <utils/tooltip/tooltip.h>
#include <utils/utilsicons.h>
#include <utils/qtcassert.h>

#include <QFutureWatcher>
#include <QKeyEvent>
#include <QMessageBox>
#include <QToolTip>
#include <QLoggingCategory>
#include <QPushButton>

using namespace Android;
using namespace Android::Internal;

namespace {
static Q_LOGGING_CATEGORY(avdDialogLog, "qtc.android.avdDialog", QtWarningMsg)
}

AvdDialog::AvdDialog(const AndroidConfig &config, QWidget *parent)
    : QDialog(parent),
      m_allowedNameChars(QLatin1String("[a-z|A-Z|0-9|._-]*")),
      m_androidConfig(config),
      m_sdkManager(m_androidConfig)
{
    m_avdDialog.setupUi(this);
    m_hideTipTimer.setInterval(2000);
    m_hideTipTimer.setSingleShot(true);

    connect(&m_hideTipTimer, &QTimer::timeout, this, &Utils::ToolTip::hide);
    connect(m_avdDialog.deviceDefinitionTypeComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AvdDialog::updateDeviceDefinitionComboBox);
    connect(m_avdDialog.abiComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AvdDialog::updateApiLevelComboBox);

    deviceTypeToStringMap.insert(AvdDialog::Phone, "Phone");
    deviceTypeToStringMap.insert(AvdDialog::Tablet, "Tablet");
    deviceTypeToStringMap.insert(AvdDialog::Automotive, "Automotive");
    deviceTypeToStringMap.insert(AvdDialog::TV, "TV");
    deviceTypeToStringMap.insert(AvdDialog::Wear, "Wear");

    m_avdDialog.abiComboBox->addItems(QStringList({
                                           ProjectExplorer::Constants::ANDROID_ABI_X86,
                                           ProjectExplorer::Constants::ANDROID_ABI_X86_64,
                                           ProjectExplorer::Constants::ANDROID_ABI_ARMEABI_V7A,
                                           ProjectExplorer::Constants::ANDROID_ABI_ARM64_V8A}));

    auto v = new QRegularExpressionValidator(m_allowedNameChars, this);
    m_avdDialog.nameLineEdit->setValidator(v);
    m_avdDialog.nameLineEdit->installEventFilter(this);

    m_avdDialog.warningText->setType(Utils::InfoLabel::Warning);
    m_avdDialog.warningText->setElideMode(Qt::ElideNone);

    parseDeviceDefinitionsList();
    for (const QString &type : deviceTypeToStringMap)
        m_avdDialog.deviceDefinitionTypeComboBox->addItem(type);

    updateApiLevelComboBox();
}

int AvdDialog::exec()
{
    const int execResult = QDialog::exec();
    if (execResult == QDialog::Accepted) {
        CreateAvdInfo result;
        result.systemImage = systemImage();
        result.name = name();
        result.abi = abi();
        result.deviceDefinition = deviceDefinition();
        result.sdcardSize = sdcardSize();
        result.overwrite = m_avdDialog.overwriteCheckBox->isChecked();

        const AndroidAvdManager avdManager = AndroidAvdManager(m_androidConfig);
        QFutureWatcher<CreateAvdInfo> createAvdFutureWatcher;
        createAvdFutureWatcher.setFuture(avdManager.createAvd(result));

        QEventLoop loop;
        QObject::connect(&createAvdFutureWatcher, &QFutureWatcher<CreateAvdInfo>::finished,
                         &loop, &QEventLoop::quit);
        QObject::connect(&createAvdFutureWatcher, &QFutureWatcher<CreateAvdInfo>::canceled,
                         &loop, &QEventLoop::quit);
        loop.exec(QEventLoop::ExcludeUserInputEvents);

        const QFuture<CreateAvdInfo> future = createAvdFutureWatcher.future();
        if (future.isResultReadyAt(0))
            m_createdAvdInfo = future.result();
    }

    return execResult;
}

bool AvdDialog::isValid() const
{
    return !name().isEmpty() && systemImage() && systemImage()->isValid() && !abi().isEmpty();
}

ProjectExplorer::IDevice::Ptr AvdDialog::device() const
{
    if (!m_createdAvdInfo.systemImage) {
        qCWarning(avdDialogLog) << "System image of the created AVD is nullptr";
        return IDevice::Ptr();
    }
    AndroidDevice *dev = new AndroidDevice();
    const Utils::Id deviceId = AndroidDevice::idFromAvdInfo(m_createdAvdInfo);
    using namespace ProjectExplorer;
    dev->setupId(IDevice::AutoDetected, deviceId);
    dev->setMachineType(IDevice::Emulator);
    dev->setDisplayName(m_createdAvdInfo.name);
    dev->setDeviceState(IDevice::DeviceConnected);
    dev->setExtraData(Constants::AndroidAvdName, m_createdAvdInfo.name);
    dev->setExtraData(Constants::AndroidCpuAbi, {m_createdAvdInfo.abi});
    dev->setExtraData(Constants::AndroidSdk, m_createdAvdInfo.systemImage->apiLevel());
    return IDevice::Ptr(dev);
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

    const auto lines = output.split('\n');
    for (const QString &line : lines) {
        if (line.startsWith("---------") || line.isEmpty()) {
            DeviceDefinitionStruct deviceDefinition;
            for (const QString &line : avdDeviceInfo) {
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
    DeviceType curDeviceType = deviceTypeToStringMap.key(
        m_avdDialog.deviceDefinitionTypeComboBox->currentText());

    m_avdDialog.deviceDefinitionComboBox->clear();
    for (const DeviceDefinitionStruct &item : qAsConst(m_deviceDefinitionsList)) {
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
    SystemImageList installedSystemImages = m_sdkManager.installedSystemImages();
    DeviceType curDeviceType = deviceTypeToStringMap.key(
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
    for (SystemImage *image : qAsConst(filteredList)) {
            QString imageString = "android-" % QString::number(image->apiLevel());
            const QStringList imageSplits = image->sdkStylePath().split(';');
            if (imageSplits.size() == 4)
                imageString += QStringLiteral(" (%1)").arg(imageSplits.at(2));
            m_avdDialog.targetApiComboBox->addItem(imageString,
                                                   QVariant::fromValue<SystemImage *>(image));
            m_avdDialog.targetApiComboBox->setItemData(m_avdDialog.targetApiComboBox->count() - 1,
                                                       image->descriptionText(),
                                                       Qt::ToolTipRole);
    }

    if (installedSystemImages.isEmpty()) {
        m_avdDialog.targetApiComboBox->setEnabled(false);
        m_avdDialog.warningText->setVisible(true);
        m_avdDialog.warningText->setText(
            tr("Cannot create a new AVD. No suitable Android system image is installed.<br/>"
               "Install a system image for the intended Android version from the SDK Manager."));
        m_avdDialog.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    } else if (filteredList.isEmpty()) {
        m_avdDialog.targetApiComboBox->setEnabled(false);
        m_avdDialog.warningText->setVisible(true);
        m_avdDialog.warningText->setText(tr("Cannot create an AVD for ABI %1.<br/>Install a system "
                                            "image for it from the SDK Manager tab first.")
                                             .arg(abi()));
        m_avdDialog.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    } else {
        m_avdDialog.warningText->setVisible(false);
        m_avdDialog.targetApiComboBox->setEnabled(true);
        m_avdDialog.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
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
