/**************************************************************************
**
** Copyright (c) 2013 BogDan Vatra <bog_dan_ro@yahoo.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "androidsettingswidget.h"

#include "ui_androidsettingswidget.h"

#include "androidconfigurations.h"

#include "androidconstants.h"

#include <utils/hostosinfo.h>

#include <QFile>
#include <QTextStream>
#include <QProcess>

#include <QFileDialog>
#include <QMessageBox>
#include <QModelIndex>

namespace Android {
namespace Internal {

void AvdModel::setAvdList(const QVector<AndroidDeviceInfo> &list)
{
    beginResetModel();
    m_list = list;
    endResetModel();
}

QString AvdModel::avdName(const QModelIndex &index)
{
    return m_list[index.row()].serialNumber;
}

QVariant AvdModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole || !index.isValid())
        return QVariant();
    switch (index.column()) {
        case 0:
            return m_list[index.row()].serialNumber;
        case 1:
            return QString::fromLatin1("API %1").arg(m_list[index.row()].sdk);
        case 2:
            return m_list[index.row()].cpuABI;
    }
    return QVariant();
}

QVariant AvdModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case 0:
                //: AVD - Android Virtual Device
                return tr("AVD Name");
            case 1:
                return tr("AVD Target");
            case 2:
                return tr("CPU/ABI");
        }
    }
    return QAbstractItemModel::headerData(section, orientation, role );
}

int AvdModel::rowCount(const QModelIndex &/*parent*/) const
{
    return m_list.size();
}

int AvdModel::columnCount(const QModelIndex &/*parent*/) const
{
    return 3;
}


AndroidSettingsWidget::AndroidSettingsWidget(QWidget *parent)
    : QWidget(parent),
      m_ui(new Ui_AndroidSettingsWidget),
      m_androidConfig(AndroidConfigurations::instance().config()),
      m_saveSettingsRequested(false)
{
    initGui();
}

AndroidSettingsWidget::~AndroidSettingsWidget()
{
    if (m_saveSettingsRequested)
        AndroidConfigurations::instance().setConfig(m_androidConfig);
    delete m_ui;
}

QString AndroidSettingsWidget::searchKeywords() const
{
    QString rc;
    QTextStream(&rc) << m_ui->SDKLocationLabel->text()
        << ' ' << m_ui->SDKLocationLineEdit->text()
        << ' ' << m_ui->NDKLocationLabel->text()
        << ' ' << m_ui->NDKLocationLineEdit->text()
        << ' ' << m_ui->NDKToolchainVersionLabel->text()
        << ' ' << m_ui->AntLocationLabel->text()
        << ' ' << m_ui->AntLocationLineEdit->text()
        << ' ' << m_ui->GdbLocationLabel->text()
        << ' ' << m_ui->GdbLocationLineEdit->text()
        << ' ' << m_ui->GdbserverLocationLabel->text()
        << ' ' << m_ui->GdbserverLocationLineEdit->text()
        << ' ' << m_ui->GdbLocationLabelx86->text()
        << ' ' << m_ui->GdbLocationLineEditx86->text()
        << ' ' << m_ui->GdbserverLocationLabelx86->text()
        << ' ' << m_ui->GdbserverLocationLineEditx86->text()
        << ' ' << m_ui->OpenJDKLocationLabel->text()
        << ' ' << m_ui->OpenJDKLocationLineEdit->text()
        << ' ' << m_ui->AVDManagerLabel->text()
        << ' ' << m_ui->DataPartitionSizeLable->text()
        << ' ' << m_ui->DataPartitionSizeSpinBox->text();
    rc.remove(QLatin1Char('&'));
    return rc;
}

void AndroidSettingsWidget::initGui()
{
    m_ui->setupUi(this);
    if (checkSDK(m_androidConfig.sdkLocation))
        m_ui->SDKLocationLineEdit->setText(m_androidConfig.sdkLocation.toUserOutput());
    else
        m_androidConfig.sdkLocation.clear();
    if (checkNDK(m_androidConfig.ndkLocation))
        m_ui->NDKLocationLineEdit->setText(m_androidConfig.ndkLocation.toUserOutput());
    else
        m_androidConfig.ndkLocation.clear();
    m_ui->AntLocationLineEdit->setText(m_androidConfig.antLocation.toUserOutput());
    m_ui->GdbLocationLineEdit->setText(m_androidConfig.armGdbLocation.toUserOutput());
    m_ui->GdbserverLocationLineEdit->setText(m_androidConfig.armGdbserverLocation.toUserOutput());
    m_ui->GdbLocationLineEditx86->setText(m_androidConfig.x86GdbLocation.toUserOutput());
    m_ui->GdbserverLocationLineEditx86->setText(m_androidConfig.x86GdbserverLocation.toUserOutput());
    m_ui->OpenJDKLocationLineEdit->setText(m_androidConfig.openJDKLocation.toUserOutput());
    m_ui->DataPartitionSizeSpinBox->setValue(m_androidConfig.partitionSize);
    m_ui->AVDTableView->setModel(&m_AVDModel);
    m_AVDModel.setAvdList(AndroidConfigurations::instance().androidVirtualDevices());
    m_ui->AVDTableView->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    m_ui->AVDTableView->horizontalHeader()->setResizeMode(1, QHeaderView::ResizeToContents);
    fillToolchainVersions();
}

void AndroidSettingsWidget::saveSettings(bool saveNow)
{
    // We must defer this step because of a stupid bug on MacOS. See QTCREATORBUG-1675.
    if (saveNow) {
        AndroidConfigurations::instance().setConfig(m_androidConfig);
        m_saveSettingsRequested = false;
    } else {
        m_saveSettingsRequested = true;
    }
}


bool AndroidSettingsWidget::checkSDK(const Utils::FileName &location)
{
    if (location.isEmpty())
        return false;
    Utils::FileName adb = location;
    Utils::FileName androidExe = location;
    Utils::FileName androidBat = location;
    Utils::FileName emulator = location;
    if (!adb.appendPath(QLatin1String("platform-tools/adb" QTC_HOST_EXE_SUFFIX)).toFileInfo().exists()
            || (!androidExe.appendPath(QLatin1String("/tools/android" QTC_HOST_EXE_SUFFIX)).toFileInfo().exists()
                && !androidBat.appendPath(QLatin1String("/tools/android" ANDROID_BAT_SUFFIX)).toFileInfo().exists())
            || !emulator.appendPath(QLatin1String("/tools/emulator" QTC_HOST_EXE_SUFFIX)).toFileInfo().exists()) {
        QMessageBox::critical(this, tr("Android SDK Folder"), tr("\"%1\" does not seem to be an Android SDK top folder.").arg(location.toUserOutput()));
        return false;
    }
    return true;
}

bool AndroidSettingsWidget::checkNDK(const Utils::FileName &location)
{
    m_ui->toolchainVersionComboBox->setEnabled(false);
    m_ui->GdbLocationLineEdit->setEnabled(false);
    m_ui->GdbLocationPushButton->setEnabled(false);
    m_ui->GdbserverLocationLineEdit->setEnabled(false);
    m_ui->GdbserverLocationPushButton->setEnabled(false);
    if (location.isEmpty())
        return false;
    Utils::FileName platformPath = location;
    Utils::FileName toolChainPath = location;
    Utils::FileName sourcesPath = location;
    if (!platformPath.appendPath(QLatin1String("platforms")).toFileInfo().exists()
            || !toolChainPath.appendPath(QLatin1String("toolchains")).toFileInfo().exists()
            || !sourcesPath.appendPath(QLatin1String("sources/cxx-stl")).toFileInfo().exists()) {
        QMessageBox::critical(this, tr("Android NDK Folder"), tr("\"%1\" does not seem to be an Android NDK top folder.").arg(location.toUserOutput()));
        return false;
    }
    m_androidConfig.ndkLocation = location;
    m_ui->toolchainVersionComboBox->setEnabled(true);
    m_ui->GdbLocationLineEdit->setEnabled(true);
    m_ui->GdbLocationPushButton->setEnabled(true);
    m_ui->GdbserverLocationLineEdit->setEnabled(true);
    m_ui->GdbserverLocationPushButton->setEnabled(true);
    return true;

}

void AndroidSettingsWidget::sdkLocationEditingFinished()
{
    Utils::FileName location = Utils::FileName::fromUserInput(m_ui->SDKLocationLineEdit->text());
    if (!checkSDK(location)) {
        m_ui->AVDManagerFrame->setEnabled(false);
        return;
    }
    m_androidConfig.sdkLocation = location;
    saveSettings(true);
    m_AVDModel.setAvdList(AndroidConfigurations::instance().androidVirtualDevices());
    m_ui->AVDManagerFrame->setEnabled(true);
}

void AndroidSettingsWidget::ndkLocationEditingFinished()
{
    Utils::FileName location = Utils::FileName::fromUserInput(m_ui->NDKLocationLineEdit->text());
    if (!checkNDK(location))
        return;
    saveSettings(true);
    fillToolchainVersions();
}

void AndroidSettingsWidget::fillToolchainVersions()
{
    QStringList toolchainVersions = AndroidConfigurations::instance().ndkToolchainVersions();
    QString toolchain = m_androidConfig.ndkToolchainVersion;
    m_ui->toolchainVersionComboBox->clear();
    foreach (const QString &item, toolchainVersions)
        m_ui->toolchainVersionComboBox->addItem(item);
    if (!toolchain.isEmpty())
        m_ui->toolchainVersionComboBox->setCurrentIndex(toolchainVersions.indexOf(toolchain));
    else
        m_ui->toolchainVersionComboBox->setCurrentIndex(0);
}

void AndroidSettingsWidget::toolchainVersionIndexChanged(QString version)
{
    m_androidConfig.ndkToolchainVersion = version;
    saveSettings(true);
}


void AndroidSettingsWidget::antLocationEditingFinished()
{
    Utils::FileName location = Utils::FileName::fromUserInput(m_ui->AntLocationLineEdit->text());
    if (location.isEmpty() || !location.toFileInfo().exists())
        return;
    m_androidConfig.antLocation = location;
}

void AndroidSettingsWidget::gdbLocationEditingFinished()
{
    Utils::FileName location = Utils::FileName::fromUserInput(m_ui->GdbLocationLineEdit->text());
    if (location.isEmpty() || !location.toFileInfo().exists())
        return;
    m_androidConfig.armGdbLocation = location;
}

void AndroidSettingsWidget::gdbserverLocationEditingFinished()
{
    Utils::FileName location = Utils::FileName::fromUserInput(m_ui->GdbserverLocationLineEdit->text());
    if (location.isEmpty() || !location.toFileInfo().exists())
        return;
    m_androidConfig.armGdbserverLocation = location;
}

void AndroidSettingsWidget::gdbLocationX86EditingFinished()
{
    Utils::FileName location = Utils::FileName::fromUserInput(m_ui->GdbLocationLineEditx86->text());
    if (location.isEmpty() || !location.toFileInfo().exists())
        return;
    m_androidConfig.x86GdbLocation = location;
}

void AndroidSettingsWidget::gdbserverLocationX86EditingFinished()
{
    Utils::FileName location = Utils::FileName::fromUserInput(m_ui->GdbserverLocationLineEditx86->text());
    if (location.isEmpty() || !location.toFileInfo().exists())
        return;
    m_androidConfig.x86GdbserverLocation = location;
}

void AndroidSettingsWidget::openJDKLocationEditingFinished()
{
    Utils::FileName location = Utils::FileName::fromUserInput(m_ui->OpenJDKLocationLineEdit->text());
    if (location.isEmpty() || !location.toFileInfo().exists())
        return;
    m_androidConfig.openJDKLocation = location;
}

void AndroidSettingsWidget::browseSDKLocation()
{
    Utils::FileName dir = Utils::FileName::fromString(QFileDialog::getExistingDirectory(this, tr("Select Android SDK folder")));
    if (!checkSDK(dir))
        return;
    m_ui->SDKLocationLineEdit->setText(dir.toUserOutput());
    sdkLocationEditingFinished();
}

void AndroidSettingsWidget::browseNDKLocation()
{
    Utils::FileName dir = Utils::FileName::fromString(QFileDialog::getExistingDirectory(this, tr("Select Android NDK folder")));
    if (!checkNDK(dir))
        return;
    m_ui->NDKLocationLineEdit->setText(dir.toUserOutput());
    ndkLocationEditingFinished();
}

void AndroidSettingsWidget::browseAntLocation()
{
    QString dir;
    QString antApp;
    if (Utils::HostOsInfo::isWindowsHost()) {
        dir = QDir::homePath();
        antApp = QLatin1String("ant.bat");
    } else {
        dir = QLatin1String("/usr/bin/ant");
        antApp = QLatin1String("ant");
    }
    const QString file =
        QFileDialog::getOpenFileName(this, tr("Select ant Script"), dir, antApp);
    if (!file.length())
        return;
    m_ui->AntLocationLineEdit->setText(file);
    antLocationEditingFinished();
}

void AndroidSettingsWidget::browseGdbLocation()
{
    Utils::FileName gdbPath = AndroidConfigurations::instance().gdbPath(ProjectExplorer::Abi::ArmArchitecture);
    Utils::FileName file = Utils::FileName::fromString(QFileDialog::getOpenFileName(this, tr("Select GDB Executable"), gdbPath.toString()));
    if (file.isEmpty())
        return;
    m_ui->GdbLocationLineEdit->setText(file.toUserOutput());
    gdbLocationEditingFinished();
}

void AndroidSettingsWidget::browseGdbserverLocation()
{
    Utils::FileName gdbserverPath = AndroidConfigurations::instance().gdbServerPath(ProjectExplorer::Abi::ArmArchitecture);
    Utils::FileName file = Utils::FileName::fromString(QFileDialog::getOpenFileName(this, tr("Select GDB Server Android Executable"), gdbserverPath.toString()));
    if (file.isEmpty())
        return;
    m_ui->GdbserverLocationLineEdit->setText(file.toUserOutput());
    gdbserverLocationEditingFinished();
}

void AndroidSettingsWidget::browseGdbLocationX86()
{
    Utils::FileName gdbPath = AndroidConfigurations::instance().gdbPath(ProjectExplorer::Abi::X86Architecture);
    Utils::FileName file = Utils::FileName::fromString(QFileDialog::getOpenFileName(this, tr("Select GDB Executable"), gdbPath.toString()));
    if (file.isEmpty())
        return;
    m_ui->GdbLocationLineEditx86->setText(file.toUserOutput());
    gdbLocationX86EditingFinished();
}

void AndroidSettingsWidget::browseGdbserverLocationX86()
{
    Utils::FileName gdbserverPath = AndroidConfigurations::instance().gdbServerPath(ProjectExplorer::Abi::X86Architecture);
    Utils::FileName file = Utils::FileName::fromString(QFileDialog::getOpenFileName(this, tr("Select GDB Server Android Executable"), gdbserverPath.toString()));
    if (file.isEmpty())
        return;
    m_ui->GdbserverLocationLineEditx86->setText(file.toUserOutput());
    gdbserverLocationX86EditingFinished();
}

void AndroidSettingsWidget::browseOpenJDKLocation()
{
    Utils::FileName openJDKPath = AndroidConfigurations::instance().openJDKPath();
    Utils::FileName file = Utils::FileName::fromString(QFileDialog::getOpenFileName(this, tr("Select OpenJDK Path"), openJDKPath.toString()));
    if (file.isEmpty())
        return;
    m_ui->OpenJDKLocationLineEdit->setText(file.toUserOutput());
    openJDKLocationEditingFinished();
}

void AndroidSettingsWidget::addAVD()
{
    AndroidConfigurations::instance().createAVD();
    m_AVDModel.setAvdList(AndroidConfigurations::instance().androidVirtualDevices());
}

void AndroidSettingsWidget::removeAVD()
{
    AndroidConfigurations::instance().removeAVD(m_AVDModel.avdName(m_ui->AVDTableView->currentIndex()));
    m_AVDModel.setAvdList(AndroidConfigurations::instance().androidVirtualDevices());
}

void AndroidSettingsWidget::startAVD()
{
    int tempApiLevel = -1;
    AndroidConfigurations::instance().startAVD(&tempApiLevel, m_AVDModel.avdName(m_ui->AVDTableView->currentIndex()));
}

void AndroidSettingsWidget::avdActivated(QModelIndex index)
{
    m_ui->AVDRemovePushButton->setEnabled(index.isValid());
    m_ui->AVDStartPushButton->setEnabled(index.isValid());
}

void AndroidSettingsWidget::dataPartitionSizeEditingFinished()
{
    m_androidConfig.partitionSize = m_ui->DataPartitionSizeSpinBox->value();
}

void AndroidSettingsWidget::manageAVD()
{
    QProcess *avdProcess = new QProcess();
    connect(this, SIGNAL(destroyed()), avdProcess, SLOT(deleteLater()));
    connect(avdProcess, SIGNAL(finished(int)), avdProcess, SLOT(deleteLater()));
    avdProcess->start(AndroidConfigurations::instance().androidToolPath().toString(),
                      QStringList() << QLatin1String("avd"));
}


} // namespace Internal
} // namespace Android
