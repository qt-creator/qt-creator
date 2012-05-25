/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 BogDan Vatra <bog_dan_ro@yahoo.com>
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "androidsettingswidget.h"

#include "ui_androidsettingswidget.h"

#include "androidconfigurations.h"

#include "androidconstants.h"

#include <QFile>
#include <QTextStream>
#include <QProcess>

#include <QFileDialog>
#include <QMessageBox>
#include <QModelIndex>

namespace Android {
namespace Internal {

void AVDModel::setAvdList(QVector<AndroidDevice> list)
{
    m_list = list;
    reset();
}

QString AVDModel::avdName(const QModelIndex &index)
{
    return m_list[index.row()].serialNumber;
}

QVariant AVDModel::data(const QModelIndex &index, int role) const
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

QVariant AVDModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case 0:
                return tr("AVD Name");
            case 1:
                return tr("AVD Target");
            case 2:
                return tr("CPU/ABI");
        }
    }
    return QAbstractItemModel::headerData(section, orientation, role );
}

int AVDModel::rowCount(const QModelIndex &/*parent*/) const
{
    return m_list.size();
}

int AVDModel::columnCount(const QModelIndex &/*parent*/) const
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
    m_ui->toolchainVersionComboBox->clear();
    if (checkSDK(m_androidConfig.sdkLocation))
        m_ui->SDKLocationLineEdit->setText(m_androidConfig.sdkLocation);
    else
        m_androidConfig.sdkLocation.clear();
    if (checkNDK(m_androidConfig.ndkLocation))
        m_ui->NDKLocationLineEdit->setText(m_androidConfig.ndkLocation);
    else
        m_androidConfig.ndkLocation.clear();
    m_ui->AntLocationLineEdit->setText(m_androidConfig.antLocation);
    m_ui->GdbLocationLineEdit->setText(m_androidConfig.armGdbLocation);
    m_ui->GdbserverLocationLineEdit->setText(m_androidConfig.armGdbserverLocation);
    m_ui->GdbLocationLineEditx86->setText(m_androidConfig.x86GdbLocation);
    m_ui->GdbserverLocationLineEditx86->setText(m_androidConfig.x86GdbserverLocation);
    m_ui->OpenJDKLocationLineEdit->setText(m_androidConfig.openJDKLocation);
    m_ui->DataPartitionSizeSpinBox->setValue(m_androidConfig.partitionSize);
    m_ui->AVDTableView->setModel(&m_AVDModel);
    m_AVDModel.setAvdList(AndroidConfigurations::instance().androidVirtualDevices());
    m_ui->AVDTableView->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    m_ui->AVDTableView->horizontalHeader()->setResizeMode(1, QHeaderView::ResizeToContents);
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


bool AndroidSettingsWidget::checkSDK(const QString &location)
{
    if (!location.length())
        return false;
    if (!QFile::exists(location + QLatin1String("/platform-tools/adb" ANDROID_EXE_SUFFIX))
            || (!QFile::exists(location + QLatin1String("/tools/android" ANDROID_EXE_SUFFIX))
                && !QFile::exists(location + QLatin1String("/tools/android" ANDROID_BAT_SUFFIX)))
            || !QFile::exists(location + QLatin1String("/tools/emulator" ANDROID_EXE_SUFFIX))) {
        QMessageBox::critical(this, tr("Android SDK Folder"), tr("\"%1\" doesn't seem to be an Android SDK top folder").arg(location));
        return false;
    }
    return true;
}

bool AndroidSettingsWidget::checkNDK(const QString &location)
{
    m_ui->toolchainVersionComboBox->setEnabled(false);
    m_ui->GdbLocationLineEdit->setEnabled(false);
    m_ui->GdbLocationPushButton->setEnabled(false);
    m_ui->GdbserverLocationLineEdit->setEnabled(false);
    m_ui->GdbserverLocationPushButton->setEnabled(false);
    if (!location.length())
        return false;
    if (!QFile::exists(location + QLatin1String("/platforms"))
            || !QFile::exists(location + QLatin1String("/toolchains"))
            || !QFile::exists(location + QLatin1String("/sources/cxx-stl"))) {
        QMessageBox::critical(this, tr("Android SDK Folder"), tr("\"%1\" doesn't seem to be an Android NDK top folder").arg(location));
        return false;
    }
    m_ui->toolchainVersionComboBox->setEnabled(true);
    m_ui->GdbLocationLineEdit->setEnabled(true);
    m_ui->GdbLocationPushButton->setEnabled(true);
    m_ui->GdbserverLocationLineEdit->setEnabled(true);
    m_ui->GdbserverLocationPushButton->setEnabled(true);
    fillToolchainVersions();
    return true;

}

void AndroidSettingsWidget::sdkLocationEditingFinished()
{
    QString location = m_ui->SDKLocationLineEdit->text();
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
    QString location = m_ui->NDKLocationLineEdit->text();
    if (!checkNDK(location))
        return;
    m_androidConfig.ndkLocation = location;
    saveSettings(true);
}

void AndroidSettingsWidget::fillToolchainVersions()
{
    m_ui->toolchainVersionComboBox->clear();
    QStringList toolchainVersions = AndroidConfigurations::instance().ndkToolchainVersions();
    QString toolchain = m_androidConfig.ndkToolchainVersion;
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
    QString location = m_ui->AntLocationLineEdit->text();
    if (!location.length() || !QFile::exists(location))
        return;
    m_androidConfig.antLocation = location;
}

void AndroidSettingsWidget::gdbLocationEditingFinished()
{
    QString location = m_ui->GdbLocationLineEdit->text();
    if (!location.length() || !QFile::exists(location))
        return;
    m_androidConfig.armGdbLocation = location;
}

void AndroidSettingsWidget::gdbserverLocationEditingFinished()
{
    QString location = m_ui->GdbserverLocationLineEdit->text();
    if (!location.length() || !QFile::exists(location))
        return;
    m_androidConfig.armGdbserverLocation = location;
}

void AndroidSettingsWidget::gdbLocationX86EditingFinished()
{
    QString location = m_ui->GdbLocationLineEditx86->text();
    if (!location.length() || !QFile::exists(location))
        return;
    m_androidConfig.x86GdbLocation = location;
}

void AndroidSettingsWidget::gdbserverLocationX86EditingFinished()
{
    QString location = m_ui->GdbserverLocationLineEditx86->text();
    if (!location.length() || !QFile::exists(location))
        return;
    m_androidConfig.x86GdbserverLocation = location;
}

void AndroidSettingsWidget::openJDKLocationEditingFinished()
{
    QString location = m_ui->OpenJDKLocationLineEdit->text();
    if (!location.length() || !QFile::exists(location))
        return;
    m_androidConfig.openJDKLocation = location;
}

void AndroidSettingsWidget::browseSDKLocation()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Android SDK Folder"));
    if (!checkSDK(dir))
        return;
    m_ui->SDKLocationLineEdit->setText(dir);
    sdkLocationEditingFinished();
}

void AndroidSettingsWidget::browseNDKLocation()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Android NDK Folder"));
    if (!checkNDK(dir))
        return;
    m_ui->NDKLocationLineEdit->setText(dir);
    ndkLocationEditingFinished();
}

void AndroidSettingsWidget::browseAntLocation()
{
    QString dir = QDir::homePath();
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
    dir = QLatin1String("/usr/bin/ant");
    QLatin1String antApp("ant");
#elif defined(Q_OS_WIN)
    QLatin1String antApp("ant.bat");
#elif defined(Q_OS_DARWIN)
    dir = QLatin1String("/opt/local/bin/ant");
    QLatin1String antApp("ant");
#endif
    const QString file =
        QFileDialog::getOpenFileName(this, tr("Select ant Script"), dir, antApp);
    if (!file.length())
        return;
    m_ui->AntLocationLineEdit->setText(file);
    antLocationEditingFinished();
}

void AndroidSettingsWidget::browseGdbLocation()
{
    QString gdbPath = AndroidConfigurations::instance().gdbPath(ProjectExplorer::Abi::ArmArchitecture);
    QString file = QFileDialog::getOpenFileName(this, tr("Select gdb Executable"),gdbPath);
    if (!file.length())
        return;
    m_ui->GdbLocationLineEdit->setText(file);
    gdbLocationEditingFinished();
}

void AndroidSettingsWidget::browseGdbserverLocation()
{
    QString gdbserverPath = AndroidConfigurations::instance().gdbServerPath(ProjectExplorer::Abi::ArmArchitecture);
    const QString file =
        QFileDialog::getOpenFileName(this, tr("Select gdbserver Android Executable"),gdbserverPath);
    if (!file.length())
        return;
    m_ui->GdbserverLocationLineEdit->setText(file);
    gdbserverLocationEditingFinished();
}

void AndroidSettingsWidget::browseGdbLocationX86()
{
    QString gdbPath = AndroidConfigurations::instance().gdbPath(ProjectExplorer::Abi::X86Architecture);
    const QString file =
        QFileDialog::getOpenFileName(this, tr("Select gdb Executable"),gdbPath);
    if (!file.length())
        return;
    m_ui->GdbLocationLineEditx86->setText(file);
    gdbLocationX86EditingFinished();
}

void AndroidSettingsWidget::browseGdbserverLocationX86()
{
    QString gdbserverPath = AndroidConfigurations::instance().gdbServerPath(ProjectExplorer::Abi::X86Architecture);
    const QString file =
        QFileDialog::getOpenFileName(this, tr("Select gdbserver Android Executable"), gdbserverPath);
    if (!file.length())
        return;
    m_ui->GdbserverLocationLineEditx86->setText(file);
    gdbserverLocationX86EditingFinished();
}

void AndroidSettingsWidget::browseOpenJDKLocation()
{
    QString openJDKPath = AndroidConfigurations::instance().openJDKPath();
    const QString file =
        QFileDialog::getOpenFileName(this, tr("Select OpenJDK Path"), openJDKPath);
    if (!file.length())
        return;
    m_ui->OpenJDKLocationLineEdit->setText(file);
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
    avdProcess->start(AndroidConfigurations::instance().androidToolPath(), QStringList() << QLatin1String("avd"));
}


} // namespace Internal
} // namespace Android
