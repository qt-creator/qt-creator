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
#include "androidtoolmanager.h"

#include "androidmanager.h"

#include "utils/algorithm.h"
#include "utils/environment.h"
#include "utils/qtcassert.h"
#include "utils/runextensions.h"
#include "utils/synchronousprocess.h"

#include <QLoggingCategory>

namespace {
Q_LOGGING_CATEGORY(androidToolLog, "qtc.android.sdkManager")
}

namespace Android {
namespace Internal {

using namespace Utils;

class AndroidToolOutputParser
{
public:
    void parseTargetListing(const QString &output, const FileName &sdkLocation,
                            SdkPlatformList *platformList);

    QList<SdkPlatform> m_installedPlatforms;
};

/*!
    Runs the \c android tool located at \a toolPath with arguments \a args and environment \a
    environment. Returns \c true for successful execution. Command's output is copied to \a
    output.
 */
static bool androidToolCommand(Utils::FileName toolPath, const QStringList &args,
                               const Environment &environment, QString *output)
{
    QString androidToolPath = toolPath.toString();
    SynchronousProcess proc;
    proc.setProcessEnvironment(environment.toProcessEnvironment());
    SynchronousProcessResponse response = proc.runBlocking(androidToolPath, args);
    if (response.result == SynchronousProcessResponse::Finished) {
        if (output)
            *output = response.allOutput();
        return true;
    }
    return false;
}

static QStringList cleanAndroidABIs(const QStringList &abis)
{
    QStringList res;
    foreach (const QString &abi, abis) {
        int index = abi.lastIndexOf(QLatin1Char('/'));
        if (index == -1)
            res << abi;
        else
            res << abi.mid(index + 1);
    }
    return res;
}

AndroidToolManager::AndroidToolManager(const AndroidConfig &config) :
    m_config(config),
    m_parser(new AndroidToolOutputParser)
{

}

AndroidToolManager::~AndroidToolManager()
{

}

SdkPlatformList AndroidToolManager::availableSdkPlatforms() const
{
    SdkPlatformList list;
    QString targetListing;
    if (androidToolCommand(m_config.androidToolPath(), QStringList({"list",  "target"}),
                           androidToolEnvironment(), &targetListing)) {
        m_parser->parseTargetListing(targetListing, m_config.sdkLocation(), &list);
    } else {
        qCDebug(androidToolLog) << "Android tool target listing failed";
    }
    return list;
}

void AndroidToolManager::launchAvdManager() const
{
    QProcess::startDetached(m_config.androidToolPath().toString(), QStringList("avd"));
}

QFuture<AndroidConfig::CreateAvdInfo>
AndroidToolManager::createAvd(AndroidConfig::CreateAvdInfo info) const
{
    return Utils::runAsync(&AndroidToolManager::createAvdImpl, info,
                           m_config.androidToolPath(), androidToolEnvironment());
}

bool AndroidToolManager::removeAvd(const QString &name) const
{
    SynchronousProcess proc;
    proc.setTimeoutS(5);
    proc.setProcessEnvironment(androidToolEnvironment().toProcessEnvironment());
    SynchronousProcessResponse response
            = proc.runBlocking(m_config.androidToolPath().toString(),
                               QStringList({"delete", "avd", "-n", name}));
    return response.result == SynchronousProcessResponse::Finished && response.exitCode == 0;
}

QFuture<AndroidDeviceInfoList> AndroidToolManager::androidVirtualDevicesFuture() const
{
    return Utils::runAsync(&AndroidToolManager::androidVirtualDevices,
                           m_config.androidToolPath(), m_config.sdkLocation(),
                           androidToolEnvironment());
}

Environment AndroidToolManager::androidToolEnvironment() const
{
    Environment env = Environment::systemEnvironment();
    Utils::FileName jdkLocation = m_config.openJDKLocation();
    if (!jdkLocation.isEmpty()) {
        env.set(QLatin1String("JAVA_HOME"), jdkLocation.toUserOutput());
        Utils::FileName binPath = jdkLocation;
        binPath.appendPath(QLatin1String("bin"));
        env.prependOrSetPath(binPath.toUserOutput());
    }
    return env;
}

AndroidConfig::CreateAvdInfo AndroidToolManager::createAvdImpl(AndroidConfig::CreateAvdInfo info,
                                                               FileName androidToolPath,
                                                               Environment env)
{
    QProcess proc;
    proc.setProcessEnvironment(env.toProcessEnvironment());
    QStringList arguments;
    arguments << QLatin1String("create") << QLatin1String("avd")
              << QLatin1String("-t") << info.target.name
              << QLatin1String("-n") << info.name
              << QLatin1String("-b") << info.abi;
    if (info.sdcardSize > 0)
        arguments << QLatin1String("-c") << QString::fromLatin1("%1M").arg(info.sdcardSize);
    proc.start(androidToolPath.toString(), arguments);
    if (!proc.waitForStarted()) {
        info.error = tr("Could not start process \"%1 %2\"")
                .arg(androidToolPath.toString(), arguments.join(QLatin1Char(' ')));
        return info;
    }
    QTC_CHECK(proc.state() == QProcess::Running);
    proc.write(QByteArray("yes\n")); // yes to "Do you wish to create a custom hardware profile"

    QByteArray question;
    while (true) {
        proc.waitForReadyRead(500);
        question += proc.readAllStandardOutput();
        if (question.endsWith(QByteArray("]:"))) {
            // truncate to last line
            int index = question.lastIndexOf(QByteArray("\n"));
            if (index != -1)
                question = question.mid(index);
            if (question.contains("hw.gpu.enabled"))
                proc.write(QByteArray("yes\n"));
            else
                proc.write(QByteArray("\n"));
            question.clear();
        }

        if (proc.state() != QProcess::Running)
            break;
    }
    QTC_CHECK(proc.state() == QProcess::NotRunning);

    QString errorOutput = QString::fromLocal8Bit(proc.readAllStandardError());
    // The exit code is always 0, so we need to check stderr
    // For now assume that any output at all indicates a error
    if (!errorOutput.isEmpty()) {
        info.error = errorOutput;
    }

    return info;
}

AndroidDeviceInfoList AndroidToolManager::androidVirtualDevices(const Utils::FileName &androidTool,
                                                                const FileName &sdkLocationPath,
                                                                const Environment &environment)
{
    AndroidDeviceInfoList devices;
    QString output;
    if (!androidToolCommand(androidTool, QStringList({"list", "avd"}), environment, &output))
        return devices;

    QStringList avds = output.split('\n');
    if (avds.empty())
        return devices;

    while (avds.first().startsWith(QLatin1String("* daemon")))
        avds.removeFirst(); // remove the daemon logs
    avds.removeFirst(); // remove "List of devices attached" header line

    bool nextLineIsTargetLine = false;

    AndroidDeviceInfo dev;
    for (int i = 0; i < avds.size(); i++) {
        QString line = avds.at(i);
        if (!line.contains(QLatin1String("Name:")))
            continue;

        int index = line.indexOf(QLatin1Char(':')) + 2;
        if (index >= line.size())
            break;
        dev.avdname = line.mid(index).trimmed();
        dev.sdk = -1;
        dev.cpuAbi.clear();
        ++i;
        for (; i < avds.size(); ++i) {
            line = avds.at(i);
            if (line.contains(QLatin1String("---------")))
                break;

            if (line.contains(QLatin1String("Target:")) || nextLineIsTargetLine) {
                if (line.contains(QLatin1String("Google APIs"))) {
                    nextLineIsTargetLine = true;
                    continue;
                }

                nextLineIsTargetLine = false;

                int lastIndex = line.lastIndexOf(QLatin1Char(' '));
                if (lastIndex == -1) // skip line
                    break;
                QString tmp = line.mid(lastIndex).remove(QLatin1Char(')')).trimmed();
                Utils::FileName platformPath = sdkLocationPath;
                platformPath.appendPath(QString("/platforms/android-%1").arg(tmp));
                dev.sdk = AndroidManager::findApiLevel(platformPath);
            }
            if (line.contains(QLatin1String("Tag/ABI:"))) {
                int lastIndex = line.lastIndexOf(QLatin1Char('/')) + 1;
                if (lastIndex >= line.size())
                    break;
                dev.cpuAbi = QStringList(line.mid(lastIndex));
            } else if (line.contains(QLatin1String("ABI:"))) {
                int lastIndex = line.lastIndexOf(QLatin1Char(' ')) + 1;
                if (lastIndex >= line.size())
                    break;
                dev.cpuAbi = QStringList(line.mid(lastIndex).trimmed());
            }
        }
        // armeabi-v7a devices can also run armeabi code
        if (dev.cpuAbi == QStringList("armeabi-v7a"))
            dev.cpuAbi << QLatin1String("armeabi");
        dev.state = AndroidDeviceInfo::OkState;
        dev.type = AndroidDeviceInfo::Emulator;
        if (dev.cpuAbi.isEmpty() || dev.sdk == -1)
            continue;
        devices.push_back(dev);
    }
    Utils::sort(devices);

    return devices;
}

void AndroidToolOutputParser::parseTargetListing(const QString &output,
                                                 const Utils::FileName &sdkLocation,
                                                 SdkPlatformList *platformList)
{
    if (!platformList)
        return;

    auto addSystemImage = [](const QStringList& abiList, SdkPlatform &platform) {
        foreach (auto imageAbi, abiList) {
            SystemImage image;
            image.abiName = imageAbi;
            image.apiLevel = platform.apiLevel;
            platform.systemImages.append(image);
        }
    };

    SdkPlatform platform;
    QStringList abiList;
    foreach (const QString &l, output.split('\n')) {
        const QString line = l.trimmed();
        if (line.startsWith(QLatin1String("id:")) && line.contains(QLatin1String("android-"))) {
            int index = line.indexOf(QLatin1String("\"android-"));
            if (index == -1)
                continue;
            QString androidTarget = line.mid(index + 1, line.length() - index - 2);
            const QString tmp = androidTarget.mid(androidTarget.lastIndexOf(QLatin1Char('-')) + 1);
            Utils::FileName platformPath = sdkLocation;
            platformPath.appendPath(QString("/platforms/android-%1").arg(tmp));
            platform.installedLocation = platformPath;
            platform.apiLevel = AndroidManager::findApiLevel(platformPath);
        } else if (line.startsWith(QLatin1String("Name:"))) {
            platform.name = line.mid(6);
        } else if (line.startsWith(QLatin1String("Tag/ABIs :"))) {
            abiList = cleanAndroidABIs(line.mid(10).trimmed().split(QLatin1String(", ")));
        } else if (line.startsWith(QLatin1String("ABIs"))) {
            abiList = cleanAndroidABIs(line.mid(6).trimmed().split(QLatin1String(", ")));
        } else if (line.startsWith(QLatin1String("---")) || line.startsWith(QLatin1String("==="))) {
            if (platform.apiLevel == -1)
                continue;

            addSystemImage(abiList, platform);
            *platformList << platform;

            platform = SdkPlatform();
            abiList.clear();
        }
    }

    // The last parsed Platform.
    if (platform.apiLevel != -1) {
        addSystemImage(abiList, platform);
        *platformList << platform;
    }

    Utils::sort(*platformList);
}

} // namespace Internal
} // namespace Android
