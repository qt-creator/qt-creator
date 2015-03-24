/**************************************************************************
**
** Copyright (C) 2015 BogDan Vatra <bog_dan_ro@yahoo.com>
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
#include <QTimer>
#include <QTime>

#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QModelIndex>
#include <QtCore/QUrl>
#include <QtConcurrentRun>

namespace Android {
namespace Internal {

void AvdModel::setAvdList(const QVector<AndroidDeviceInfo> &list)
{
    beginResetModel();
    m_list = list;
    endResetModel();
}

QModelIndex AvdModel::indexForAvdName(const QString &avdName) const
{
    for (int i = 0; i < m_list.size(); ++i) {
        if (m_list.at(i).serialNumber == avdName)
            return index(i, 0);
    }
    return QModelIndex();
}

QString AvdModel::avdName(const QModelIndex &index) const
{
    return m_list.at(index.row()).serialNumber;
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

    connect(&m_checkGdbWatcher, SIGNAL(finished()),
            this, SLOT(checkGdbFinished()));

    m_ui->SDKLocationPathChooser->setFileName(m_androidConfig.sdkLocation());
    m_ui->SDKLocationPathChooser->setPromptDialogTitle(tr("Select Android SDK folder"));
    m_ui->NDKLocationPathChooser->setFileName(m_androidConfig.ndkLocation());
    m_ui->NDKLocationPathChooser->setPromptDialogTitle(tr("Select Android NDK folder"));

    QString dir;
    QString filter;
    if (Utils::HostOsInfo::isWindowsHost()) {
        dir = QDir::homePath() + QLatin1String("/ant.bat");
        filter = QLatin1String("ant (ant.bat)");
    } else if (Utils::HostOsInfo::isMacHost()) {
        // work around QTBUG-7739 that prohibits filters that don't start with *
        dir = QLatin1String("/usr/bin/ant");
        filter = QLatin1String("ant (*ant)");
    } else {
        dir = QLatin1String("/usr/bin/ant");
        filter = QLatin1String("ant (ant)");
    }
    m_ui->AntLocationPathChooser->setFileName(m_androidConfig.antLocation());
    m_ui->AntLocationPathChooser->setExpectedKind(Utils::PathChooser::Command);
    m_ui->AntLocationPathChooser->setPromptDialogTitle(tr("Select ant Script"));
    m_ui->AntLocationPathChooser->setInitialBrowsePathBackup(dir);
    m_ui->AntLocationPathChooser->setPromptDialogFilter(filter);
    m_ui->UseGradleCheckBox->setChecked(m_androidConfig.useGrandle());

    m_ui->OpenJDKLocationPathChooser->setFileName(m_androidConfig.openJDKLocation());
    m_ui->OpenJDKLocationPathChooser->setPromptDialogTitle(tr("Select JDK Path"));
    m_ui->DataPartitionSizeSpinBox->setValue(m_androidConfig.partitionSize());
    m_ui->CreateKitCheckBox->setChecked(m_androidConfig.automaticKitCreation());
    m_ui->AVDTableView->setModel(&m_AVDModel);
    m_ui->AVDTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_ui->AVDTableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    m_ui->downloadAntToolButton->setVisible(!Utils::HostOsInfo::isLinuxHost());
    m_ui->downloadOpenJDKToolButton->setVisible(!Utils::HostOsInfo::isLinuxHost());

    connect(m_ui->gdbWarningLabel, SIGNAL(linkActivated(QString)),
            this, SLOT(showGdbWarningDialog()));

    connect(&m_virtualDevicesWatcher, SIGNAL(finished()),
            this, SLOT(updateAvds()));

    check(All);
    applyToUi(All);

    connect(&m_futureWatcher, SIGNAL(finished()),
            this, SLOT(avdAdded()));

    connect(m_ui->NDKLocationPathChooser, SIGNAL(changed(QString)), this, SLOT(ndkLocationEditingFinished()));
    connect(m_ui->SDKLocationPathChooser, SIGNAL(changed(QString)), this, SLOT(sdkLocationEditingFinished()));
    connect(m_ui->AntLocationPathChooser, SIGNAL(changed(QString)), this, SLOT(antLocationEditingFinished()));
    connect(m_ui->OpenJDKLocationPathChooser, SIGNAL(changed(QString)), this, SLOT(openJDKLocationEditingFinished()));

}

AndroidSettingsWidget::~AndroidSettingsWidget()
{
    delete m_ui;
    m_futureWatcher.waitForFinished();
}

// NOTE: Will be run via QFuture
static QPair<QStringList, bool> checkGdbForBrokenPython(const QStringList &paths)
{
    foreach (const QString &path, paths) {
        QTime timer;
        timer.start();
        QProcess proc;
        proc.setProcessChannelMode(QProcess::MergedChannels);
        proc.start(path);
        proc.waitForStarted();

        QByteArray output;
        while (proc.waitForReadyRead(300)) {
            output += proc.readAll();
            if (output.contains("(gdb)"))
                break;
            if (timer.elapsed() > 7 * 1000)
                return qMakePair(paths, true); // Took too long, abort
        }

        output.clear();

        proc.write("python import string\n");
        proc.write("python print(string.ascii_uppercase)\n");
        proc.write("python import struct\n");
        proc.write("quit\n");
        while (proc.waitForFinished(300)) {
            if (timer.elapsed() > 9 * 1000)
                return qMakePair(paths, true); // Took too long, abort
        }
        proc.waitForFinished();

        output = proc.readAll();

        bool error = output.contains("_PyObject_Free")
                || output.contains("_PyExc_IOError")
                || output.contains("_sysconfigdata_nd ")
                || !output.contains("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        if (error)
            return qMakePair(paths, error);
    }
    return qMakePair(paths, false);
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
        m_ui->gdbWarningIconLabel->setVisible(false);
        m_ui->gdbWarningLabel->setVisible(false);
        if (m_androidConfig.ndkLocation().isEmpty()) {
            m_ndkState = NotSet;
        } else if (!platformPath.appendPath(QLatin1String("platforms")).exists()
                || !toolChainPath.appendPath(QLatin1String("toolchains")).exists()
                || !sourcesPath.appendPath(QLatin1String("sources/cxx-stl")).exists()) {
            m_ndkState = Error;
            m_ndkErrorMessage = tr("\"%1\" does not seem to be an Android NDK top folder.")
                    .arg(m_androidConfig.ndkLocation().toUserOutput());
        } else if (platformPath.toString().contains(QLatin1Char(' '))) {
            m_ndkState = Error;
            m_ndkErrorMessage = tr("The Android NDK cannot be installed into a path with spaces.");
        } else {
            QList<AndroidToolChainFactory::AndroidToolChainInformation> compilerPaths
                    = AndroidToolChainFactory::toolchainPathsForNdk(m_androidConfig.ndkLocation());
            m_ndkCompilerCount = compilerPaths.count();

            // Check for a gdb with a broken python
            QStringList gdbPaths;
            foreach (const AndroidToolChainFactory::AndroidToolChainInformation &ati, compilerPaths) {
                // we only check the arm gdbs, that's indicative enough
                if (ati.abi.architecture() != ProjectExplorer::Abi::ArmArchitecture)
                    continue;
                Utils::FileName gdbPath = m_androidConfig.gdbPath(ati.abi, ati.version);
                if (gdbPath.exists())
                    gdbPaths << gdbPath.toString();
            }

            if (!gdbPaths.isEmpty()) {
                m_checkGdbWatcher.setFuture(QtConcurrent::run(&checkGdbForBrokenPython, gdbPaths));
                m_gdbCheckPaths = gdbPaths;
            }

            // See if we have qt versions for those toolchains
            QSet<ProjectExplorer::Abi> toolchainsForAbi;
            foreach (const AndroidToolChainFactory::AndroidToolChainInformation &ati, compilerPaths)
                toolchainsForAbi.insert(ati.abi);

            QSet<ProjectExplorer::Abi> qtVersionsForAbi;
            foreach (QtSupport::BaseQtVersion *qtVersion, QtSupport::QtVersionManager::versions()) {
                if (qtVersion->type() != QLatin1String(Constants::ANDROIDQT) || qtVersion->qtAbis().isEmpty())
                    continue;
                qtVersionsForAbi.insert(qtVersion->qtAbis().first());
            }

            QSet<ProjectExplorer::Abi> missingQtArchs = toolchainsForAbi.subtract(qtVersionsForAbi);
            if (missingQtArchs.isEmpty()) {
                m_ndkMissingQtArchs.clear();
            } else {
                if (missingQtArchs.count() == 1) {
                    m_ndkMissingQtArchs = tr("Qt version for architecture %1 is missing.\n"
                                             "To add the Qt version, select Options > Build & Run > Qt Versions.")
                            .arg((*missingQtArchs.constBegin()).toString());
                } else {
                    m_ndkMissingQtArchs = tr("Qt versions for %1 architectures are missing.\n"
                                             "To add the Qt versions, select Options > Build & Run > Qt Versions.")
                            .arg(missingQtArchs.size());
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
            bin.appendPath(QLatin1String("bin/javac" QTC_HOST_EXE_SUFFIX));
            if (!m_androidConfig.openJDKLocation().exists() || !bin.exists())
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
            Utils::FileName location = Utils::FileName::fromUserInput(m_ui->SDKLocationPathChooser->rawPath());
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
        if (m_ndkState == NotSet) {
            m_ui->ndkWarningIconLabel->setVisible(false);
            m_ui->toolchainFoundLabel->setVisible(false);
            m_ui->kitWarningIconLabel->setVisible(false);
            m_ui->kitWarningLabel->setVisible(false);
        } else if (m_ndkState == Error) {
            m_ui->toolchainFoundLabel->setText(m_ndkErrorMessage);
            m_ui->toolchainFoundLabel->setVisible(true);
            m_ui->ndkWarningIconLabel->setVisible(true);
            m_ui->kitWarningIconLabel->setVisible(false);
            m_ui->kitWarningLabel->setVisible(false);
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

        startUpdateAvd();
    }
}

void AndroidSettingsWidget::disableAvdControls()
{
    m_ui->AVDAddPushButton->setEnabled(false);
    m_ui->AVDTableView->setEnabled(false);
    m_ui->AVDRemovePushButton->setEnabled(false);
    m_ui->AVDStartPushButton->setEnabled(false);
}

void AndroidSettingsWidget::enableAvdControls()
{
    m_ui->AVDTableView->setEnabled(true);
    m_ui->AVDAddPushButton->setEnabled(true);
    avdActivated(m_ui->AVDTableView->currentIndex());
}

void AndroidSettingsWidget::startUpdateAvd()
{
    disableAvdControls();
    m_virtualDevicesWatcher.setFuture(m_androidConfig.androidVirtualDevicesFuture());
}

void AndroidSettingsWidget::updateAvds()
{
    m_AVDModel.setAvdList(m_virtualDevicesWatcher.result());
    if (!m_lastAddedAvd.isEmpty()) {
        m_ui->AVDTableView->setCurrentIndex(m_AVDModel.indexForAvdName(m_lastAddedAvd));
        m_lastAddedAvd.clear();
    }
    enableAvdControls();
}

bool AndroidSettingsWidget::sdkLocationIsValid() const
{
    Utils::FileName androidExe = m_androidConfig.sdkLocation();
    Utils::FileName androidBat = m_androidConfig.sdkLocation();
    Utils::FileName emulator = m_androidConfig.sdkLocation();
    return (androidExe.appendPath(QLatin1String("/tools/android" QTC_HOST_EXE_SUFFIX)).exists()
            || androidBat.appendPath(QLatin1String("/tools/android" ANDROID_BAT_SUFFIX)).exists())
            && emulator.appendPath(QLatin1String("/tools/emulator" QTC_HOST_EXE_SUFFIX)).exists();
}

bool AndroidSettingsWidget::sdkPlatformToolsInstalled() const
{
    Utils::FileName adb = m_androidConfig.sdkLocation();
    return adb.appendPath(QLatin1String("platform-tools/adb" QTC_HOST_EXE_SUFFIX)).exists();
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
    m_androidConfig.setSdkLocation(Utils::FileName::fromUserInput(m_ui->SDKLocationPathChooser->rawPath()));

    check(Sdk);

    if (m_sdkState == Okay)
        searchForAnt(m_androidConfig.sdkLocation());

    applyToUi(Sdk);
}

void AndroidSettingsWidget::ndkLocationEditingFinished()
{
    m_androidConfig.setNdkLocation(Utils::FileName::fromUserInput(m_ui->NDKLocationPathChooser->rawPath()));

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
            if (ant.exists()) {
                m_androidConfig.setAntLocation(ant);
                m_ui->AntLocationPathChooser->setFileName(ant);
            }
        }
    }
}

void AndroidSettingsWidget::antLocationEditingFinished()
{
    m_androidConfig.setAntLocation(Utils::FileName::fromUserInput(m_ui->AntLocationPathChooser->rawPath()));
}

void AndroidSettingsWidget::openJDKLocationEditingFinished()
{
    m_androidConfig.setOpenJDKLocation(Utils::FileName::fromUserInput(m_ui->OpenJDKLocationPathChooser->rawPath()));

    check(Java);
    applyToUi(Java);
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
    disableAvdControls();
    AndroidConfig::CreateAvdInfo info = m_androidConfig.gatherCreateAVDInfo(this);

    if (info.target.isEmpty()) {
        enableAvdControls();
        return;
    }

    m_futureWatcher.setFuture(m_androidConfig.createAVD(info));
}

void AndroidSettingsWidget::avdAdded()
{
    AndroidConfig::CreateAvdInfo info = m_futureWatcher.result();
    if (!info.error.isEmpty()) {
        enableAvdControls();
        QMessageBox::critical(this, QApplication::translate("AndroidConfig", "Error Creating AVD"), info.error);
        return;
    }

    startUpdateAvd();
    m_lastAddedAvd = info.name;
}

void AndroidSettingsWidget::removeAVD()
{
    disableAvdControls();
    QString avdName = m_AVDModel.avdName(m_ui->AVDTableView->currentIndex());
    if (QMessageBox::question(this, tr("Remove Android Virtual Device"),
                              tr("Remove device \"%1\"? This cannot be undone.").arg(avdName),
                              QMessageBox::Yes | QMessageBox::No)
            == QMessageBox::No) {
        enableAvdControls();
        return;
    }

    m_androidConfig.removeAVD(avdName);
    startUpdateAvd();
}

void AndroidSettingsWidget::startAVD()
{
    m_androidConfig.startAVDAsync(m_AVDModel.avdName(m_ui->AVDTableView->currentIndex()));
}

void AndroidSettingsWidget::avdActivated(const QModelIndex &index)
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

void AndroidSettingsWidget::useGradleToggled()
{
    m_androidConfig.setUseGradle(m_ui->UseGradleCheckBox->isChecked());
}

void AndroidSettingsWidget::checkGdbFinished()
{
    QPair<QStringList, bool> result = m_checkGdbWatcher.future().result();
    if (result.first != m_gdbCheckPaths) // no longer relevant
        return;
    m_ui->gdbWarningIconLabel->setVisible(result.second);
    m_ui->gdbWarningLabel->setVisible(result.second);
}

void AndroidSettingsWidget::showGdbWarningDialog()
{
    QMessageBox::warning(this,
                         tr("Unsupported GDB"),
                         tr("The GDB inside this NDK seems to not support Python. "
                            "The Qt Project offers fixed GDB builds at: "
                            "<a href=\"http://download.qt.io/official_releases/gdb/\">"
                            "http://download.qt.io/official_releases/gdb/</a>"));
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
