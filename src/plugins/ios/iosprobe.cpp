/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "iosprobe.h"

#include <utils/synchronousprocess.h>

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
    QString program = QLatin1String("/usr/bin/xcode-select");
    QStringList arguments(QLatin1String("--print-path"));

    Utils::SynchronousProcess selectedXcode;
    selectedXcode.setTimeoutS(5);
    Utils::SynchronousProcessResponse response = selectedXcode.run(program, arguments);
    if (response.result != Utils::SynchronousProcessResponse::Finished) {
        qCWarning(probeLog) << QString::fromLatin1("Could not detect selected xcode with /usr/bin/xcode-select");
    } else {
        QString path = response.stdOut();
        path.chop(1);
        addDeveloperPath(path);
    }
    addDeveloperPath(QLatin1String("/Applications/Xcode.app/Contents/Developer"));
}

void IosProbe::setupDefaultToolchains(const QString &devPath, const QString &xcodeName)
{
    qCDebug(probeLog) << QString::fromLatin1("Setting up platform \"%1\".").arg(xcodeName);
    QString indent = QLatin1String("  ");

    auto getClangInfo = [devPath, indent](const QString &compiler) {
        QFileInfo compilerInfo(devPath
                                + QLatin1String("/Toolchains/XcodeDefault.xctoolchain/usr/bin/")
                                + compiler);
        if (!compilerInfo.exists())
            qCWarning(probeLog) << indent << QString::fromLatin1("Default toolchain %1 not found.")
                                    .arg(compilerInfo.canonicalFilePath());
        return compilerInfo;
    };

    // detect clang (default toolchain)
    const QFileInfo clangCppInfo = getClangInfo("clang++");
    const bool hasClangCpp = clangCppInfo.exists();

    const QFileInfo clangCInfo = getClangInfo("clang");
    const bool hasClangC = clangCInfo.exists();

    // Platforms
    const QDir platformsDir(devPath + QLatin1String("/Platforms"));
    const QFileInfoList platforms = platformsDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
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
                qCDebug(probeLog) << indent << QString::fromLatin1("Skipping unknown platform %1").arg(name);
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

            const QString clangFullName = name + QLatin1String("-clang") + xcodeName;
            const QString clang11FullName = name + QLatin1String("-clang11") + xcodeName;
            // detect gcc
            QFileInfo gccCppInfo(fInfo.absoluteFilePath() + QLatin1String("/Developer/usr/bin/g++"));
            if (!gccCppInfo.exists())
                gccCppInfo = QFileInfo(devPath + QLatin1String("/usr/bin/g++"));
            const bool hasGccCppCompiler = gccCppInfo.exists();

            QFileInfo gccCInfo(fInfo.absoluteFilePath() + QLatin1String("/Developer/usr/bin/gcc"));
            if (!gccCInfo.exists())
                gccCInfo = QFileInfo(devPath + QLatin1String("/usr/bin/gcc"));
            const bool hasGccCCompiler = gccCInfo.exists();

            const QString gccFullName = name + QLatin1String("-gcc") + xcodeName;

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

            auto getArch = [extraFlags](const QFileInfo &compiler) {
                QStringList flags = extraFlags;
                flags << QLatin1String("-dumpmachine");
                const QStringList compilerTriplet = qsystem(compiler.canonicalFilePath(), flags)
                        .simplified().split(QLatin1Char('-'));
                return compilerTriplet.value(0);
            };

            if (hasClangCpp || hasClangC) {
                Platform clangProfile;
                clangProfile.type = Platform::CLang;
                clangProfile.developerPath = Utils::FileName::fromString(devPath);
                clangProfile.platformKind = 0;
                clangProfile.platformPath = Utils::FileName(fInfo);
                clangProfile.architecture = getArch(hasClangCpp ? clangCppInfo : clangCInfo);
                clangProfile.backendFlags = extraFlags;
                qCDebug(probeLog) << indent << QString::fromLatin1("* adding profile %1").arg(clangProfile.name);
                clangProfile.name = clangFullName;
                if (hasClangC) {
                    clangProfile.cCompilerPath = Utils::FileName(clangCInfo);
                    m_platforms[clangFullName] = clangProfile;
                }
                if (hasClangCpp) {
                    clangProfile.cxxCompilerPath = Utils::FileName(clangCppInfo);
                    m_platforms[clangFullName] = clangProfile;
                    clangProfile.platformKind |= Platform::Cxx11Support;
                    clangProfile.backendFlags.append(QLatin1String("-std=c++11"));
                    clangProfile.backendFlags.append(QLatin1String("-stdlib=libc++"));
                    clangProfile.name = clang11FullName;
                    m_platforms[clang11FullName] = clangProfile;
                }
            }

            if (hasGccCppCompiler || hasGccCCompiler) {
                Platform gccProfile;
                gccProfile.type = Platform::GCC;
                gccProfile.developerPath = Utils::FileName::fromString(devPath);
                gccProfile.name = gccFullName;
                gccProfile.platformKind = 0;
                // use the arm-apple-darwin10-llvm-* variant and avoid the extraFlags if available???
                gccProfile.platformPath = Utils::FileName(fInfo);
                gccProfile.architecture = getArch(hasGccCppCompiler ? gccCppInfo : gccCInfo);
                gccProfile.backendFlags = extraFlags;
                qCDebug(probeLog) << indent << QString::fromLatin1("* adding profile %1").arg(gccProfile.name);
                if (hasGccCppCompiler) {
                    gccProfile.cxxCompilerPath = Utils::FileName(gccCppInfo);
                }
                if (hasGccCCompiler) {
                    gccProfile.cCompilerPath = Utils::FileName(gccCInfo);
                }
                m_platforms[gccFullName] = gccProfile;
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

            if (!sysRoot.isEmpty()) {
                auto itr = m_platforms.begin();
                while (itr != m_platforms.end()) {
                    if (itr.key() == clangFullName ||
                            itr.key() == clang11FullName ||
                            itr.key() == gccFullName) {
                        itr.value().platformKind |= Platform::BasePlatform;
                        itr.value().sdkPath = Utils::FileName::fromString(sysRoot);
                    }
                    ++itr;
                }
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

QDebug operator<<(QDebug debug, const Platform &platform)
{
    QDebugStateSaver saver(debug); Q_UNUSED(saver)
    debug.nospace() << "(name=" << platform.name
                    << ", C++ compiler=" << platform.cxxCompilerPath.toString()
                    << ", C compiler=" << platform.cCompilerPath.toString()
                    << ", flags=" << platform.backendFlags
                    << ")";
    return debug;
}

bool Platform::operator==(const Platform &other) const
{
    return name == other.name;
}

uint qHash(const Platform &platform)
{
    return qHash(platform.name);
}

}
