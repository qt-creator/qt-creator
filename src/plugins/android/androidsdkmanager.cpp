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
#include <QRegularExpression>
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
const char revisionKey[] = "Version:";
const char descriptionKey[] = "Description:";

const int sdkManagerCmdTimeoutS = 60;

using namespace Utils;

int platformNameToApiLevel(const QString &platformName)
{
    int apiLevel = -1;
    QRegularExpression re("(android-)(?<apiLevel>[0-9]{1,})",
                          QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(platformName);
    if (match.hasMatch()) {
        QString apiLevelStr = match.captured("apiLevel");
        apiLevel = apiLevelStr.toInt();
    }
    return apiLevel;
}

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
static bool sdkManagerCommand(const AndroidConfig &config, const QStringList &args, QString *output,
                              int timeout = sdkManagerCmdTimeoutS)
{
    QString sdkManagerToolPath = config.sdkManagerToolPath().toString();
    SynchronousProcess proc;
    proc.setTimeoutS(timeout);
    proc.setTimeOutMessageBoxEnabled(true);
    SynchronousProcessResponse response = proc.run(sdkManagerToolPath, args);
    if (output)
        *output = response.allOutput();
    return response.result == SynchronousProcessResponse::Finished;
}


class AndroidSdkManagerPrivate
{
public:
    AndroidSdkManagerPrivate(AndroidSdkManager &sdkManager, const AndroidConfig &config);
    ~AndroidSdkManagerPrivate();

    AndroidSdkPackageList filteredPackages(AndroidSdkPackage::PackageState state,
                                           AndroidSdkPackage::PackageType type,
                                           bool forceUpdate = false);
    const AndroidSdkPackageList &allPackages(bool forceUpdate = false);
    void refreshSdkPackages(bool forceReload = false);

private:
    void reloadSdkPackages();
    void clearPackages();

    AndroidSdkManager &m_sdkManager;
    const AndroidConfig &m_config;
    AndroidSdkPackageList m_allPackages;
    FileName lastSdkManagerPath;
};

/*!
    \class SdkManagerOutputParser
    \brief The SdkManagerOutputParser class is a helper class to parse the output of the \c sdkmanager
    commands.
 */
class SdkManagerOutputParser
{
    class GenericPackageData
    {
    public:
        bool isValid() const { return !revision.isNull() && !description.isNull(); }
        QStringList headerParts;
        QVersionNumber revision;
        QString description;
        Utils::FileName installedLocation;
        QMap<QString, QString> extraData;
    };

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

    SdkManagerOutputParser(AndroidSdkPackageList &container) : m_packages(container) {}
    void parsePackageListing(const QString &output);

    AndroidSdkPackageList &m_packages;

private:
    void compilePackageAssociations();
    void parsePackageData(MarkerTag packageMarker, const QStringList &data);
    bool parseAbstractData(GenericPackageData &output, const QStringList &input, int minParts,
                           const QString &logStrTag, QStringList extraKeys = QStringList()) const;
    AndroidSdkPackage *parsePlatform(const QStringList &data) const;
    QPair<SystemImage *, int> parseSystemImage(const QStringList &data) const;
    MarkerTag parseMarkers(const QString &line);

    MarkerTag m_currentSection = MarkerTag::None;
    QHash<AndroidSdkPackage *, int> m_systemImages;
};

const std::map<SdkManagerOutputParser::MarkerTag, const char *> markerTags {
    {SdkManagerOutputParser::MarkerTag::InstalledPackagesMarker,   "Installed packages:"},
    {SdkManagerOutputParser::MarkerTag::AvailablePackagesMarkers,  "Available Packages:"},
    {SdkManagerOutputParser::MarkerTag::AvailablePackagesMarkers,  "Available Updates:"},
    {SdkManagerOutputParser::MarkerTag::PlatformMarker,            "platforms"},
    {SdkManagerOutputParser::MarkerTag::SystemImageMarker,         "system-images"}
};

AndroidSdkManager::AndroidSdkManager(const AndroidConfig &config, QObject *parent):
    QObject(parent),
    m_d(new AndroidSdkManagerPrivate(*this, config))
{
}

AndroidSdkManager::~AndroidSdkManager()
{

}

SdkPlatformList AndroidSdkManager::installedSdkPlatforms()
{
    AndroidSdkPackageList list = m_d->filteredPackages(AndroidSdkPackage::Installed,
                                                       AndroidSdkPackage::SdkPlatformPackage);
    return Utils::transform(list, [](AndroidSdkPackage *p) {
       return static_cast<SdkPlatform *>(p);
    });
}

const AndroidSdkPackageList &AndroidSdkManager::allSdkPackages()
{
    return m_d->allPackages();
}

AndroidSdkPackageList AndroidSdkManager::availableSdkPackages()
{
    return m_d->filteredPackages(AndroidSdkPackage::Available, AndroidSdkPackage::AnyValidType);
}

AndroidSdkPackageList AndroidSdkManager::installedSdkPackages()
{
    return m_d->filteredPackages(AndroidSdkPackage::Installed, AndroidSdkPackage::AnyValidType);
}

SdkPlatform *AndroidSdkManager::latestAndroidSdkPlatform(AndroidSdkPackage::PackageState state)
{
    SdkPlatform *result = nullptr;
    const AndroidSdkPackageList list = m_d->filteredPackages(state,
                                                             AndroidSdkPackage::SdkPlatformPackage);
    for (AndroidSdkPackage *p : list) {
        auto platform = static_cast<SdkPlatform *>(p);
        if (!result || result->apiLevel() < platform->apiLevel())
            result = platform;
    }
    return result;
}

SdkPlatformList AndroidSdkManager::filteredSdkPlatforms(int minApiLevel,
                                                        AndroidSdkPackage::PackageState state)
{
    const AndroidSdkPackageList list = m_d->filteredPackages(state,
                                                             AndroidSdkPackage::SdkPlatformPackage);

    SdkPlatformList result;
    for (AndroidSdkPackage *p : list) {
        auto platform = static_cast<SdkPlatform *>(p);
        if (platform && platform->apiLevel() >= minApiLevel)
            result << platform;
    }
    return result;
}

void AndroidSdkManager::reloadPackages(bool forceReload)
{
    m_d->refreshSdkPackages(forceReload);
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

    QRegularExpression delimiters("[\\n\\r]");
    foreach (QString outputLine, output.split(delimiters)) {
        MarkerTag marker = parseMarkers(outputLine.trimmed());
        if (marker & SectionMarkers) {
            // Section marker found. Update the current section being parsed.
            m_currentSection = marker;
            processCurrentPackage();
            continue;
        }

        if (m_currentSection == None)
            continue; // Continue with the verbose output until a valid section starts.

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
    compilePackageAssociations();
}

void SdkManagerOutputParser::compilePackageAssociations()
{
    // Return true if package p is already installed i.e. there exists a installed package having
    // same sdk style path and same revision as of p.
    auto isInstalled = [](const AndroidSdkPackageList &container, AndroidSdkPackage *p) {
        return Utils::anyOf(container, [p](AndroidSdkPackage *other) {
            return other->state() == AndroidSdkPackage::Installed &&
                    other->sdkStylePath() == p->sdkStylePath() &&
                    other->revision() == p->revision();
        });
    };

    auto deleteAlreadyInstalled = [isInstalled](AndroidSdkPackageList &packages) {
        for (auto p = packages.begin(); p != packages.end();) {
            if ((*p)->state() == AndroidSdkPackage::Available && isInstalled(packages, *p)) {
                delete *p;
                p = packages.erase(p);
            } else {
                ++p;
            }
        }
    };

    // Remove already installed packages.
    deleteAlreadyInstalled(m_packages);

    // Filter out available images that are already installed.
    AndroidSdkPackageList images = m_systemImages.keys();
    deleteAlreadyInstalled(images);

    // Associate the system images with sdk platforms.
    for (AndroidSdkPackage *image : images) {
        int imageApi = m_systemImages[image];
        auto itr = std::find_if(m_packages.begin(), m_packages.end(),
                                [imageApi](const AndroidSdkPackage *p) {
            const SdkPlatform *platform = nullptr;
            if (p->type() == AndroidSdkPackage::SdkPlatformPackage)
                platform = static_cast<const SdkPlatform*>(p);
            return platform && platform->apiLevel() == imageApi;
        });
        if (itr != m_packages.end()) {
            SdkPlatform *platform = static_cast<SdkPlatform*>(*itr);
            platform->addSystemImage(static_cast<SystemImage *>(image));
        }
    }
}

void SdkManagerOutputParser::parsePackageData(MarkerTag packageMarker, const QStringList &data)
{
    QTC_ASSERT(!data.isEmpty() && packageMarker != None, return);

    AndroidSdkPackage *package = nullptr;
    auto createPackage = [&](std::function<AndroidSdkPackage *(SdkManagerOutputParser *,
                                                               const QStringList &)> creator) {
        if ((package = creator(this, data)))
            m_packages.append(package);
    };

    switch (packageMarker) {
    case MarkerTag::PlatformMarker:
        createPackage(&SdkManagerOutputParser::parsePlatform);
        break;

    case MarkerTag::SystemImageMarker:
    {
        QPair<SystemImage *, int> result = parseSystemImage(data);
        if (result.first) {
            m_systemImages[result.first] = result.second;
            package = result.first;
        }
    }
        break;

    default:
        qCDebug(sdkManagerLog) << "Unhandled package: " << markerTags.at(packageMarker);
        break;
    }

    if (package) {
        switch (m_currentSection) {
        case MarkerTag::InstalledPackagesMarker:
            package->setState(AndroidSdkPackage::Installed);
            break;
        case MarkerTag::AvailablePackagesMarkers:
        case MarkerTag::AvailableUpdatesMarker:
            package->setState(AndroidSdkPackage::Available);
            break;
        default:
            qCDebug(sdkManagerLog) << "Invalid section marker: " << markerTags.at(m_currentSection);
            break;
        }
    }
}

bool SdkManagerOutputParser::parseAbstractData(SdkManagerOutputParser::GenericPackageData &output,
                                               const QStringList &input, int minParts,
                                               const QString &logStrTag,
                                               QStringList extraKeys) const
{
    if (input.isEmpty()) {
        qCDebug(sdkManagerLog) << logStrTag + ": Empty input";
        return false;
    }

    output.headerParts = input.at(0).split(';');
    if (output.headerParts.count() < minParts) {
        qCDebug(sdkManagerLog) << logStrTag + "%1: Unexpected header:" << input;
        return false;
    }

    extraKeys << installLocationKey << revisionKey << descriptionKey;
    foreach (QString line, input) {
        QString value;
        for (auto key: extraKeys) {
            if (valueForKey(key, line, &value)) {
                if (key == installLocationKey)
                    output.installedLocation = Utils::FileName::fromString(value);
                else if (key == revisionKey)
                    output.revision = QVersionNumber::fromString(value);
                else if (key == descriptionKey)
                    output.description = value;
                else
                    output.extraData[key] = value;
                break;
            }
        }
    }

    return output.isValid();
}

AndroidSdkPackage *SdkManagerOutputParser::parsePlatform(const QStringList &data) const
{
    SdkPlatform *platform = nullptr;
    GenericPackageData packageData;
    if (parseAbstractData(packageData, data, 2, "Platform")) {
        int apiLevel = platformNameToApiLevel(packageData.headerParts.at(1));
        if (apiLevel == -1) {
            qCDebug(sdkManagerLog) << "Platform: Can not parse api level:"<< data;
            return nullptr;
        }
        platform = new SdkPlatform(packageData.revision, data.at(0), apiLevel);
        platform->setDescriptionText(packageData.description);
        platform->setInstalledLocation(packageData.installedLocation);
    } else {
        qCDebug(sdkManagerLog) << "Platform: Parsing failed. Minimum required data unavailable:"
                               << data;
    }
    return platform;
}

QPair<SystemImage *, int> SdkManagerOutputParser::parseSystemImage(const QStringList &data) const
{
    QPair <SystemImage *, int> result(nullptr, -1);
    GenericPackageData packageData;
    if (parseAbstractData(packageData, data, 4, "System-image")) {
        int apiLevel = platformNameToApiLevel(packageData.headerParts.at(1));
        if (apiLevel == -1) {
            qCDebug(sdkManagerLog) << "System-image: Can not parse api level:"<< data;
            return result;
        }
        auto image = new SystemImage(packageData.revision, data.at(0),
                                     packageData.headerParts.at(3));
        image->setInstalledLocation(packageData.installedLocation);
        image->setDisplayText(packageData.description);
        image->setDescriptionText(packageData.description);
        result = qMakePair(image, apiLevel);
    } else {
        qCDebug(sdkManagerLog) << "System-image: Minimum required data unavailable: "<< data;
    }
    return result;
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

AndroidSdkManagerPrivate::AndroidSdkManagerPrivate(AndroidSdkManager &sdkManager,
                                                   const AndroidConfig &config):
    m_sdkManager(sdkManager),
    m_config(config)
{

}

AndroidSdkManagerPrivate::~AndroidSdkManagerPrivate()
{
    clearPackages();
}

AndroidSdkPackageList
AndroidSdkManagerPrivate::filteredPackages(AndroidSdkPackage::PackageState state,
                                           AndroidSdkPackage::PackageType type, bool forceUpdate)
{
    refreshSdkPackages(forceUpdate);
    return Utils::filtered(m_allPackages, [state, type](const AndroidSdkPackage *p) {
       return p->state() & state && p->type() & type;
    });
}

const AndroidSdkPackageList &AndroidSdkManagerPrivate::allPackages(bool forceUpdate)
{
    refreshSdkPackages(forceUpdate);
    return m_allPackages;
}

void AndroidSdkManagerPrivate::reloadSdkPackages()
{
    m_sdkManager.packageReloadBegin();
    clearPackages();

    lastSdkManagerPath = m_config.sdkManagerToolPath();

    if (m_config.sdkToolsVersion().isNull()) {
        // Configuration has invalid sdk path or corrupt installation.
        m_sdkManager.packageReloadFinished();
        return;
    }

    if (m_config.sdkToolsVersion() < sdkManagerIntroVersion) {
        // Old Sdk tools.
        AndroidToolManager toolManager(m_config);
        auto toAndroidSdkPackages = [](SdkPlatform *p) -> AndroidSdkPackage *{
            return p;
        };
        m_allPackages = Utils::transform(toolManager.availableSdkPlatforms(), toAndroidSdkPackages);
    } else {
        QString packageListing;
        if (sdkManagerCommand(m_config, QStringList({"--list", "--verbose"}), &packageListing)) {
            SdkManagerOutputParser parser(m_allPackages);
            parser.parsePackageListing(packageListing);
        }
    }
    m_sdkManager.packageReloadFinished();
}

void AndroidSdkManagerPrivate::refreshSdkPackages(bool forceReload)
{
    // Sdk path changed. Updated packages.
    // QTC updates the package listing only
    if (m_config.sdkManagerToolPath() != lastSdkManagerPath || forceReload)
        reloadSdkPackages();
}

void AndroidSdkManagerPrivate::clearPackages()
{
    for (AndroidSdkPackage *p : m_allPackages)
        delete p;
    m_allPackages.clear();
}

} // namespace Internal
} // namespace Android
