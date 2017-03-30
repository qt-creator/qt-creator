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
#include "androidsdkmanager.h"

#include "androidmanager.h"
#include "androidtoolmanager.h"

#include "utils/algorithm.h"
#include "utils/qtcassert.h"
#include "utils/synchronousprocess.h"
#include "utils/environment.h"

#include <QLoggingCategory>
#include <QSettings>

namespace {
Q_LOGGING_CATEGORY(sdkManagerLog, "qtc.android.sdkManager")
}

namespace Android {
namespace Internal {

// Though sdk manager is introduced in 25.2.3 but the verbose mode is avaialble in 25.3.0
// and android tool is supported in 25.2.3
const QVersionNumber sdkManagerIntroVersion(25, 3 ,0);

const char installLocationKey[] = "Installed Location:";
const char apiLevelPropertyKey[] = "AndroidVersion.ApiLevel";
const char abiPropertyKey[] = "SystemImage.Abi";

using namespace Utils;

/*!
    Parses the \a line for a [spaces]key[spaces]value[spaces] pattern and returns
    \c true if \a key is found, false otherwise. Result is copied into \a value.
 */
static bool valueForKey(QString key, const QString &line, QString *value = nullptr)
{
    auto trimmedInput = line.trimmed();
    if (trimmedInput.startsWith(key)) {
        if (value)
            *value = trimmedInput.section(key, 1, 1).trimmed();
        return true;
    }
    return false;
}

/*!
    Runs the \c sdkmanger tool specific to configuration \a config with arguments \a args. Returns
    \c true if the command is successfully executed. Output is copied into \a output. The function
    blocks the calling thread.
 */
static bool sdkManagerCommand(const AndroidConfig config, const QStringList &args, QString *output)
{
    QString sdkManagerToolPath = config.sdkManagerToolPath().toString();
    SynchronousProcess proc;
    SynchronousProcessResponse response = proc.runBlocking(sdkManagerToolPath, args);
    if (response.result == SynchronousProcessResponse::Finished) {
        if (output)
            *output = response.allOutput();
        return true;
    }
    return false;
}

/*!
    \class SdkManagerOutputParser
    \brief The SdkManagerOutputParser class is a helper class to parse the output of the \c sdkmanager
    commands.
 */
class SdkManagerOutputParser
{
public:
    enum MarkerTag
    {
        None                        = 0x01,
        InstalledPackagesMarker     = 0x02,
        AvailablePackagesMarkers    = 0x04,
        AvailableUpdatesMarker      = 0x08,
        EmptyMarker                 = 0x10,
        PlatformMarker              = 0x20,
        SystemImageMarker           = 0x40,
        SectionMarkers = InstalledPackagesMarker | AvailablePackagesMarkers | AvailableUpdatesMarker
    };

    void parsePackageListing(const QString &output);

    SdkPlatformList m_installedPlatforms;

private:
    void compileData();
    void parsePackageData(MarkerTag packageMarker, const QStringList &data);
    bool parsePlatform(const QStringList &data, SdkPlatform *platform) const;
    bool parseSystemImage(const QStringList &data, SystemImage *image);
    MarkerTag parseMarkers(const QString &line);

    MarkerTag m_currentSection = MarkerTag::None;
    SystemImageList m_installedSystemImages;
};

const std::map<SdkManagerOutputParser::MarkerTag, const char *> markerTags {
    {SdkManagerOutputParser::MarkerTag::InstalledPackagesMarker,   "Installed packages:"},
    {SdkManagerOutputParser::MarkerTag::AvailablePackagesMarkers,  "Available Packages:"},
    {SdkManagerOutputParser::MarkerTag::AvailablePackagesMarkers,  "Available Updates:"},
    {SdkManagerOutputParser::MarkerTag::PlatformMarker,            "platforms"},
    {SdkManagerOutputParser::MarkerTag::SystemImageMarker,         "system-images"}
};

AndroidSdkManager::AndroidSdkManager(const AndroidConfig &config):
    m_config(config),
    m_parser(new SdkManagerOutputParser)
{
    QString packageListing;
    if (sdkManagerCommand(config, QStringList({"--list", "--verbose"}), &packageListing)) {
        m_parser->parsePackageListing(packageListing);
    }
}

AndroidSdkManager::~AndroidSdkManager()
{

}

SdkPlatformList AndroidSdkManager::availableSdkPlatforms()
{
    if (m_config.sdkToolsVersion() < sdkManagerIntroVersion) {
        AndroidToolManager toolManager(m_config);
        return toolManager.availableSdkPlatforms();
    }

    return m_parser->m_installedPlatforms;
}

void SdkManagerOutputParser::parsePackageListing(const QString &output)
{
    QStringList packageData;
    bool collectingPackageData = false;
    MarkerTag currentPackageMarker = MarkerTag::None;

    auto processCurrentPackage = [&]() {
        if (collectingPackageData) {
            collectingPackageData = false;
            parsePackageData(currentPackageMarker, packageData);
            packageData.clear();
        }
    };

    foreach (QString outputLine, output.split('\n')) {
        MarkerTag marker = parseMarkers(outputLine);

        if (marker & SectionMarkers) {
            // Section marker found. Update the current section being parsed.
            m_currentSection = marker;
            processCurrentPackage();
            continue;
        }

        if (m_currentSection == None
                || m_currentSection == AvailablePackagesMarkers  // At this point. Not interested in
                || m_currentSection == AvailableUpdatesMarker) { // available or update packages.
            // Let go of verbose output utill a valid section starts.
            continue;
        }

        if (marker == EmptyMarker) {
            // Empty marker. Occurs at the end of a package details.
            // Process the collected package data, if any.
            processCurrentPackage();
            continue;
        }

        if (marker == None) {
            if (collectingPackageData)
                packageData << outputLine; // Collect data until next marker.
            else
                continue;
        } else {
            // Package marker found.
            processCurrentPackage(); // New package starts. Process the collected package data, if any.
            currentPackageMarker = marker;
            collectingPackageData = true;
            packageData << outputLine;
        }
    }
    compileData();
    Utils::sort(m_installedPlatforms);
}

void SdkManagerOutputParser::compileData()
{
    // Associate the system images with sdk platforms.
    for (auto &image : m_installedSystemImages) {
        auto findPlatfom = [image](const SdkPlatform &platform) {
            return platform.apiLevel == image.apiLevel;
        };
        auto itr = std::find_if(m_installedPlatforms.begin(), m_installedPlatforms.end(), findPlatfom);
        if (itr != m_installedPlatforms.end())
            itr->systemImages.append(image);
    }
}

void SdkManagerOutputParser::parsePackageData(MarkerTag packageMarker, const QStringList &data)
{
    QTC_ASSERT(!data.isEmpty() && packageMarker != None, return);

    if (m_currentSection != MarkerTag::InstalledPackagesMarker)
        return; // For now, only interested in installed packages.

    switch (packageMarker) {
    case MarkerTag::PlatformMarker:
    {
        SdkPlatform platform;
        if (parsePlatform(data, &platform))
            m_installedPlatforms.append(platform);
        else
            qCDebug(sdkManagerLog) << "Platform: Parsing failed: " << data;
    }
        break;

    case MarkerTag::SystemImageMarker:
    {
        SystemImage image;
        if (parseSystemImage(data, &image))
            m_installedSystemImages.append(image);
        else
            qCDebug(sdkManagerLog) << "System Image: Parsing failed: " << data;
    }
        break;

    default:
        qCDebug(sdkManagerLog) << "Unhandled package: " << markerTags.at(packageMarker);
        break;
    }
}

bool SdkManagerOutputParser::parsePlatform(const QStringList &data, SdkPlatform *platform) const
{
    QTC_ASSERT(platform && !data.isEmpty(), return false);

    QStringList parts = data.at(0).split(';');
    if (parts.count() < 2) {
        qCDebug(sdkManagerLog) << "Platform: Unexpected header: "<< data;
        return false;
    }
    platform->name = parts[1];
    platform->package = data.at(0);

    foreach (QString line, data) {
        QString value;
        if (valueForKey(installLocationKey, line, &value))
            platform->installedLocation = Utils::FileName::fromString(value);
    }

    int apiLevel = AndroidManager::findApiLevel(platform->installedLocation);
    if (apiLevel != -1)
        platform->apiLevel = apiLevel;
    else
        qCDebug(sdkManagerLog) << "Platform: Can not parse api level: "<< data;

    return apiLevel != -1;
}

bool SdkManagerOutputParser::parseSystemImage(const QStringList &data, SystemImage *image)
{
    QTC_ASSERT(image && !data.isEmpty(), return false);

    QStringList parts = data.at(0).split(';');
    QTC_ASSERT(!data.isEmpty() && parts.count() >= 4,
               qCDebug(sdkManagerLog) << "System Image: Unexpected header: " << data);

    image->package = data.at(0);
    foreach (QString line, data) {
        QString value;
        if (valueForKey(installLocationKey, line, &value))
            image->installedLocation = Utils::FileName::fromString(value);
    }

    Utils::FileName propertiesPath = image->installedLocation;
    propertiesPath.appendPath("/source.properties");
    if (propertiesPath.exists()) {
        // Installed System Image.
        QSettings imageProperties(propertiesPath.toString(), QSettings::IniFormat);
        bool validApiLevel = false;
        image->apiLevel = imageProperties.value(apiLevelPropertyKey).toInt(&validApiLevel);
        if (!validApiLevel) {
            qCDebug(sdkManagerLog) << "System Image: Can not parse api level: "<< data;
            return false;
        }
        image->abiName = imageProperties.value(abiPropertyKey).toString();
    } else if (parts.count() >= 4){
        image->apiLevel = parts[1].section('-', 1, 1).toInt();
        image->abiName = parts[3];
    } else {
        qCDebug(sdkManagerLog) << "System Image: Can not parse: "<< data;
        return false;
    }

    return true;
}

SdkManagerOutputParser::MarkerTag SdkManagerOutputParser::parseMarkers(const QString &line)
{
    if (line.isEmpty())
        return EmptyMarker;

    for (auto pair: markerTags) {
        if (line.startsWith(QLatin1String(pair.second)))
            return pair.first;
    }

    return None;
}

} // namespace Internal
} // namespace Android
