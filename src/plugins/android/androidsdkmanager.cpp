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

#include "androidconstants.h"
#include "androidmanager.h"
#include "androidtoolmanager.h"

#include "utils/algorithm.h"
#include "utils/qtcassert.h"
#include "utils/runextensions.h"
#include "utils/synchronousprocess.h"
#include "utils/qtcprocess.h"

#include <QFutureWatcher>
#include <QLoggingCategory>
#include <QReadWriteLock>
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
const char commonArgsKey[] = "Common Arguments:";

const int sdkManagerCmdTimeoutS = 60;
const int sdkManagerOperationTimeoutS = 600;

const QRegularExpression assertionReg("(\\(\\s*y\\s*[\\/\\\\]\\s*n\\s*\\)\\s*)(?<mark>[\\:\\?])",
                                      QRegularExpression::CaseInsensitiveOption |
                                      QRegularExpression::MultilineOption);

using namespace Utils;
using SdkCmdFutureInterface = QFutureInterface<AndroidSdkManager::OperationOutput>;

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

int parseProgress(const QString &out, bool &foundAssertion)
{
    int progress = -1;
    if (out.isEmpty())
        return progress;
    QRegularExpression reg("(?<progress>\\d*)%");
    QStringList lines = out.split(QRegularExpression("[\\n\\r]"), QString::SkipEmptyParts);
    for (const QString &line : lines) {
        QRegularExpressionMatch match = reg.match(line);
        if (match.hasMatch()) {
            progress = match.captured("progress").toInt();
            if (progress < 0 || progress > 100)
                progress = -1;
        }
        if (!foundAssertion)
            foundAssertion = assertionReg.match(line).hasMatch();
    }
    return progress;
}

void watcherDeleter(QFutureWatcher<void> *watcher)
{
    if (!watcher->isFinished() && !watcher->isCanceled())
        watcher->cancel();

    if (!watcher->isFinished())
        watcher->waitForFinished();

    delete watcher;
}

/*!
    Runs the \c sdkmanger tool with arguments \a args. Returns \c true if the command is
    successfully executed. Output is copied into \a output. The function blocks the calling thread.
 */
static bool sdkManagerCommand(const AndroidConfig &config, const QStringList &args,
                              QString *output, int timeout = sdkManagerCmdTimeoutS)
{
    SynchronousProcess proc;
    proc.setProcessEnvironment(AndroidConfigurations::toolsEnvironment(config));
    proc.setTimeoutS(timeout);
    proc.setTimeOutMessageBoxEnabled(true);
    SynchronousProcessResponse response = proc.run(config.sdkManagerToolPath().toString(), args);
    if (output)
        *output = response.allOutput();
    return response.result == SynchronousProcessResponse::Finished;
}

/*!
    Runs the \c sdkmanger tool with arguments \a args. The operation command progress is updated in
    to the future interface \a fi and \a output is populated with command output. The command listens
    to cancel signal emmitted by \a sdkManager and kill the commands. The command is also killed
    after the lapse of \a timeout seconds. The function blocks the calling thread.
 */
static void sdkManagerCommand(const AndroidConfig &config, const QStringList &args,
                              AndroidSdkManager &sdkManager, SdkCmdFutureInterface &fi,
                              AndroidSdkManager::OperationOutput &output, double progressQuota,
                              bool interruptible = true, int timeout = sdkManagerOperationTimeoutS)
{
    int offset = fi.progressValue();
    SynchronousProcess proc;
    proc.setProcessEnvironment(AndroidConfigurations::toolsEnvironment(config));
    bool assertionFound = false;
    proc.setStdErrBufferedSignalsEnabled(true);
    proc.setStdOutBufferedSignalsEnabled(true);
    proc.setTimeoutS(timeout);
    QObject::connect(&proc, &SynchronousProcess::stdOutBuffered,
                     [offset, progressQuota, &proc, &assertionFound, &fi](const QString &out) {
        int progressPercent = parseProgress(out, assertionFound);
        if (assertionFound)
            proc.terminate();
        if (progressPercent != -1)
            fi.setProgressValue(offset + qRound((progressPercent / 100.0) * progressQuota));
    });
    QObject::connect(&proc, &SynchronousProcess::stdErrBuffered, [&output](const QString &err) {
        output.stdError = err;
    });
    if (interruptible) {
        QObject::connect(&sdkManager, &AndroidSdkManager::cancelActiveOperations,
                         &proc, &SynchronousProcess::terminate);
    }
    SynchronousProcessResponse response = proc.run(config.sdkManagerToolPath().toString(), args);
    if (assertionFound) {
        output.success = false;
        output.stdOutput = response.stdOut();
        output.stdError = QCoreApplication::translate("Android::Internal::AndroidSdkManager",
                                                      "The operation requires user interaction. "
                                                      "Use the \"sdkmanager\" command-line tool.");
    } else {
        output.success = response.result == SynchronousProcessResponse::Finished;
    }
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

    void parseCommonArguments(QFutureInterface<QString> &fi);
    void updateInstalled(SdkCmdFutureInterface &fi);
    void update(SdkCmdFutureInterface &fi, const QStringList &install,
                const QStringList &uninstall);
    void checkPendingLicense(SdkCmdFutureInterface &fi);
    void getPendingLicense(SdkCmdFutureInterface &fi);

    void addWatcher(const QFuture<AndroidSdkManager::OperationOutput> &future);
    void setLicenseInput(bool acceptLicense);

    std::unique_ptr<QFutureWatcher<void>, decltype(&watcherDeleter)> m_activeOperation;

private:
    QByteArray getUserInput() const;
    void clearUserInput();
    void reloadSdkPackages();
    void clearPackages();
    bool onLicenseStdOut(const QString &output, bool notify,
                         AndroidSdkManager::OperationOutput &result, SdkCmdFutureInterface &fi);

    AndroidSdkManager &m_sdkManager;
    const AndroidConfig &m_config;
    AndroidSdkPackageList m_allPackages;
    FileName lastSdkManagerPath;
    QString m_licenseTextCache;
    QByteArray m_licenseUserInput;
    mutable QReadWriteLock m_licenseInputLock;
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
        None                        = 0x001,
        InstalledPackagesMarker     = 0x002,
        AvailablePackagesMarkers    = 0x004,
        AvailableUpdatesMarker      = 0x008,
        EmptyMarker                 = 0x010,
        PlatformMarker              = 0x020,
        SystemImageMarker           = 0x040,
        BuildToolsMarker            = 0x080,
        SdkToolsMarker              = 0x100,
        PlatformToolsMarker         = 0x200,
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
    BuildTools *parseBuildToolsPackage(const QStringList &data) const;
    SdkTools *parseSdkToolsPackage(const QStringList &data) const;
    PlatformTools *parsePlatformToolsPackage(const QStringList &data) const;
    MarkerTag parseMarkers(const QString &line);

    MarkerTag m_currentSection = MarkerTag::None;
    QHash<AndroidSdkPackage *, int> m_systemImages;
};

const std::map<SdkManagerOutputParser::MarkerTag, const char *> markerTags {
    {SdkManagerOutputParser::MarkerTag::InstalledPackagesMarker,    "Installed packages:"},
    {SdkManagerOutputParser::MarkerTag::AvailablePackagesMarkers,   "Available Packages:"},
    {SdkManagerOutputParser::MarkerTag::AvailablePackagesMarkers,   "Available Updates:"},
    {SdkManagerOutputParser::MarkerTag::PlatformMarker,             "platforms"},
    {SdkManagerOutputParser::MarkerTag::SystemImageMarker,          "system-images"},
    {SdkManagerOutputParser::MarkerTag::BuildToolsMarker,           "build-tools"},
    {SdkManagerOutputParser::MarkerTag::SdkToolsMarker,             "tools"},
    {SdkManagerOutputParser::MarkerTag::PlatformToolsMarker,        "platform-tools"}
};

AndroidSdkManager::AndroidSdkManager(const AndroidConfig &config, QObject *parent):
    QObject(parent),
    m_d(new AndroidSdkManagerPrivate(*this, config))
{
}

AndroidSdkManager::~AndroidSdkManager()
{
    cancelOperatons();
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

bool AndroidSdkManager::isBusy() const
{
    return m_d->m_activeOperation && !m_d->m_activeOperation->isFinished();
}

QFuture<QString> AndroidSdkManager::availableArguments() const
{
    return Utils::runAsync(&AndroidSdkManagerPrivate::parseCommonArguments, m_d.get());
}

QFuture<AndroidSdkManager::OperationOutput> AndroidSdkManager::updateAll()
{
    if (isBusy()) {
        return QFuture<AndroidSdkManager::OperationOutput>();
    }
    auto future = Utils::runAsync(&AndroidSdkManagerPrivate::updateInstalled, m_d.get());
    m_d->addWatcher(future);
    return future;
}

QFuture<AndroidSdkManager::OperationOutput>
AndroidSdkManager::update(const QStringList &install, const QStringList &uninstall)
{
    if (isBusy())
        return QFuture<AndroidSdkManager::OperationOutput>();
    auto future = Utils::runAsync(&AndroidSdkManagerPrivate::update, m_d.get(), install, uninstall);
    m_d->addWatcher(future);
    return future;
}

QFuture<AndroidSdkManager::OperationOutput> AndroidSdkManager::checkPendingLicenses()
{
    if (isBusy())
        return QFuture<AndroidSdkManager::OperationOutput>();
    auto future = Utils::runAsync(&AndroidSdkManagerPrivate::checkPendingLicense, m_d.get());
    m_d->addWatcher(future);
    return future;
}

QFuture<AndroidSdkManager::OperationOutput> AndroidSdkManager::runLicenseCommand()
{
    if (isBusy())
        return QFuture<AndroidSdkManager::OperationOutput>();
    auto future = Utils::runAsync(&AndroidSdkManagerPrivate::getPendingLicense, m_d.get());
    m_d->addWatcher(future);
    return future;
}

void AndroidSdkManager::cancelOperatons()
{
    emit cancelActiveOperations();
    m_d->m_activeOperation.reset();
}

void AndroidSdkManager::acceptSdkLicense(bool accept)
{
    m_d->setLicenseInput(accept);
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
    case MarkerTag::BuildToolsMarker:
        createPackage(&SdkManagerOutputParser::parseBuildToolsPackage);
        break;

    case MarkerTag::SdkToolsMarker:
        createPackage(&SdkManagerOutputParser::parseSdkToolsPackage);
        break;

    case MarkerTag::PlatformToolsMarker:
        createPackage(&SdkManagerOutputParser::parsePlatformToolsPackage);
        break;

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

BuildTools *SdkManagerOutputParser::parseBuildToolsPackage(const QStringList &data) const
{
    BuildTools *buildTools = nullptr;
    GenericPackageData packageData;
    if (parseAbstractData(packageData, data, 2, "Build-tools")) {
        buildTools = new BuildTools(packageData.revision, data.at(0));
        buildTools->setDescriptionText(packageData.description);
        buildTools->setDisplayText(packageData.description);
        buildTools->setInstalledLocation(packageData.installedLocation);
    } else {
        qCDebug(sdkManagerLog) << "Build-tools: Parsing failed. Minimum required data unavailable:"
                               << data;
    }
    return buildTools;
}

SdkTools *SdkManagerOutputParser::parseSdkToolsPackage(const QStringList &data) const
{
    SdkTools *sdkTools = nullptr;
    GenericPackageData packageData;
    if (parseAbstractData(packageData, data, 1, "SDK-tools")) {
        sdkTools = new SdkTools(packageData.revision, data.at(0));
        sdkTools->setDescriptionText(packageData.description);
        sdkTools->setDisplayText(packageData.description);
        sdkTools->setInstalledLocation(packageData.installedLocation);
    } else {
        qCDebug(sdkManagerLog) << "SDK-tools: Parsing failed. Minimum required data unavailable:"
                               << data;
    }
    return sdkTools;
}

PlatformTools *SdkManagerOutputParser::parsePlatformToolsPackage(const QStringList &data) const
{
    PlatformTools *platformTools = nullptr;
    GenericPackageData packageData;
    if (parseAbstractData(packageData, data, 1, "Platform-tools")) {
        platformTools = new PlatformTools(packageData.revision, data.at(0));
        platformTools->setDescriptionText(packageData.description);
        platformTools->setDisplayText(packageData.description);
        platformTools->setInstalledLocation(packageData.installedLocation);
    } else {
        qCDebug(sdkManagerLog) << "Platform-tools: Parsing failed. Minimum required data "
                                  "unavailable:" << data;
    }
    return platformTools;
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
    m_activeOperation(nullptr, watcherDeleter),
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
        QStringList args({"--list", "--verbose"});
        args << m_config.sdkManagerToolArgs();
        if (sdkManagerCommand(m_config, args, &packageListing)) {
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

void AndroidSdkManagerPrivate::updateInstalled(SdkCmdFutureInterface &fi)
{
    fi.setProgressRange(0, 100);
    fi.setProgressValue(0);
    AndroidSdkManager::OperationOutput result;
    result.type = AndroidSdkManager::UpdateAll;
    result.stdOutput = QCoreApplication::translate("AndroidSdkManager",
                                                   "Updating installed packages.");
    fi.reportResult(result);
    QStringList args("--update");
    args << m_config.sdkManagerToolArgs();
    if (!fi.isCanceled())
        sdkManagerCommand(m_config, args, m_sdkManager, fi, result, 100);
    else
        qCDebug(sdkManagerLog) << "Update: Operation cancelled before start";

    if (result.stdError.isEmpty() && !result.success)
        result.stdError = QCoreApplication::translate("AndroidSdkManager", "Failed.");
    result.stdOutput = QCoreApplication::translate("AndroidSdkManager", "Done\n\n");
    fi.reportResult(result);
    fi.setProgressValue(100);
}

void AndroidSdkManagerPrivate::update(SdkCmdFutureInterface &fi, const QStringList &install,
                                      const QStringList &uninstall)
{
    fi.setProgressRange(0, 100);
    fi.setProgressValue(0);
    double progressQuota = 100.0 / (install.count() + uninstall.count());
    int currentProgress = 0;

    QString installTag = QCoreApplication::translate("AndroidSdkManager", "Installing");
    QString uninstallTag = QCoreApplication::translate("AndroidSdkManager", "Uninstalling");

    auto doOperation = [&](const QString& packagePath, const QStringList& args,
            bool isInstall) {
        AndroidSdkManager::OperationOutput result;
        result.type = AndroidSdkManager::UpdatePackage;
        result.stdOutput = QString("%1 %2").arg(isInstall ? installTag : uninstallTag)
                .arg(packagePath);
        fi.reportResult(result);
        if (fi.isCanceled())
            qCDebug(sdkManagerLog) << args << "Update: Operation cancelled before start";
        else
            sdkManagerCommand(m_config, args, m_sdkManager, fi, result, progressQuota, isInstall);
        currentProgress += progressQuota;
        fi.setProgressValue(currentProgress);
        if (result.stdError.isEmpty() && !result.success)
            result.stdError = QCoreApplication::translate("AndroidSdkManager", "Failed");
        result.stdOutput = QCoreApplication::translate("AndroidSdkManager", "Done\n\n");
        fi.reportResult(result);
        return fi.isCanceled();
    };


    // Uninstall packages
    for (const QString &sdkStylePath : uninstall) {
        // Uninstall operations are not interptible. We don't want to leave half uninstalled.
        QStringList args;
        args << "--uninstall" << sdkStylePath << m_config.sdkManagerToolArgs();
        if (doOperation(sdkStylePath, args, false))
            break;
    }

    // Install packages
    for (const QString &sdkStylePath : install) {
        QStringList args(sdkStylePath);
        args << m_config.sdkManagerToolArgs();
        if (doOperation(sdkStylePath, args, true))
            break;
    }
    fi.setProgressValue(100);
}

void AndroidSdkManagerPrivate::checkPendingLicense(SdkCmdFutureInterface &fi)
{
    fi.setProgressRange(0, 100);
    fi.setProgressValue(0);
    AndroidSdkManager::OperationOutput result;
    result.type = AndroidSdkManager::LicenseCheck;
    QStringList args("--licenses");
    if (!fi.isCanceled())
        sdkManagerCommand(m_config, args, m_sdkManager, fi, result, 100.0);
    else
        qCDebug(sdkManagerLog) << "Update: Operation cancelled before start";

    fi.reportResult(result);
    fi.setProgressValue(100);
}

void AndroidSdkManagerPrivate::getPendingLicense(SdkCmdFutureInterface &fi)
{
    fi.setProgressRange(0, 100);
    fi.setProgressValue(0);
    AndroidSdkManager::OperationOutput result;
    result.type = AndroidSdkManager::LicenseWorkflow;
    QtcProcess licenseCommand;
    licenseCommand.setProcessEnvironment(AndroidConfigurations::toolsEnvironment(m_config));
    bool reviewingLicenses = false;
    licenseCommand.setCommand(m_config.sdkManagerToolPath().toString(), {"--licenses"});
    if (Utils::HostOsInfo::isWindowsHost())
        licenseCommand.setUseCtrlCStub(true);
    licenseCommand.start();
    QTextCodec *codec = QTextCodec::codecForLocale();
    int inputCounter = 0, steps = -1;
    while (!licenseCommand.waitForFinished(200)) {
        QString stdOut = codec->toUnicode(licenseCommand.readAllStandardOutput());
        bool assertionFound = false;
        if (!stdOut.isEmpty())
            assertionFound = onLicenseStdOut(stdOut, reviewingLicenses, result, fi);

        if (reviewingLicenses) {
            // Check user input
            QByteArray userInput = getUserInput();
            if (!userInput.isEmpty()) {
                clearUserInput();
                licenseCommand.write(userInput);
                ++inputCounter;
                if (steps != -1)
                    fi.setProgressValue(qRound((inputCounter / (double)steps) * 100));
            }
        } else if (assertionFound) {
            // The first assertion is to start reviewing licenses. Always accept.
            reviewingLicenses = true;
            QRegularExpression reg("(\\d+\\sof\\s)(?<steps>\\d+)");
            QRegularExpressionMatch match = reg.match(stdOut);
            if (match.hasMatch())
                steps = match.captured("steps").toInt();
            licenseCommand.write("Y\n");
        }

        if (fi.isCanceled()) {
            licenseCommand.terminate();
            if (!licenseCommand.waitForFinished(300)) {
                licenseCommand.kill();
                licenseCommand.waitForFinished(200);
            }
        }
        if (licenseCommand.state() == QProcess::NotRunning)
            break;
    }

    m_licenseTextCache.clear();
    result.success = licenseCommand.exitStatus() == QtcProcess::NormalExit;
    if (!result.success) {
        result.stdError = QCoreApplication::translate("Android::Internal::AndroidSdkManager",
                                                      "License command failed.\n\n");
    }
    fi.reportResult(result);
    fi.setProgressValue(100);
}

void AndroidSdkManagerPrivate::setLicenseInput(bool acceptLicense)
{
    QWriteLocker locker(&m_licenseInputLock);
    m_licenseUserInput = acceptLicense ? "Y\n" : "n\n";
}

QByteArray AndroidSdkManagerPrivate::getUserInput() const
{
    QReadLocker locker(&m_licenseInputLock);
    return m_licenseUserInput;
}

void AndroidSdkManagerPrivate::clearUserInput()
{
    QWriteLocker locker(&m_licenseInputLock);
    m_licenseUserInput.clear();
}

bool AndroidSdkManagerPrivate::onLicenseStdOut(const QString &output, bool notify,
                                               AndroidSdkManager::OperationOutput &result,
                                               SdkCmdFutureInterface &fi)
{
    m_licenseTextCache.append(output);
    QRegularExpressionMatch assertionMatch = assertionReg.match(m_licenseTextCache);
    if (assertionMatch.hasMatch()) {
        if (notify) {
            result.stdOutput = m_licenseTextCache;
            fi.reportResult(result);
        }
        // Clear the current contents. The found license text is dispatched. Continue collecting the
        // next license text.
        m_licenseTextCache.clear();
        return true;
    }
    return false;
}

void AndroidSdkManagerPrivate::addWatcher(const QFuture<AndroidSdkManager::OperationOutput> &future)
{
    if (future.isFinished())
        return;
    m_activeOperation.reset(new QFutureWatcher<void>());
    m_activeOperation->setFuture(future);
}

void AndroidSdkManagerPrivate::parseCommonArguments(QFutureInterface<QString> &fi)
{
    QString argumentDetails;
    QString output;
    sdkManagerCommand(m_config, QStringList("--help"), &output);
    bool foundTag = false;
    for (const QString& line : output.split('\n')) {
        if (fi.isCanceled())
            break;
        if (foundTag)
            argumentDetails.append(line + "\n");
        else if (line.startsWith(commonArgsKey))
            foundTag = true;
    }

    if (!fi.isCanceled())
        fi.reportResult(argumentDetails);
}

void AndroidSdkManagerPrivate::clearPackages()
{
    for (AndroidSdkPackage *p : m_allPackages)
        delete p;
    m_allPackages.clear();
}

} // namespace Internal
} // namespace Android
