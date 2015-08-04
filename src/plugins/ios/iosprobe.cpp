/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include "iosprobe.h"

#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>
#include <QLoggingCategory>
#include <QProcess>

static Q_LOGGING_CATEGORY(probeLog, "qtc.ios.probe")

namespace Ios {

static QString qsystem(const QString &exe, const QStringList &args = QStringList())
{
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.start(exe, args);
    p.waitForFinished();
    return QString::fromLocal8Bit(p.readAll());
}

QMap<QString, Platform> IosProbe::detectPlatforms(const QString &devPath)
{
    IosProbe probe;
    probe.addDeveloperPath(devPath);
    probe.detectFirst();
    return probe.detectedPlatforms();
}

static int compareVersions(const QString &v1, const QString &v2)
{
    QStringList v1L = v1.split(QLatin1Char('.'));
    QStringList v2L = v2.split(QLatin1Char('.'));
    int i = 0;
    while (v1L.length() > i && v2L.length() > i) {
        bool n1Ok, n2Ok;
        int n1 = v1L.value(i).toInt(&n1Ok);
        int n2 = v2L.value(i).toInt(&n2Ok);
        if (!(n1Ok && n2Ok)) {
            qCWarning(probeLog) << QString::fromLatin1("Failed to compare version %1 and %2").arg(v1, v2);
            return 0;
        }
        if (n1 > n2)
            return -1;
        else if (n1 < n2)
            return 1;
        ++i;
    }
    if (v1L.length() > v2L.length())
        return -1;
    if (v1L.length() < v2L.length())
        return 1;
    return 0;
}

void IosProbe::addDeveloperPath(const QString &path)
{
    if (path.isEmpty())
        return;
    QFileInfo pInfo(path);
    if (!pInfo.exists() || !pInfo.isDir())
        return;
    if (m_developerPaths.contains(path))
        return;
    m_developerPaths.append(path);
    qCDebug(probeLog) << QString::fromLatin1("Added developer path %1").arg(path);
}

void IosProbe::detectDeveloperPaths()
{
    QProcess selectedXcode;
    QString program = QLatin1String("/usr/bin/xcode-select");
    QStringList arguments(QLatin1String("--print-path"));
    selectedXcode.start(program, arguments, QProcess::ReadOnly);
    if (!selectedXcode.waitForFinished() || selectedXcode.exitCode()) {
        qCWarning(probeLog) << QString::fromLatin1("Could not detect selected xcode with /usr/bin/xcode-select");
    } else {
        QString path = QString::fromLocal8Bit(selectedXcode.readAllStandardOutput());
        path.chop(1);
        addDeveloperPath(path);
    }
    addDeveloperPath(QLatin1String("/Applications/Xcode.app/Contents/Developer"));
}

void IosProbe::setupDefaultToolchains(const QString &devPath, const QString &xcodeName)
{
    qCDebug(probeLog) << QString::fromLatin1("Setting up platform \"%1\".").arg(xcodeName);
    QString indent = QLatin1String("  ");

    // detect clang (default toolchain)
    QFileInfo clangFileInfo(devPath
                            + QLatin1String("/Toolchains/XcodeDefault.xctoolchain/usr/bin")
                            + QLatin1String("/clang++"));
    bool hasClang = clangFileInfo.exists();
    if (!hasClang)
        qCWarning(probeLog) << indent << QString::fromLatin1("Default toolchain %1 not found.")
                                .arg(clangFileInfo.canonicalFilePath());
    // Platforms
    QDir platformsDir(devPath + QLatin1String("/Platforms"));
    QFileInfoList platforms = platformsDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    foreach (const QFileInfo &fInfo, platforms) {
        if (fInfo.isDir() && fInfo.suffix() == QLatin1String("platform")) {
            qCDebug(probeLog) << indent << QString::fromLatin1("Setting up %1").arg(fInfo.fileName());
            QSettings infoSettings(fInfo.absoluteFilePath() + QLatin1String("/Info.plist"),
                                   QSettings::NativeFormat);
            if (!infoSettings.contains(QLatin1String("Name"))) {
                qCWarning(probeLog) << indent << QString::fromLatin1("Missing platform name in Info.plist of %1")
                             .arg(fInfo.absoluteFilePath());
                continue;
            }
            QString name = infoSettings.value(QLatin1String("Name")).toString();
            if (name != QLatin1String("macosx") && name != QLatin1String("iphoneos")
                    && name != QLatin1String("iphonesimulator"))
            {
                qCWarning(probeLog) << indent << QString::fromLatin1("Skipping unknown platform %1").arg(name);
                continue;
            }

            const QString platformSdkVersion = infoSettings.value(QLatin1String("Version")).toString();

            // prepare default platform properties
            QVariantMap defaultProp = infoSettings.value(QLatin1String("DefaultProperties"))
                    .toMap();
            QVariantMap overrideProp = infoSettings.value(QLatin1String("OverrideProperties"))
                    .toMap();
            QMapIterator<QString, QVariant> i(overrideProp);
            while (i.hasNext()) {
                i.next();
                // use unite? might lead to double insertions...
                defaultProp[i.key()] = i.value();
            }

            QString clangFullName = name + QLatin1String("-clang") + xcodeName;
            QString clang11FullName = name + QLatin1String("-clang11") + xcodeName;
            // detect gcc
            QFileInfo gccFileInfo(fInfo.absoluteFilePath() + QLatin1String("/Developer/usr/bin/g++"));
            QString gccFullName = name + QLatin1String("-gcc") + xcodeName;
            if (!gccFileInfo.exists())
                gccFileInfo = QFileInfo(devPath + QLatin1String("/usr/bin/g++"));
            bool hasGcc = gccFileInfo.exists();

            QStringList extraFlags;
            if (defaultProp.contains(QLatin1String("NATIVE_ARCH"))) {
                QString arch = defaultProp.value(QLatin1String("NATIVE_ARCH")).toString();
                if (!arch.startsWith(QLatin1String("arm")))
                    qCWarning(probeLog) << indent << QString::fromLatin1("Expected arm architecture, not %1").arg(arch);
                extraFlags << QLatin1String("-arch") << arch;
            } else if (name == QLatin1String("iphonesimulator")) {
                // don't generate a toolchain for 64 bit (to fix when we support that)
                extraFlags << QLatin1String("-arch") << QLatin1String("i386");
            }
            if (hasClang) {
                Platform clangProfile;
                clangProfile.developerPath = Utils::FileName::fromString(devPath);
                clangProfile.platformKind = 0;
                clangProfile.name = clangFullName;
                clangProfile.platformPath = Utils::FileName(fInfo);
                clangProfile.compilerPath = Utils::FileName(clangFileInfo);
                QStringList flags = extraFlags;
                flags << QLatin1String("-dumpmachine");
                QString compilerTriplet = qsystem(clangFileInfo.canonicalFilePath(), flags)
                        .simplified();
                QStringList compilerTripletl = compilerTriplet.split(QLatin1Char('-'));
                clangProfile.architecture = compilerTripletl.value(0);
                clangProfile.backendFlags = extraFlags;
                qCDebug(probeLog) << indent << QString::fromLatin1("* adding profile %1").arg(clangProfile.name);
                m_platforms[clangProfile.name] = clangProfile;
                clangProfile.platformKind |= Platform::Cxx11Support;
                clangProfile.backendFlags.append(QLatin1String("-std=c++11"));
                clangProfile.backendFlags.append(QLatin1String("-stdlib=libc++"));
                clangProfile.name = clang11FullName;
                m_platforms[clangProfile.name] = clangProfile;
            }
            if (hasGcc) {
                Platform gccProfile;
                gccProfile.developerPath = Utils::FileName::fromString(devPath);
                gccProfile.name = gccFullName;
                gccProfile.platformKind = 0;
                // use the arm-apple-darwin10-llvm-* variant and avoid the extraFlags if available???
                gccProfile.platformPath = Utils::FileName(fInfo);
                gccProfile.compilerPath = Utils::FileName(gccFileInfo);
                QStringList flags = extraFlags;
                flags << QLatin1String("-dumpmachine");
                QString compilerTriplet = qsystem(gccFileInfo.canonicalFilePath(), flags)
                        .simplified();
                QStringList compilerTripletl = compilerTriplet.split(QLatin1Char('-'));
                gccProfile.architecture = compilerTripletl.value(0);
                gccProfile.backendFlags = extraFlags;
                qCDebug(probeLog) << indent << QString::fromLatin1("* adding profile %1").arg(gccProfile.name);
                m_platforms[gccProfile.name] = gccProfile;
            }

            // set SDKs/sysroot
            QString sysRoot;
            {
                QString sdkName;
                if (defaultProp.contains(QLatin1String("SDKROOT")))
                    sdkName = defaultProp.value(QLatin1String("SDKROOT")).toString();
                QString sdkPath;
                QString sdkPathWithSameVersion;
                QDir sdks(fInfo.absoluteFilePath() + QLatin1String("/Developer/SDKs"));
                QString maxVersion;
                foreach (const QFileInfo &sdkDirInfo, sdks.entryInfoList(QDir::Dirs
                                                                         | QDir::NoDotAndDotDot)) {
                    indent = QLatin1String("    ");
                    QSettings sdkInfo(sdkDirInfo.absoluteFilePath()
                                      + QLatin1String("/SDKSettings.plist"),
                                      QSettings::NativeFormat);
                    QString versionStr = sdkInfo.value(QLatin1String("Version")).toString();
                    QVariant currentSdkName = sdkInfo.value(QLatin1String("CanonicalName"));
                    bool isBaseSdk = sdkInfo.value((QLatin1String("isBaseSDK"))).toString()
                            .toLower() != QLatin1String("no");
                    if (!isBaseSdk) {
                        qCDebug(probeLog) << indent << QString::fromLatin1("Skipping non base Sdk %1")
                                                .arg(currentSdkName.toString());
                        continue;
                    }
                    if (sdkName.isEmpty()) {
                        if (maxVersion.isEmpty() || compareVersions(maxVersion, versionStr) > 0) {
                            maxVersion = versionStr;
                            sdkPath = sdkDirInfo.canonicalFilePath();
                        }
                    } else if (currentSdkName == sdkName) {
                        sdkPath = sdkDirInfo.canonicalFilePath();
                    } else if (currentSdkName.toString().startsWith(sdkName) /*if sdkName doesn't contain version*/
                            && compareVersions(platformSdkVersion, versionStr) == 0)
                        sdkPathWithSameVersion = sdkDirInfo.canonicalFilePath();
                }
                if (sdkPath.isEmpty())
                    sysRoot = sdkPathWithSameVersion;
                else
                    sysRoot = sdkPath;
                if (sysRoot.isEmpty() && !sdkName.isEmpty())
                    qCDebug(probeLog) << indent << QString::fromLatin1("Failed to find sysroot %1").arg(sdkName);
            }
            if (hasClang && !sysRoot.isEmpty()) {
                m_platforms[clangFullName].platformKind |= Platform::BasePlatform;
                m_platforms[clangFullName].sdkPath = Utils::FileName::fromString(sysRoot);
                m_platforms[clang11FullName].platformKind |= Platform::BasePlatform;
                m_platforms[clang11FullName].sdkPath = Utils::FileName::fromString(sysRoot);
            }
            if (hasGcc && !sysRoot.isEmpty()) {
                m_platforms[gccFullName].platformKind |= Platform::BasePlatform;
                m_platforms[gccFullName].sdkPath = Utils::FileName::fromString(sysRoot);
            }
        }
        indent = QLatin1String("  ");
    }
}

void IosProbe::detectFirst()
{
    detectDeveloperPaths();
    if (!m_developerPaths.isEmpty())
        setupDefaultToolchains(m_developerPaths.value(0),QLatin1String(""));
}

QMap<QString, Platform> IosProbe::detectedPlatforms()
{
    return m_platforms;
}

}
