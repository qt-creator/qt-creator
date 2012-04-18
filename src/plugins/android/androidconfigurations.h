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

#ifndef ANDROIDCONFIGURATIONS_H
#define ANDROIDCONFIGURATIONS_H

#include <QObject>
#include <QString>
#include <QVector>
#include <projectexplorer/abi.h>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Android {
namespace Internal {

#ifdef Q_OS_LINUX
    const QLatin1String ToolchainHost("linux-x86");
#else
# ifdef Q_OS_DARWIN
    const QLatin1String ToolchainHost("darwin-x86");
# else
#  ifdef Q_OS_WIN32
    const QLatin1String ToolchainHost("windows");
#  else
#  warning No Android supported OSs found
    const QLatin1String ToolchainHost("linux-x86");
#  endif
# endif
#endif

class AndroidConfig
{
public:
    AndroidConfig();
    AndroidConfig(const QSettings &settings);
    void save(QSettings &settings) const;

    QString sdkLocation;
    QString ndkLocation;
    QString ndkToolchainVersion;
    QString antLocation;
    QString armGdbLocation;
    QString armGdbserverLocation;
    QString x86GdbLocation;
    QString x86GdbserverLocation;
    QString openJDKLocation;
    QString keystoreLocation;
    unsigned partitionSize;
};

struct AndroidDevice {
    QString serialNumber;
    QString cpuABI;
    int sdk;
};

class AndroidConfigurations : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(AndroidConfigurations)
public:

    static AndroidConfigurations &instance(QObject *parent = 0);
    AndroidConfig config() const { return m_config; }
    void setConfig(const AndroidConfig &config);
    QStringList sdkTargets(int minApiLevel = 0) const;
    QStringList ndkToolchainVersions() const;
    QString adbToolPath() const;
    QString androidToolPath() const;
    QString antToolPath() const;
    QString emulatorToolPath() const;
    QString gccPath(ProjectExplorer::Abi::Architecture architecture) const;
    QString gdbServerPath(ProjectExplorer::Abi::Architecture architecture) const;
    QString gdbPath(ProjectExplorer::Abi::Architecture architecture) const;
    QString openJDKPath() const;
    QString keytoolPath() const;
    QString jarsignerPath() const;
    QString stripPath(ProjectExplorer::Abi::Architecture architecture) const;
    QString readelfPath(ProjectExplorer::Abi::Architecture architecture) const;
    QString getDeployDeviceSerialNumber(int *apiLevel) const;
    bool createAVD(const QString &target, const QString &name, int sdcardSize) const;
    bool removeAVD(const QString &name) const;
    QVector<AndroidDevice> connectedDevices(int apiLevel = -1) const;
    QVector<AndroidDevice> androidVirtualDevices() const;
    QString startAVD(int *apiLevel, const QString &name = QString()) const;
    QString bestMatch(const QString &targetAPI) const;

    static QLatin1String toolchainPrefix(ProjectExplorer::Abi::Architecture architecture);
    static QLatin1String toolsPrefix(ProjectExplorer::Abi::Architecture architecture);

signals:
    void updated();

public slots:
    bool createAVD(int minApiLevel = 0) const;

private:
    QString toolPath(ProjectExplorer::Abi::Architecture architecture) const;
    QString openJDKBinPath() const;

    AndroidConfigurations(QObject *parent);
    void load();
    void save();

    int getSDKVersion(const QString &device) const;
    void updateAvailablePlatforms();


    static AndroidConfigurations *m_instance;
    AndroidConfig m_config;
    QVector<int> m_availablePlatforms;
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDCONFIGURATIONS_H
