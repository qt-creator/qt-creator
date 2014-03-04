/**************************************************************************
**
** Copyright (c) 2014 BogDan Vatra <bog_dan_ro@yahoo.com>
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
#include "androidtoolchain.h"

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/pathchooser.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/kitinformation.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>

#include <QFile>
#include <QTextStream>
#include <QProcess>

#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QModelIndex>
#include <QtCore/QUrl>

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
        case 2: {
            QStringList cpuAbis = m_list[index.row()].cpuAbi;
            return cpuAbis.isEmpty() ? QVariant() : QVariant(cpuAbis.first());
        }
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
      m_sdkState(NotSet),
      m_ndkState(NotSet),
      m_javaState(NotSet),
      m_ui(new Ui_AndroidSettingsWidget),
      m_androidConfig(AndroidConfigurations::currentConfig())
{
    m_ui->setupUi(this);

    m_ui->SDKLocationLineEdit->setText(m_androidConfig.sdkLocation().toUserOutput());
    m_ui->NDKLocationLineEdit->setText(m_androidConfig.ndkLocation().toUserOutput());

    m_ui->AntLocationLineEdit->setText(m_androidConfig.antLocation().toUserOutput());
    m_ui->OpenJDKLocationLineEdit->setText(m_androidConfig.openJDKLocation().toUserOutput());
    m_ui->DataPartitionSizeSpinBox->setValue(m_androidConfig.partitionSize());
    m_ui->CreateKitCheckBox->setChecked(m_androidConfig.automaticKitCreation());
    m_ui->AVDTableView->setModel(&m_AVDModel);
    m_AVDModel.setAvdList(m_androidConfig.androidVirtualDevices());
    m_ui->AVDTableView->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    m_ui->AVDTableView->horizontalHeader()->setResizeMode(1, QHeaderView::ResizeToContents);

    m_ui->downloadAntToolButton->setVisible(Utils::HostOsInfo::isWindowsHost());
    m_ui->downloadOpenJDKToolButton->setVisible(Utils::HostOsInfo::isWindowsHost());

    m_ui->SDKLocationPushButton->setText(Utils::PathChooser::browseButtonLabel());
    m_ui->NDKLocationPushButton->setText(Utils::PathChooser::browseButtonLabel());
    m_ui->AntLocationPushButton->setText(Utils::PathChooser::browseButtonLabel());
    m_ui->OpenJDKLocationPushButton->setText(Utils::PathChooser::browseButtonLabel());

    check(All);
    applyToUi(All);
}

AndroidSettingsWidget::~AndroidSettingsWidget()
{
    delete m_ui;
}

void AndroidSettingsWidget::check(AndroidSettingsWidget::Mode mode)
{
    if (mode & Sdk) {
        m_sdkState = Okay;
        if (m_androidConfig.sdkLocation().isEmpty())
            m_sdkState = NotSet;
        else if (!(sdkLocationIsValid() && sdkPlatformToolsInstalled()))
            m_sdkState = Error;
    }

    if (mode & Ndk) {
        m_ndkState = Okay;
        Utils::FileName platformPath = m_androidConfig.ndkLocation();
        Utils::FileName toolChainPath = m_androidConfig.ndkLocation();
        Utils::FileName sourcesPath = m_androidConfig.ndkLocation();
        if (m_androidConfig.ndkLocation().isEmpty()) {
            m_ndkState = NotSet;
        } else if (!platformPath.appendPath(QLatin1String("platforms")).toFileInfo().exists()
                || !toolChainPath.appendPath(QLatin1String("toolchains")).toFileInfo().exists()
                || !sourcesPath.appendPath(QLatin1String("sources/cxx-stl")).toFileInfo().exists()) {
            m_ndkState = Error;
        } else {
            QList<AndroidToolChainFactory::AndroidToolChainInformation> compilerPaths
                    = AndroidToolChainFactory::toolchainPathsForNdk(m_androidConfig.ndkLocation());
            m_ndkCompilerCount = compilerPaths.count();


            // See if we have qt versions for those toolchains
            QSet<ProjectExplorer::Abi::Architecture> toolchainsForArch;
            foreach (const AndroidToolChainFactory::AndroidToolChainInformation &ati, compilerPaths)
                toolchainsForArch.insert(ati.architecture);

            QSet<ProjectExplorer::Abi::Architecture> qtVersionsForArch;
            foreach (QtSupport::BaseQtVersion *qtVersion, QtSupport::QtVersionManager::versions()) {
                if (qtVersion->type() != QLatin1String(Constants::ANDROIDQT) || qtVersion->qtAbis().isEmpty())
                    continue;
                qtVersionsForArch.insert(qtVersion->qtAbis().first().architecture());
            }

            QSet<ProjectExplorer::Abi::Architecture> missingQtArchs = toolchainsForArch.subtract(qtVersionsForArch);
            if (missingQtArchs.isEmpty()) {
                m_ndkMissingQtArchs.clear();
            } else {
                if (missingQtArchs.count() == 1) {
                    m_ndkMissingQtArchs = tr("Qt version for architecture %1 is missing.\n"
                                             "To add the Qt version, select Options > Build & Run > Qt Versions.")
                            .arg(ProjectExplorer::Abi::toString((*missingQtArchs.constBegin())));
                } else {
                    QStringList missingArchs;
                    foreach (ProjectExplorer::Abi::Architecture arch, missingQtArchs)
                        missingArchs.append(ProjectExplorer::Abi::toString(arch));
                    m_ndkMissingQtArchs = tr("Qt versions for architectures %1 are missing.\n"
                                             "To add the Qt versions, select Options > Build & Run > Qt Versions.")
                            .arg(missingArchs.join(QLatin1String(", ")));
                }
            }
        }
    }

    if (mode & Java) {
        m_javaState = Okay;
        if (m_androidConfig.openJDKLocation().isEmpty()) {
            m_javaState = NotSet;
        } else {
            Utils::FileName bin = m_androidConfig.openJDKLocation();
            bin.appendPath(QLatin1String("bin"));
            if (!m_androidConfig.openJDKLocation().toFileInfo().exists()
                    || !bin.toFileInfo().exists())
                m_javaState = Error;
        }
    }
}

void AndroidSettingsWidget::applyToUi(AndroidSettingsWidget::Mode mode)
{
    if (mode & Sdk) {
        if (m_sdkState == Error) {
            m_ui->sdkWarningIconLabel->setVisible(true);
            m_ui->sdkWarningLabel->setVisible(true);
            Utils::FileName location = Utils::FileName::fromUserInput(m_ui->SDKLocationLineEdit->text());
            if (sdkLocationIsValid())
                m_ui->sdkWarningLabel->setText(tr("The Platform tools are missing. Please use the Android SDK Manager to install them."));
            else
                m_ui->sdkWarningLabel->setText(tr("\"%1\" does not seem to be an Android SDK top folder.").arg(location.toUserOutput()));
        } else {
            m_ui->sdkWarningIconLabel->setVisible(false);
            m_ui->sdkWarningLabel->setVisible(false);
        }
    }

    if (mode & Ndk) {
        Utils::FileName location = Utils::FileName::fromUserInput(m_ui->NDKLocationLineEdit->text());
        if (m_ndkState == NotSet) {
            m_ui->ndkWarningIconLabel->setVisible(false);
            m_ui->toolchainFoundLabel->setVisible(false);
            m_ui->kitWarningIconLabel->setVisible(false);
            m_ui->kitWarningLabel->setVisible(false);
        } else if (m_ndkState == Error) {
            m_ui->toolchainFoundLabel->setText(tr("\"%1\" does not seem to be an Android NDK top folder.").arg(location.toUserOutput()));
            m_ui->toolchainFoundLabel->setVisible(true);
            m_ui->ndkWarningIconLabel->setVisible(true);
        } else {
            if (m_ndkCompilerCount > 0) {
                m_ui->ndkWarningIconLabel->setVisible(false);
                m_ui->toolchainFoundLabel->setText(tr("Found %n toolchains for this NDK.", 0, m_ndkCompilerCount));
                m_ui->toolchainFoundLabel->setVisible(true);
            } else {
                m_ui->ndkWarningIconLabel->setVisible(false);
                m_ui->toolchainFoundLabel->setVisible(false);
            }

            if (m_ndkMissingQtArchs.isEmpty()) {
                m_ui->kitWarningIconLabel->setVisible(false);
                m_ui->kitWarningLabel->setVisible(false);
            } else {
                m_ui->kitWarningIconLabel->setVisible(true);
                m_ui->kitWarningLabel->setVisible(true);
                m_ui->kitWarningLabel->setText(m_ndkMissingQtArchs);
            }
        }
    }

    if (mode & Java) {
        Utils::FileName location = m_androidConfig.openJDKLocation();
        bool error = m_javaState == Error;
        m_ui->jdkWarningIconLabel->setVisible(error);
        m_ui->jdkWarningLabel->setVisible(error);
        if (error)
            m_ui->jdkWarningLabel->setText(tr("\"%1\" does not seem to be a JDK folder.").arg(location.toUserOutput()));
    }

    if (mode & Sdk || mode & Java) {
        if (m_sdkState == Okay && m_javaState == Okay) {
            m_ui->AVDManagerFrame->setEnabled(true);
        } else {
            m_ui->AVDManagerFrame->setEnabled(false);
        }

        m_AVDModel.setAvdList(m_androidConfig.androidVirtualDevices());
    }
}

bool AndroidSettingsWidget::sdkLocationIsValid() const
{
    Utils::FileName androidExe = m_androidConfig.sdkLocation();
    Utils::FileName androidBat = m_androidConfig.sdkLocation();
    Utils::FileName emulator = m_androidConfig.sdkLocation();
    return (androidExe.appendPath(QLatin1String("/tools/android" QTC_HOST_EXE_SUFFIX)).toFileInfo().exists()
            || androidBat.appendPath(QLatin1String("/tools/android" ANDROID_BAT_SUFFIX)).toFileInfo().exists())
            && emulator.appendPath(QLatin1String("/tools/emulator" QTC_HOST_EXE_SUFFIX)).toFileInfo().exists();
}

bool AndroidSettingsWidget::sdkPlatformToolsInstalled() const
{
    Utils::FileName adb = m_androidConfig.sdkLocation();
    return adb.appendPath(QLatin1String("platform-tools/adb" QTC_HOST_EXE_SUFFIX)).toFileInfo().exists();
}

void AndroidSettingsWidget::saveSettings()
{
    sdkLocationEditingFinished();
    ndkLocationEditingFinished();
    antLocationEditingFinished();
    openJDKLocationEditingFinished();
    dataPartitionSizeEditingFinished();
    AndroidConfigurations::setConfig(m_androidConfig);
}

int indexOf(const QList<AndroidToolChainFactory::AndroidToolChainInformation> &list, const Utils::FileName &f)
{
    int end = list.count();
    for (int i = 0; i < end; ++i) {
        if (list.at(i).compilerCommand == f)
            return i;
    }
    return -1;
}

void AndroidSettingsWidget::sdkLocationEditingFinished()
{
    m_androidConfig.setSdkLocation(Utils::FileName::fromUserInput(m_ui->SDKLocationLineEdit->text()));

    check(Sdk);

    if (m_sdkState == Okay)
        searchForAnt(m_androidConfig.sdkLocation());

    applyToUi(Sdk);
}

void AndroidSettingsWidget::ndkLocationEditingFinished()
{
    m_androidConfig.setNdkLocation(Utils::FileName::fromUserInput(m_ui->NDKLocationLineEdit->text()));

    check(Ndk);

    if (m_ndkState == Okay)
        searchForAnt(m_androidConfig.ndkLocation());

    applyToUi(Ndk);
}

void AndroidSettingsWidget::searchForAnt(const Utils::FileName &location)
{
    if (!m_androidConfig.antLocation().isEmpty())
            return;
    if (location.isEmpty())
        return;
    QDir parentFolder = location.toFileInfo().absoluteDir();
    foreach (const QString &file, parentFolder.entryList()) {
        if (file.startsWith(QLatin1String("apache-ant"))) {
            Utils::FileName ant = Utils::FileName::fromString(parentFolder.absolutePath());
            ant.appendPath(file).appendPath(QLatin1String("bin"));
            if (Utils::HostOsInfo::isWindowsHost())
                ant.appendPath(QLatin1String("ant.bat"));
            else
                ant.appendPath(QLatin1String("ant"));
            if (ant.toFileInfo().exists()) {
                m_androidConfig.setAntLocation(ant);
                m_ui->AntLocationLineEdit->setText(ant.toUserOutput());
            }
        }
    }
}

void AndroidSettingsWidget::antLocationEditingFinished()
{
    m_androidConfig.setAntLocation(Utils::FileName::fromUserInput(m_ui->AntLocationLineEdit->text()));
}

void AndroidSettingsWidget::openJDKLocationEditingFinished()
{
    m_androidConfig.setOpenJDKLocation(Utils::FileName::fromUserInput(m_ui->OpenJDKLocationLineEdit->text()));

    check(Java);
    applyToUi(Java);
}

void AndroidSettingsWidget::browseSDKLocation()
{
    Utils::FileName dir = Utils::FileName::fromString(
                QFileDialog::getExistingDirectory(this, tr("Select Android SDK folder"),
                                                  m_ui->SDKLocationLineEdit->text()));
    if (dir.isEmpty())
        return;
    m_ui->SDKLocationLineEdit->setText(dir.toUserOutput());
    sdkLocationEditingFinished();
}

void AndroidSettingsWidget::browseNDKLocation()
{
    Utils::FileName dir = Utils::FileName::fromString(
                QFileDialog::getExistingDirectory(this, tr("Select Android NDK folder"),
                                                  m_ui->NDKLocationLineEdit->text()));
    if (dir.isEmpty())
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

void AndroidSettingsWidget::browseOpenJDKLocation()
{
    Utils::FileName openJDKPath = m_androidConfig.openJDKLocation();
    Utils::FileName file = Utils::FileName::fromString(QFileDialog::getExistingDirectory(this, tr("Select OpenJDK Path"), openJDKPath.toString()));
    if (file.isEmpty())
        return;
    m_ui->OpenJDKLocationLineEdit->setText(file.toUserOutput());
    openJDKLocationEditingFinished();
}

void AndroidSettingsWidget::openSDKDownloadUrl()
{
    QDesktopServices::openUrl(QUrl::fromUserInput(QLatin1String("http://developer.android.com/sdk")));
}

void AndroidSettingsWidget::openNDKDownloadUrl()
{
    QDesktopServices::openUrl(QUrl::fromUserInput(QLatin1String("http://developer.android.com/tools/sdk/ndk/index.html#Downloads")));
}

void AndroidSettingsWidget::openAntDownloadUrl()
{
    QDesktopServices::openUrl(QUrl::fromUserInput(QLatin1String("http://ant.apache.org/bindownload.cgi")));
}

void AndroidSettingsWidget::openOpenJDKDownloadUrl()
{
    QDesktopServices::openUrl(QUrl::fromUserInput(QLatin1String("http://www.oracle.com/technetwork/java/javase/downloads")));
}

void AndroidSettingsWidget::addAVD()
{
    m_androidConfig.createAVD(this);
    m_AVDModel.setAvdList(m_androidConfig.androidVirtualDevices());
    avdActivated(m_ui->AVDTableView->currentIndex());
}

void AndroidSettingsWidget::removeAVD()
{
    m_androidConfig.removeAVD(m_AVDModel.avdName(m_ui->AVDTableView->currentIndex()));
    m_AVDModel.setAvdList(m_androidConfig.androidVirtualDevices());
    avdActivated(m_ui->AVDTableView->currentIndex());
}

void AndroidSettingsWidget::startAVD()
{
    m_androidConfig.startAVDAsync(m_AVDModel.avdName(m_ui->AVDTableView->currentIndex()));
}

void AndroidSettingsWidget::avdActivated(QModelIndex index)
{
    m_ui->AVDRemovePushButton->setEnabled(index.isValid());
    m_ui->AVDStartPushButton->setEnabled(index.isValid());
}

void AndroidSettingsWidget::dataPartitionSizeEditingFinished()
{
    m_androidConfig.setPartitionSize(m_ui->DataPartitionSizeSpinBox->value());
}

void AndroidSettingsWidget::createKitToggled()
{
    m_androidConfig.setAutomaticKitCreation(m_ui->CreateKitCheckBox->isChecked());
}

void AndroidSettingsWidget::manageAVD()
{
    QProcess *avdProcess = new QProcess();
    connect(this, SIGNAL(destroyed()), avdProcess, SLOT(deleteLater()));
    connect(avdProcess, SIGNAL(finished(int)), avdProcess, SLOT(deleteLater()));

    avdProcess->setProcessEnvironment(m_androidConfig.androidToolEnvironment().toProcessEnvironment());
    QString executable = m_androidConfig.androidToolPath().toString();
    QStringList arguments = QStringList() << QLatin1String("avd");

    avdProcess->start(executable, arguments);
}


} // namespace Internal
} // namespace Android
