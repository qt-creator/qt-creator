// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "avddialog.h"
#include "androidtr.h"
#include "androidconfigurations.h"
#include "androiddevice.h"
#include "androidsdkmanager.h"

#include <coreplugin/icore.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/algorithm.h>
#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/tooltip/tooltip.h>
#include <utils/utilsicons.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QKeyEvent>
#include <QLineEdit>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QToolTip>

using namespace ProjectExplorer;
using namespace Utils;

namespace Android::Internal {

static Q_LOGGING_CATEGORY(avdDialogLog, "qtc.android.avdDialog", QtWarningMsg)

AvdDialog::AvdDialog(QWidget *parent)
    : QDialog(parent)
    , m_allowedNameChars(QLatin1String("[a-z|A-Z|0-9|._-]*"))
{
    resize(800, 0);
    setWindowTitle(Tr::tr("Create new AVD"));

    m_abiComboBox = new QComboBox;
    m_abiComboBox->addItems({
        ProjectExplorer::Constants::ANDROID_ABI_X86,
        ProjectExplorer::Constants::ANDROID_ABI_X86_64,
        ProjectExplorer::Constants::ANDROID_ABI_ARMEABI_V7A,
        ProjectExplorer::Constants::ANDROID_ABI_ARM64_V8A
    });

    m_sdcardSizeSpinBox = new QSpinBox;
    m_sdcardSizeSpinBox->setSuffix(Tr::tr(" MiB"));
    m_sdcardSizeSpinBox->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    m_sdcardSizeSpinBox->setRange(0, 1000000);
    m_sdcardSizeSpinBox->setValue(512);

    m_nameLineEdit = new QLineEdit;
    m_nameLineEdit->setValidator(new QRegularExpressionValidator(m_allowedNameChars, this));
    m_nameLineEdit->installEventFilter(this);

    m_targetApiComboBox = new QComboBox;

    m_deviceDefinitionComboBox = new QComboBox;

    m_warningText = new InfoLabel;
    m_warningText->setType(InfoLabel::Warning);
    m_warningText->setElideMode(Qt::ElideNone);

    m_deviceDefinitionTypeComboBox = new QComboBox;

    m_overwriteCheckBox = new QCheckBox(Tr::tr("Overwrite existing AVD name"));

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    m_hideTipTimer.setInterval(2000);
    m_hideTipTimer.setSingleShot(true);

    using namespace Layouting;

    Column {
        Form {
            Tr::tr("Name:"), m_nameLineEdit, br,
            Tr::tr("Device definition:"),
                Row { m_deviceDefinitionTypeComboBox, m_deviceDefinitionComboBox }, br,
            Tr::tr("Architecture (ABI):"), m_abiComboBox, br,
            Tr::tr("Target API:"), m_targetApiComboBox, br,
            QString(), m_warningText, br,
            Tr::tr("SD card size:"), m_sdcardSizeSpinBox, br,
            QString(), m_overwriteCheckBox,
        },
        st,
        m_buttonBox
    }.attachTo(this);

    connect(&m_hideTipTimer, &QTimer::timeout, this, &Utils::ToolTip::hide);
    connect(m_deviceDefinitionTypeComboBox, &QComboBox::currentIndexChanged,
            this, &AvdDialog::updateDeviceDefinitionComboBox);
    connect(m_abiComboBox, &QComboBox::currentIndexChanged,
            this, &AvdDialog::updateApiLevelComboBox);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    m_deviceTypeToStringMap.insert(AvdDialog::Phone, "Phone");
    m_deviceTypeToStringMap.insert(AvdDialog::Tablet, "Tablet");
    m_deviceTypeToStringMap.insert(AvdDialog::Automotive, "Automotive");
    m_deviceTypeToStringMap.insert(AvdDialog::TV, "TV");
    m_deviceTypeToStringMap.insert(AvdDialog::Wear, "Wear");
    m_deviceTypeToStringMap.insert(AvdDialog::Desktop, "Desktop");

    parseDeviceDefinitionsList();
    for (const QString &type : m_deviceTypeToStringMap)
        m_deviceDefinitionTypeComboBox->addItem(type);

    updateApiLevelComboBox();
}

int AvdDialog::exec()
{
    const int execResult = QDialog::exec();
    if (execResult == QDialog::Accepted) {
        const SystemImage *si = systemImage();
        if (!si || !si->isValid() || name().isEmpty()) {
            QMessageBox::warning(Core::ICore::dialogParent(),
                Tr::tr("Create new AVD"), Tr::tr("Cannot create AVD. Invalid input."));
            return QDialog::Rejected;
        }

        const CreateAvdInfo avdInfo{si->sdkStylePath(), si->apiLevel(), name(), abi(),
                                    deviceDefinition(), sdcardSize()};
        const auto result = AndroidDeviceManager::instance()->createAvd(
            avdInfo, m_overwriteCheckBox->isChecked());
        if (!result) {
            QMessageBox::warning(Core::ICore::dialogParent(), Tr::tr("Create new AVD"),
                                 result.error());
            return QDialog::Rejected;
        }
        m_createdAvdInfo = avdInfo;
        AndroidDeviceManager::instance()->updateAvdList();
    }
    return execResult;
}

bool AvdDialog::isValid() const
{
    return !name().isEmpty() && systemImage() && systemImage()->isValid() && !abi().isEmpty();
}

CreateAvdInfo AvdDialog::avdInfo() const
{
    return m_createdAvdInfo;
}

AvdDialog::DeviceType AvdDialog::tagToDeviceType(const QString &type_tag)
{
    if (type_tag.contains("android-wear"))
        return AvdDialog::Wear;
    else if (type_tag.contains("android-tv"))
        return AvdDialog::TV;
    else if (type_tag.contains("android-automotive"))
        return AvdDialog::Automotive;
    else if (type_tag.contains("android-desktop"))
        return AvdDialog::Desktop;
    return AvdDialog::PhoneOrTablet;
}

static bool avdManagerCommand(const QStringList &args, QString *output)
{
    CommandLine cmd(AndroidConfig::avdManagerToolPath(), args);
    Process proc;
    proc.setEnvironment(AndroidConfig::toolsEnvironment());
    qCDebug(avdDialogLog).noquote() << "Running AVD Manager command:" << cmd.toUserOutput();
    proc.setCommand(cmd);
    proc.runBlocking();
    if (proc.result() == ProcessResult::FinishedWithSuccess) {
        if (output)
            *output = proc.allOutput();
        return true;
    }
    return false;
}

void AvdDialog::parseDeviceDefinitionsList()
{
    QString output;

    if (!avdManagerCommand({"list", "device"}, &output)) {
        qCDebug(avdDialogLog) << "Avd list command failed" << output
                              << AndroidConfig::sdkToolsVersion();
        return;
    }

    /* Example output:
Available devices definitions:
id: 0 or "automotive_1024p_landscape"
    Name: Automotive (1024p landscape)
    OEM : Google
    Tag : android-automotive-playstore
---------
id: 1 or "automotive_1080p_landscape"
    Name: Automotive (1080p landscape)
    OEM : Google
    Tag : android-automotive
---------
id: 2 or "Galaxy Nexus"
    Name: Galaxy Nexus
    OEM : Google
---------
id: 3 or "desktop_large"
    Name: Large Desktop
    OEM : Google
    Tag : android-desktop
...
     */

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
    DeviceType curDeviceType = m_deviceTypeToStringMap.key(
        m_deviceDefinitionTypeComboBox->currentText());

    m_deviceDefinitionComboBox->clear();
    for (const DeviceDefinitionStruct &item : std::as_const(m_deviceDefinitionsList)) {
        if (item.deviceType == curDeviceType)
            m_deviceDefinitionComboBox->addItem(item.name_id);
    }
    m_deviceDefinitionComboBox->addItem("Custom");

    updateApiLevelComboBox();
}

const SystemImage* AvdDialog::systemImage() const
{
    return m_targetApiComboBox->currentData().value<SystemImage*>();
}

QString AvdDialog::name() const
{
    return m_nameLineEdit->text();
}

QString AvdDialog::abi() const
{
    return m_abiComboBox->currentText();
}

QString AvdDialog::deviceDefinition() const
{
    return m_deviceDefinitionComboBox->currentText();
}

int AvdDialog::sdcardSize() const
{
    return m_sdcardSizeSpinBox->value();
}

void AvdDialog::updateApiLevelComboBox()
{
    SystemImageList installedSystemImages = m_sdkManager.installedSystemImages();
    DeviceType curDeviceType = m_deviceTypeToStringMap.key(
        m_deviceDefinitionTypeComboBox->currentText());

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

    m_targetApiComboBox->clear();
    for (SystemImage *image : std::as_const(filteredList)) {
            QString imageString = "android-" % QString::number(image->apiLevel());
            const QStringList imageSplits = image->sdkStylePath().split(';');
            if (imageSplits.size() == 4)
                imageString += QStringLiteral(" (%1)").arg(imageSplits.at(2));
            m_targetApiComboBox->addItem(imageString,
                                                   QVariant::fromValue<SystemImage *>(image));
            m_targetApiComboBox->setItemData(m_targetApiComboBox->count() - 1,
                                                       image->descriptionText(),
                                                       Qt::ToolTipRole);
    }

    if (installedSystemImages.isEmpty()) {
        m_targetApiComboBox->setEnabled(false);
        m_warningText->setVisible(true);
        m_warningText->setText(
            Tr::tr("Cannot create a new AVD. No suitable Android system image is installed.<br/>"
                   "Install a system image for the intended Android version from the SDK Manager."));
        m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    } else if (filteredList.isEmpty()) {
        m_targetApiComboBox->setEnabled(false);
        m_warningText->setVisible(true);
        m_warningText->setText(Tr::tr("Cannot create an AVD for ABI %1.<br/>Install a system "
                                            "image for it from the SDK Manager tab first.")
                                             .arg(abi()));
        m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    } else {
        m_warningText->setVisible(false);
        m_targetApiComboBox->setEnabled(true);
        m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
}

bool AvdDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_nameLineEdit && event->type() == QEvent::KeyPress) {
        auto ke = static_cast<QKeyEvent *>(event);
        const QString key = ke->text();
        if (!key.isEmpty() && !m_allowedNameChars.match(key).hasMatch()) {
            QPoint position = m_nameLineEdit->parentWidget()->mapToGlobal(m_nameLineEdit->geometry().bottomLeft());
            position -= Utils::ToolTip::offsetFromPosition();
            Utils::ToolTip::show(position, Tr::tr("Allowed characters are: a-z A-Z 0-9 and . _ -"), m_nameLineEdit);
            m_hideTipTimer.start();
        } else {
            m_hideTipTimer.stop();
            Utils::ToolTip::hide();
        }
    }
    return QDialog::eventFilter(obj, event);
}

} // Android::Internal
