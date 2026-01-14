// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iosdevice.h"

#include "devicectlutils.h"
#include "iosconfigurations.h"
#include "iosconstants.h"
#include "iossimulator.h"
#include "iostoolhandler.h"
#include "iostr.h"

#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevicefactory.h>
#include <projectexplorer/devicesupport/idevicewidget.h>
#include <projectexplorer/environmentkitaspect.h>

#include <utils/co_result.h>
#include <utils/devicefileaccess.h>
#include <utils/layoutbuilder.h>
#include <utils/portlist.h>
#include <utils/qtcprocess.h>
#include <utils/shutdownguard.h>
#include <utils/synchronizedvalue.h>
#include <utils/temporaryfile.h>
#include <utils/url.h>

#include <QtTaskTree/QTaskTree>

#include <QJsonArray>
#include <QJsonDocument>
#include <QMessageBox>

#ifdef Q_OS_MAC
#include <IOKit/IOKitLib.h>
#include <IOKit/usb/IOUSBLib.h>
#include <CoreFoundation/CoreFoundation.h>

// Work around issue with not being able to retrieve USB serial number.
// See QTCREATORBUG-23460.
// For an unclear reason USBSpec.h in macOS SDK 10.15 uses a different value if
// MAC_OS_X_VERSION_MIN_REQUIRED > MAC_OS_X_VERSION_10_14, which just does not work.
#if MAC_OS_X_VERSION_MIN_REQUIRED > MAC_OS_X_VERSION_10_14
#undef kUSBSerialNumberString
#define kUSBSerialNumberString "USB Serial Number"
#endif

#endif

#include <exception>

using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace {
static Q_LOGGING_CATEGORY(detectLog, "qtc.ios.deviceDetect", QtWarningMsg)
}

#ifdef Q_OS_MAC
static QString CFStringRef2QString(CFStringRef s)
{
    unsigned char buf[250];
    CFIndex len = CFStringGetLength(s);
    CFIndex usedBufLen;
    CFIndex converted = CFStringGetBytes(s, CFRangeMake(0,len), kCFStringEncodingUTF8,
                                         '?', false, &buf[0], sizeof(buf), &usedBufLen);
    if (converted == len)
        return QString::fromUtf8(reinterpret_cast<char *>(&buf[0]), usedBufLen);
    size_t bufSize = sizeof(buf)
            + CFStringGetMaximumSizeForEncoding(len - converted, kCFStringEncodingUTF8);
    unsigned char *bigBuf = new unsigned char[bufSize];
    memcpy(bigBuf, buf, usedBufLen);
    CFIndex newUseBufLen;
    CFStringGetBytes(s, CFRangeMake(converted,len), kCFStringEncodingUTF8,
                     '?', false, &bigBuf[usedBufLen], bufSize, &newUseBufLen);
    QString res = QString::fromUtf8(reinterpret_cast<char *>(bigBuf), usedBufLen + newUseBufLen);
    delete[] bigBuf;
    return res;
}
#endif

namespace Ios::Internal {

const char kHandler[] = "Handler";

struct PathInfo
{
    QStringList pathComponents;
    std::optional<QString> domain;
    std::optional<QString> domainIdentifier;
    std::optional<QString> subPath;
};

static PathInfo getPathInfo(const FilePath &path)
{
    PathInfo info;
    // Get file list for corresponding domain and subdir
    info.pathComponents
        = Utils::transform(path.cleanPath().pathComponents(), [](const QStringView &v) {
              return v.toString();
          });
    if (info.pathComponents.size() < 2)
        return info;
    if (info.pathComponents.at(1) == "systemCrashLogs") {
        info.domain = "systemCrashLogs";
    } else if (info.pathComponents.at(1) == "temporary") {
        info.domain = "temporary";
        info.domainIdentifier = "qtcreator"; // self-chosen
    } else {
        info.domain = "appDataContainer";
        info.domainIdentifier = info.pathComponents.at(1);
    }
    info.subPath = "/" + info.pathComponents.mid(2).join('/');
    return info;
}

static Result<QMap<FilePath, FilePathInfo>> getFileList(const QString &deviceId, const FilePath &path)
{
    const PathInfo info = getPathInfo(path);
    QTC_ASSERT(info.domain && info.subPath, return make_unexpected(Tr::tr("Internal error.")));
    QStringList args{
        "devicectl",
        "device",
        "info",
        "files",
        "--device",
        deviceId,
        "--quiet",
        "--json-output",
        "-",
        "--subdirectory",
        *info.subPath,
        "--domain-type",
        *info.domain};
    if (info.domainIdentifier)
        args += QStringList({"--domain-identifier", *info.domainIdentifier});

    Process p;
    p.setCommand({FilePath::fromString("/usr/bin/xcrun"), args});
    p.runBlocking();
    const Result<QMap<Utils::FilePath, Utils::FilePathInfo>> files
        = parseFileList(p.rawStdOut(), path);
    return files;
}

class IosFileAccess : public DeviceFileAccess
{
public:
    IosFileAccess(const QString &deviceId);

    Result<Environment> deviceEnvironment() const final;

protected:
    Result<bool> isExecutableFile(const FilePath &filePath) const final;
    Result<bool> isReadableFile(const FilePath &filePath) const final;
    Result<bool> isWritableFile(const FilePath &filePath) const final;
    Result<bool> isReadableDirectory(const FilePath &filePath) const final;
    Result<bool> isWritableDirectory(const FilePath &filePath) const final;
    Result<bool> isFile(const FilePath &filePath) const final;
    Result<bool> isDirectory(const FilePath &filePath) const final;
    Result<bool> isSymLink(const FilePath &filePath) const final;
    Result<bool> hasHardLinks(const FilePath &filePath) const final;
    // Result<> ensureExistingFile(const FilePath &filePath) const final;
    // Result<> createDirectory(const FilePath &filePath) const final;
    Result<bool> exists(const FilePath &filePath) const final;
    // Result<> removeFile(const FilePath &filePath) const final;
    // Result<> removeRecursively(const FilePath &filePath) const final;
    // Result<> copyFile(const FilePath &filePath, const FilePath &target) const final;
    // Result<> createSymLink(const FilePath &filePath, const FilePath &symLink) const final;
    // Result<> renameFile(const FilePath &filePath, const FilePath &target) const final;
    // Result<FilePath> symLinkTarget(const FilePath &filePath) const final;
    Result<FilePathInfo> filePathInfo(const FilePath &filePath) const final;
    Result<QDateTime> lastModified(const FilePath &filePath) const final;
    Result<QFileDevice::Permissions> permissions(const FilePath &filePath) const final;
    // Result<> setPermissions(const FilePath &filePath, QFileDevice::Permissions) const final;
    Result<qint64> fileSize(const FilePath &filePath) const final;
    // Result<QString> owner(const FilePath &filePath) const final;
    // Result<uint> ownerId(const FilePath &filePath) const final;
    // Result<QString> group(const FilePath &filePath) const final;
    // Result<uint> groupId(const FilePath &filePath) const final;
    // Result<qint64> bytesAvailable(const FilePath &filePath) const final;
    // Result<QByteArray> fileId(const FilePath &filePath) const final;
    Result<> iterateDirectory(
        const FilePath &filePath,
        const FilePath::IterateDirCallback &callBack,
        const FileFilter &filter) const final;
    Result<QByteArray> fileContents(
        const FilePath &filePath, qint64 limit, qint64 offset) const final;
    Result<qint64> writeFileContents(const FilePath &filePath, const QByteArray &data) const final;
    // Result<FilePath> createTempFile(const FilePath &filePath) final;
    // Result<FilePath> createTempDir(const FilePath &filePath) final;
    std::vector<Result<std::unique_ptr<FilePathWatcher>>> watch(const FilePaths &paths) const final;
    bool supportsAtomicSaveFile(const FilePath &filePath) const;
    bool supportsRemovingFiles() const;

private:
    QString m_deviceId;
    struct CacheItem
    {
        FilePathInfo info;
        QDateTime time;
    };

    // TODO cache grows and is never really cleared
    using InfoCache = QMap<FilePath, CacheItem>;
    mutable SynchronizedValue<InfoCache> m_cache;
    std::optional<FilePathInfo> cachedFilePathInfo(const FilePath &filePath) const;
    void updateCache(const QMap<FilePath, FilePathInfo> &files) const;
    void invalidateCache(const FilePath &filePath) const;
};

IosFileAccess::IosFileAccess(const QString &deviceId)
    : m_deviceId(deviceId)
{}

Result<Environment> IosFileAccess::deviceEnvironment() const
{
    return Environment();
}

Result<bool> IosFileAccess::isExecutableFile(const FilePath &filePath) const
{
    const Result<FilePathInfo> info = filePathInfo(filePath);
    if (!info)
        return make_unexpected(info.error());
    return info->fileFlags & FilePathInfo::FileType && info->fileFlags & FilePathInfo::ExeUserPerm;
}

Result<bool> IosFileAccess::isReadableFile(const FilePath &filePath) const
{
    const Result<FilePathInfo> info = filePathInfo(filePath);
    if (!info)
        return make_unexpected(info.error());
    return info->fileFlags & FilePathInfo::FileType && info->fileFlags & FilePathInfo::ReadUserPerm;
}

Result<bool> IosFileAccess::isWritableFile(const FilePath &filePath) const
{
    const Result<FilePathInfo> info = filePathInfo(filePath);
    if (!info)
        return make_unexpected(info.error());
    return info->fileFlags & FilePathInfo::FileType
           && info->fileFlags & FilePathInfo::WriteUserPerm;
}

Result<bool> IosFileAccess::isReadableDirectory(const FilePath &filePath) const
{
    const Result<FilePathInfo> info = filePathInfo(filePath);
    if (!info)
        return make_unexpected(info.error());
    return info->fileFlags & FilePathInfo::DirectoryType
           && info->fileFlags & FilePathInfo::ReadUserPerm;
}

Result<bool> IosFileAccess::isWritableDirectory(const FilePath &filePath) const
{
    const Result<FilePathInfo> info = filePathInfo(filePath);
    if (!info)
        return make_unexpected(info.error());
    return info->fileFlags & FilePathInfo::DirectoryType
           && info->fileFlags & FilePathInfo::WriteUserPerm;
}

Result<bool> IosFileAccess::isFile(const FilePath &filePath) const
{
    const Result<FilePathInfo> info = filePathInfo(filePath);
    if (!info)
        return make_unexpected(info.error());
    return info->fileFlags & FilePathInfo::FileType;
}

Result<bool> IosFileAccess::isDirectory(const FilePath &filePath) const
{
    const Result<FilePathInfo> info = filePathInfo(filePath);
    if (!info)
        return make_unexpected(info.error());
    return info->fileFlags & FilePathInfo::DirectoryType;
}

Result<bool> IosFileAccess::isSymLink(const FilePath &) const
{
    return false;
}

Result<bool> IosFileAccess::hasHardLinks(const FilePath &) const
{
    return false;
}

Result<bool> IosFileAccess::exists(const FilePath &filePath) const
{
    const Result<FilePathInfo> info = filePathInfo(filePath);
    if (!info)
        return make_unexpected(info.error());
    return info->fileFlags & FilePathInfo::ExistsFlag;
}

Result<FilePathInfo> IosFileAccess::filePathInfo(const FilePath &filePath) const
{
    const std::optional<FilePathInfo> cachedInfo = cachedFilePathInfo(filePath);
    if (cachedInfo)
        return *cachedInfo;
    const PathInfo info = getPathInfo(filePath.parentDir());
    if (!info.domain) {
        // TODO: should be limited to actually existing root directories
        return FilePathInfo(
            {0,
             FilePathInfo::FileFlags(
                 FilePathInfo::PermsMask | FilePathInfo::DirectoryType | FilePathInfo::ExistsFlag),
             {}});
    }
    const Result<QMap<FilePath, FilePathInfo>> files = getFileList(m_deviceId, filePath.parentDir());
    if (!files) // TODO: should not return error if parent directory does not exist
        return make_unexpected(files.error());
    updateCache(*files);
    return files->value(filePath);
}

Result<QDateTime> IosFileAccess::lastModified(const FilePath &filePath) const
{
    const Result<FilePathInfo> info = filePathInfo(filePath);
    if (!info)
        return make_unexpected(info.error());
    return info->lastModified;
}

Result<QFileDevice::Permissions> IosFileAccess::permissions(const FilePath &filePath) const
{
    const Result<FilePathInfo> info = filePathInfo(filePath);
    if (!info)
        return make_unexpected(info.error());
    return QFileDevice::Permissions(int(info->fileFlags));
}

Result<qint64> IosFileAccess::fileSize(const FilePath &filePath) const
{
    const Result<FilePathInfo> info = filePathInfo(filePath);
    if (!info)
        return make_unexpected(info.error());
    return info->fileSize;
}

Result<> IosFileAccess::iterateDirectory(
    const FilePath &filePath,
    const FilePath::IterateDirCallback &callBack,
    const FileFilter &filter) const
{
    const auto callCallback = [&callBack](const FilePath &f, const FilePathInfo &info) {
        if (callBack.index() == 0)
            return std::get<0>(callBack)(f) == IterationPolicy::Continue;
        else
            return std::get<1>(callBack)(f, info) == IterationPolicy::Continue;
    };
    const bool isRecursive = filter.iteratorFlags.testFlag(QDirIterator::Subdirectories);
    QString nameFilterRegExStr = Utils::transform(filter.nameFilters, [](const QString &filter) {
                                     return QRegularExpression::wildcardToRegularExpression(filter);
                                 }).join(")|(");
    if (!nameFilterRegExStr.isEmpty())
        nameFilterRegExStr = "(" + nameFilterRegExStr + ")";
    const QRegularExpression nameFilterRegEx(
        nameFilterRegExStr,
        filter.fileFilters.testFlag(QDir::CaseSensitive)
            ? QRegularExpression::NoPatternOption
            : QRegularExpression::CaseInsensitiveOption);
    const auto passesFilter = [fileFilter = filter.fileFilters,
                               nameFilterRegEx](const FilePath &filePath, const FilePathInfo &info) {
        // check file patterns, but if AllDirs is set then only for files
        const bool isDirectory = info.fileFlags.testFlag(FilePathInfo::DirectoryType);
        if (!nameFilterRegEx.pattern().isEmpty()                    // we have patterns
            && !(fileFilter.testFlag(QDir::AllDirs) && isDirectory) // directories are not excluded
            && !nameFilterRegEx.match(filePath.fileName()).hasMatch()) { // it doesn't match
            return false;
        }
        if (!fileFilter.testFlag(QDir::Dirs) && isDirectory)
            return false;
        if (!fileFilter.testFlag(QDir::Files) && info.fileFlags.testFlag(FilePathInfo::FileType))
            return false;
        if (!fileFilter.testFlag(QDir::Hidden) && info.fileFlags.testFlag(FilePathInfo::HiddenFlag))
            return false;
        if (fileFilter.testFlag(QDir::NoSymLinks) && info.fileFlags.testFlag(FilePathInfo::LinkType))
            return false;
        if (fileFilter.testFlag(QDir::Readable)
            && !info.fileFlags.testFlag(FilePathInfo::ReadUserPerm))
            return false;
        if (fileFilter.testFlag(QDir::Writable)
            && !info.fileFlags.testFlag(FilePathInfo::WriteUserPerm))
            return false;
        if (fileFilter.testFlag(QDir::Executable)
            && !info.fileFlags.testFlag(FilePathInfo::ExeUserPerm))
            return false;
        return true;
    };

    FilePaths pathsToRecurse({filePath});

    // Subitems of root path are systemCrashLogs and app IDs
    if (filePath.isRootPath()) {
        pathsToRecurse.clear();
        if (!filter.fileFilters.testFlag(QDir::Dirs))
            return ResultOk;
        FilePathInfo info(
            {0,
             FilePathInfo::FileFlags(
                 FilePathInfo::PermsMask | FilePathInfo::DirectoryType | FilePathInfo::ExistsFlag),
             {}});
        const FilePath crashLogPath = filePath.withNewPath("/systemCrashLogs");
        if (passesFilter(crashLogPath, info)) {
            if (!callCallback(crashLogPath, info))
                return ResultOk;
            pathsToRecurse.append(crashLogPath);
        }
        Process p;
        p.setCommand(
            {FilePath::fromString("/usr/bin/xcrun"),
             {"devicectl",
              "device",
              "info",
              "apps",
              "--device",
              m_deviceId,
              "--quiet",
              "--json-output",
              "-"}});
        p.runBlocking();
        const Result<QSet<QString>> appIds = parseAppIdentifiers(p.rawStdOut());
        if (!appIds)
            return make_unexpected(appIds.error());
        for (const QString &id : *appIds) {
            const FilePath fp = filePath.withNewPath("/" + id);
            if (passesFilter(fp, info)) {
                if (!callCallback(fp, info))
                    return ResultOk;
                pathsToRecurse.append(fp);
            }
        }
        if (!isRecursive)
            return ResultOk;
        // continue with pathsToRecurse if filter wants to iterate subdirs
    }
    while (!pathsToRecurse.isEmpty()) {
        const FilePath current = pathsToRecurse.takeFirst();
        const Result<QMap<FilePath, FilePathInfo>> files = getFileList(m_deviceId, current);
        if (!files)
            return make_unexpected(files.error());
        updateCache(*files);
        for (auto it = files->cbegin(); it != files->cend(); ++it) {
            if (passesFilter(it.key(), it.value())) {
                if (!callCallback(it.key(), it.value()))
                    return ResultOk;
                if (isRecursive && it.value().fileFlags.testFlag(FilePathInfo::DirectoryType))
                    pathsToRecurse.append(it.key());
            }
        }
    }
    return ResultOk;
}

Result<QByteArray> IosFileAccess::fileContents(
    const FilePath &filePath, qint64 limit, qint64 offset) const
{
    const PathInfo info = getPathInfo(filePath);
    QTC_ASSERT(info.domain && info.subPath,
               return make_unexpected(
                   Tr::tr("Cannot retrieve file contents for \"%1\".")
                       .arg(filePath.toUserOutput())););
    const auto tempPath = []() -> Result<FilePath> {
        TemporaryFile tempFile("ios-file-download");
        if (tempFile.open())
            return tempFile.filePath();
        return make_unexpected(Tr::tr("Failed to create temporary file."));
    }();
    if (!tempPath)
        return make_unexpected(tempPath.error());
    QStringList args{
        "devicectl",
        "device",
        "copy",
        "from",
        "--device",
        m_deviceId,
        "--quiet",
        "--json-output",
        "-",
        "--source",
        *info.subPath,
        "--destination",
        tempPath->nativePath(),
        "--domain-type",
        *info.domain};
    if (info.domainIdentifier)
        args += QStringList({"--domain-identifier", *info.domainIdentifier});

    Process p;
    p.setCommand({FilePath::fromString("/usr/bin/xcrun"), args});
    p.runBlocking();
    const Result<> success = checkDevicectlResult(p.rawStdOut());
    if (!success)
        return make_unexpected(success.error());
    QScopeGuard remove([tempPath] { tempPath->removeFile(); });
    return tempPath->fileContents(limit, offset);
}

Result<qint64> IosFileAccess::writeFileContents(const FilePath &filePath, const QByteArray &data) const
{
    const PathInfo info = getPathInfo(filePath);
    QTC_ASSERT(info.domain && info.subPath,
               return make_unexpected(
                   Tr::tr("Cannot write file contents for \"%1\".").arg(filePath.toUserOutput())););
    const auto tempPath = []() -> Result<FilePath> {
        TemporaryFile tempFile("ios-file-download");
        if (tempFile.open())
            return tempFile.filePath();
        return make_unexpected(Tr::tr("Failed to create temporary file."));
    }();
    if (!tempPath)
        return make_unexpected(tempPath.error());
    const Result<qint64> result = tempPath->writeFileContents(data);
    QScopeGuard remove([tempPath] { tempPath->removeFile(); });
    if (!result)
        return result;
    QStringList args{
        "devicectl",
        "device",
        "copy",
        "to",
        "--device",
        m_deviceId,
        "--quiet",
        "--json-output",
        "-",
        "--destination",
        *info.subPath,
        "--source",
        tempPath->nativePath(),
        "--domain-type",
        *info.domain};
    if (info.domainIdentifier)
        args += QStringList({"--domain-identifier", *info.domainIdentifier});

    Process p;
    p.setCommand({FilePath::fromString("/usr/bin/xcrun"), args});
    p.runBlocking();
    const Result<> success = checkDevicectlResult(p.rawStdOut());
    if (!success)
        return make_unexpected(success.error());
    invalidateCache(filePath);
    return result;
}

std::vector<Result<std::unique_ptr<FilePathWatcher>>> IosFileAccess::watch(
    const FilePaths &paths) const
{
    // not really implemented, but return dummies to avoid warnings
    return Utils::transform<std::vector>(paths, [](const FilePath &) {
        return Result<std::unique_ptr<FilePathWatcher>>(std::make_unique<FilePathWatcher>());
    });
}

bool IosFileAccess::supportsAtomicSaveFile(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    // we cannot move or remove files
    return false;
}

bool IosFileAccess::supportsRemovingFiles() const
{
    return false;
}

std::optional<FilePathInfo> IosFileAccess::cachedFilePathInfo(const FilePath &filePath) const
{
    static const int kCacheSeconds = 5;
    std::optional<FilePathInfo> result;
    m_cache.write([filePath, &result](InfoCache &cache) {
        const CacheItem item = cache.value(filePath);
        if (item.time.isValid()) {
            if (item.time.secsTo(QDateTime::currentDateTime()) < kCacheSeconds)
                result = item.info;
            else
                cache.remove(filePath);
        }
    });
    return result;
}

void IosFileAccess::updateCache(const QMap<FilePath, FilePathInfo> &files) const
{
    m_cache.write([files](InfoCache &cache) {
        const auto now = QDateTime::currentDateTime();
        for (auto it = files.cbegin(); it != files.cend(); ++it)
            cache.insert(it.key(), {it.value(), now});
    });
}

void IosFileAccess::invalidateCache(const FilePath &filePath) const
{
    m_cache.writeLocked()->remove(filePath);
}

class IosDeviceInfoWidget final : public IDeviceWidget
{
public:
    IosDeviceInfoWidget(const IDevice::Ptr &device)
        : IDeviceWidget(device)
    {
        const auto iosDevice = std::static_pointer_cast<IosDevice>(device);
        using namespace Layouting;
        // clang-format off
        Form {
            Tr::tr("Device name:"), iosDevice->deviceName(), br,
            Tr::tr("Identifier:"), iosDevice->uniqueInternalDeviceId(), br,
            Tr::tr("Product type:"), iosDevice->productType(), br,
            Tr::tr("CPU Architecture:"), iosDevice->cpuArchitecture(), br,
            Tr::tr("OS Version:"), iosDevice->osVersion(), br,
            noMargin
        }.attachTo(this);
        // clang-format on

        installMarkSettingsDirtyTriggerRecursively(this);
    }

    void updateDeviceFromUi() final {}
};

IosDevice::IosDevice(CtorHelper)
{
    setType(Constants::IOS_DEVICE_TYPE);
    setDefaultDisplayName(IosDevice::name());
    setDisplayType(Tr::tr("iOS"));
    setMachineType(IDevice::Hardware);
    setOsType(Utils::OsTypeMac);
}

IosDevice::IosDevice()
    : IosDevice(CtorHelper{})
{
    setupId(IDevice::AutoDetected, Constants::IOS_DEVICE_ID);

    Utils::PortList ports;
    ports.addRange(Utils::Port(Constants::IOS_DEVICE_PORT_START),
                   Utils::Port(Constants::IOS_DEVICE_PORT_END));
    setFreePorts(ports);
}

IosDevice::IosDevice(const QString &uid)
    : IosDevice(CtorHelper{})
{
    setupId(IDevice::AutoDetected, Utils::Id(Constants::IOS_DEVICE_ID).withSuffix(uid));
}

IDevice::DeviceInfo IosDevice::deviceInformation() const
{
    IDevice::DeviceInfo res;
    for (auto i = m_extraInfo.cbegin(), end = m_extraInfo.cend(); i != end; ++i) {
        IosDeviceManager::TranslationMap tMap = IosDeviceManager::translationMap();
        if (tMap.contains(i.key()))
            res.append(DeviceInfoItem(tMap.value(i.key()), tMap.value(i.value(), i.value())));
    }
    return res;
}

IDeviceWidget *IosDevice::createWidget()
{
    return new IosDeviceInfoWidget(shared_from_this());
}

void IosDevice::fromMap(const Store &map)
{
    IDevice::fromMap(map);

    m_extraInfo.clear();
    const Store vMap = storeFromVariant(map.value(Constants::EXTRA_INFO_KEY));
    for (auto i = vMap.cbegin(), end = vMap.cend(); i != end; ++i)
        m_extraInfo.insert(stringFromKey(i.key()), i.value().toString());
    m_handler = Handler(map.value(kHandler).toInt());
    // TODO IDevice::fromMap overrides the port list that we set in the constructor
    //      this shouldn't happen
    Utils::PortList ports;
    ports.addRange(
        Utils::Port(Constants::IOS_DEVICE_PORT_START), Utils::Port(Constants::IOS_DEVICE_PORT_END));
    setFreePorts(ports);
}

void IosDevice::toMap(Store &map) const
{
    IDevice::toMap(map);

    Store vMap;
    for (auto i = m_extraInfo.cbegin(), end = m_extraInfo.cend(); i != end; ++i)
        vMap.insert(keyFromString(i.key()), i.value());
    map.insert(Constants::EXTRA_INFO_KEY, variantFromStore(vMap));
    map.insert(kHandler, int(m_handler));
}

ExecutableItem IosDevice::portsGatheringRecipe(
    [[maybe_unused]] const Storage<PortsOutputData> &output) const
{
    // We don't really know how to get all used ports on the device.
    // The code in <= 15.0 cycled through the list (30001 for the first run,
    // 30002 for the second run etc)
    // I guess that would be needed if we could run/profile multiple applications on
    // the device simultaneously, we cannot
    return Group{nullItem};
}

QUrl IosDevice::toolControlChannel(const ControlChannelHint &) const
{
    QUrl url;
    url.setScheme(Utils::urlTcpScheme());
    url.setHost("localhost");
    return url;
}

QString IosDevice::deviceName() const
{
    return m_extraInfo.value(kDeviceName);
}

QString IosDevice::uniqueDeviceID() const
{
    return id().suffixAfter(Id(Constants::IOS_DEVICE_ID));
}

QString IosDevice::uniqueInternalDeviceId() const
{
    return m_extraInfo.value(kUniqueDeviceId);
}

QString IosDevice::name()
{
    return Tr::tr("iOS Device");
}

QString IosDevice::osVersion() const
{
    return m_extraInfo.value(kOsVersion);
}

QString IosDevice::productType() const
{
    return m_extraInfo.value(kProductType);
}

QString IosDevice::cpuArchitecture() const
{
    return m_extraInfo.value(kCpuArchitecture);
}

IosDevice::Handler IosDevice::handler() const
{
    return m_handler;
}

// IosDeviceManager

IosDeviceManager::TranslationMap IosDeviceManager::translationMap()
{
    static TranslationMap *translationMap = nullptr;
    if (translationMap)
        return *translationMap;
    TranslationMap &tMap = *new TranslationMap;
    tMap[kDeviceName] = Tr::tr("Device name");
    //: Whether the device is in developer mode.
    tMap[kDeveloperStatus]                 = Tr::tr("Developer status");
    tMap[kDeviceConnected]                 = Tr::tr("Connected");
    tMap[vYes]                             = Tr::tr("yes");
    tMap[QLatin1String("NO")]              = Tr::tr("no");
    tMap[QLatin1String("*unknown*")]       = Tr::tr("unknown");
    tMap[kOsVersion]                       = Tr::tr("OS version");
    tMap[kProductType] = Tr::tr("Product type");
    translationMap = &tMap;
    return tMap;
}

void IosDeviceManager::deviceConnected(const QString &uid, const QString &name)
{
    Utils::Id baseDevId(Constants::IOS_DEVICE_ID);
    Utils::Id devType(Constants::IOS_DEVICE_TYPE);
    Utils::Id devId = baseDevId.withSuffix(uid);
    IDevice::Ptr dev = DeviceManager::find(devId);
    if (!dev) {
        auto newDev = IosDevice::make(uid);
        if (!name.isNull())
            newDev->setDisplayName(name);
        qCDebug(detectLog) << "adding ios device " << uid;
        DeviceManager::addDevice(newDev);
    } else if (dev->deviceState() != IDevice::DeviceConnected &&
               dev->deviceState() != IDevice::DeviceReadyToUse) {
        qCDebug(detectLog) << "updating ios device " << uid;

        if (dev->type() == devType) // FIXME: Should that be a QTC_ASSERT?
            DeviceManager::addDevice(dev);
        else
            DeviceManager::addDevice(IosDevice::make(uid));
    }
    updateInfo(uid);
}

void IosDeviceManager::deviceDisconnected(const QString &uid)
{
    qCDebug(detectLog) << "detected disconnection of ios device " << uid;
    // if an update is currently still running for the device being connected, cancel that
    // erasing deletes the unique_ptr which deletes the TaskTree which stops it
    m_updatesRunner.resetKey(uid);
    Utils::Id baseDevId(Constants::IOS_DEVICE_ID);
    Utils::Id devType(Constants::IOS_DEVICE_TYPE);
    Utils::Id devId = baseDevId.withSuffix(uid);
    IDevice::ConstPtr dev = DeviceManager::find(devId);
    if (!dev || dev->type() != devType) {
        qCWarning(detectLog) << "ignoring disconnection of ios device " << uid; // should neve happen
    } else {
        auto iosDev = static_cast<const IosDevice *>(dev.get());
        if (iosDev->m_extraInfo.isEmpty()
            || iosDev->m_extraInfo.value(kDeviceName) == QLatin1String("*unknown*")) {
            DeviceManager::removeDevice(iosDev->id());
        } else if (iosDev->deviceState() != IDevice::DeviceDisconnected) {
            qCDebug(detectLog) << "disconnecting device " << iosDev->uniqueDeviceID();
            DeviceManager::setDeviceState(iosDev->id(), IDevice::DeviceDisconnected);
        }
    }
}

void IosDeviceManager::updateInfo(const QString &devId)
{
    const auto getDeviceCtlVersion = ProcessTask(
        [](Process &process) {
            process.setCommand({FilePath::fromString("/usr/bin/xcrun"), {"devicectl", "--version"}});
        },
        [this](const Process &process) {
            m_deviceCtlVersion = QVersionNumber::fromString(process.stdOut());
            qCDebug(detectLog) << "devicectl version:" << *m_deviceCtlVersion;
        });

    const auto infoFromDeviceCtl = ProcessTask(
        [](Process &process) {
            process.setCommand({FilePath::fromString("/usr/bin/xcrun"),
                                {"devicectl", "list", "devices", "--quiet", "--json-output", "-"}});
        },
        [this, devId](const Process &process) {
            const Result<QMap<QString, QString>> result = parseDeviceInfo(process.rawStdOut(),
                                                                                devId);
            if (!result) {
                qCDebug(detectLog) << result.error();
                return DoneResult::Error;
            }
            deviceInfo(devId, IosDevice::Handler::DeviceCtl, *result);
            return DoneResult::Success;
        },
        CallDone::OnSuccess);

    const auto infoFromIosTool = IosToolTask([this, devId](IosToolRunner &runner) {
        runner.setDeviceType(IosDeviceType::IosDevice);
        runner.setStartHandler([this, devId](IosToolHandler *handler) {
            connect(
                handler,
                &IosToolHandler::deviceInfo,
                this,
                [this](IosToolHandler *, const QString &uid, const Ios::IosToolHandler::Dict &info) {
                    deviceInfo(uid, IosDevice::Handler::IosTool, info);
                },
                Qt::QueuedConnection);
            handler->requestDeviceInfo(devId);
        });
    });

    // clang-format off
    const Group recipe {
        parallel,
        continueOnError,
        m_deviceCtlVersion ? nullItem : getDeviceCtlVersion,
        Group {
            sequential, stopOnSuccess, infoFromDeviceCtl, infoFromIosTool
        }
    };
    // clang-format on

    m_updatesRunner.start(devId, recipe);
}

void IosDeviceManager::deviceInfo(const QString &uid,
                                  IosDevice::Handler handler,
                                  const Ios::IosToolHandler::Dict &info)
{
    qCDebug(detectLog) << "got device information:" << info;
    Utils::Id baseDevId(Constants::IOS_DEVICE_ID);
    Utils::Id devType(Constants::IOS_DEVICE_TYPE);
    Utils::Id devId = baseDevId.withSuffix(uid);
    IDevice::Ptr dev = DeviceManager::find(devId);
    bool skipUpdate = false;
    IosDevice::Ptr newDev;
    if (dev && dev->type() == devType) {
        IosDevice::Ptr iosDev = std::static_pointer_cast<IosDevice>(dev);
        if (iosDev->m_handler == handler && iosDev->m_extraInfo == info) {
            skipUpdate = true;
            newDev = iosDev;
        } else {
            Store store;
            iosDev->toMap(store);
            newDev = IosDevice::make();
            newDev->fromMap(store);
        }
    } else {
        newDev = IosDevice::make(uid);
    }
    if (!skipUpdate) {
        if (info.contains(kDeviceName))
            newDev->setDisplayName(info.value(kDeviceName));
        newDev->m_extraInfo = info;
        newDev->m_handler = handler;
        qCDebug(detectLog) << "updated info of ios device " << uid;
        dev = newDev;
        DeviceManager::addDevice(dev);
    }
    QLatin1String devStatusKey = QLatin1String("developerStatus");
    if (info.contains(devStatusKey)) {
        QString devStatus = info.value(devStatusKey);
        if (devStatus == vDevelopment) {
            newDev->setFileAccess(std::make_shared<IosFileAccess>(newDev->uniqueInternalDeviceId()));
            DeviceManager::setDeviceState(newDev->id(), IDevice::DeviceReadyToUse);
            m_userModeDeviceIds.removeOne(uid);
        } else {
            DeviceManager::setDeviceState(newDev->id(), IDevice::DeviceConnected);
            bool shouldIgnore = newDev->m_ignoreDevice;
            newDev->m_ignoreDevice = true;
            if (devStatus == vOff) {
                if (!m_devModeDialog && !shouldIgnore && !IosConfigurations::ignoreAllDevices()) {
                    m_devModeDialog = new QMessageBox(Core::ICore::dialogParent());
                    m_devModeDialog->setText(
                        Tr::tr("An iOS device in user mode has been detected."));
                    m_devModeDialog->setInformativeText(
                        Tr::tr("Do you want to see how to set it up for development?"));
                    m_devModeDialog->setStandardButtons(QMessageBox::NoAll | QMessageBox::No
                                                        | QMessageBox::Yes);
                    m_devModeDialog->setDefaultButton(QMessageBox::Yes);
                    m_devModeDialog->setAttribute(Qt::WA_DeleteOnClose);
                    connect(m_devModeDialog, &QDialog::finished, this, [](int result) {
                        switch (result) {
                        case QMessageBox::Yes:
                            Core::HelpManager::showHelpUrl(
                                "qthelp://org.qt-project.qtcreator/doc/"
                                "creator-how-to-connect-ios-devices.html");
                            break;
                        case QMessageBox::No:
                            break;
                        case QMessageBox::NoAll:
                            IosConfigurations::setIgnoreAllDevices(true);
                            break;
                        default:
                            break;
                        }
                    });
                    m_devModeDialog->show();
                }
            }
            if (!m_userModeDeviceIds.contains(uid))
                m_userModeDeviceIds.append(uid);
            m_userModeDevicesTimer.start();
        }
    }
}

#ifdef Q_OS_MAC
namespace {
io_iterator_t gAddedIter;
io_iterator_t gRemovedIter;

extern "C" {
void deviceConnectedCallback(void *refCon, io_iterator_t iterator)
{
    try {
        kern_return_t       kr;
        io_service_t        usbDevice;
        (void) refCon;

        while ((usbDevice = IOIteratorNext(iterator))) {
            io_name_t       deviceName;

            // Get the USB device's name.
            kr = IORegistryEntryGetName(usbDevice, deviceName);
            QString name;
            if (KERN_SUCCESS == kr)
                name = QString::fromLocal8Bit(deviceName);
            qCDebug(detectLog) << "ios device " << name << " in deviceAddedCallback";

            CFStringRef cfUid = static_cast<CFStringRef>(IORegistryEntryCreateCFProperty(
                                                             usbDevice,
                                                             CFSTR(kUSBSerialNumberString),
                                                             kCFAllocatorDefault, 0));
            if (cfUid) {
                QString uid = CFStringRef2QString(cfUid);
                CFRelease(cfUid);
                qCDebug(detectLog) << "device UID is" << uid;
                IosDeviceManager::instance()->deviceConnected(uid, name);
            } else {
                qCDebug(detectLog) << "failed to retrieve device's UID";
            }

            // Done with this USB device; release the reference added by IOIteratorNext
            kr = IOObjectRelease(usbDevice);
        }
    }
    catch (const std::exception &e) {
        qCWarning(detectLog) << "Exception " << e.what() << " in iosdevice.cpp deviceConnectedCallback";
    }
    catch (...) {
        qCWarning(detectLog) << "Exception in iosdevice.cpp deviceConnectedCallback";
        throw;
    }
}

void deviceDisconnectedCallback(void *refCon, io_iterator_t iterator)
{
    try {
        kern_return_t       kr;
        io_service_t        usbDevice;
        (void) refCon;

        while ((usbDevice = IOIteratorNext(iterator))) {
            io_name_t       deviceName;

            // Get the USB device's name.
            kr = IORegistryEntryGetName(usbDevice, deviceName);
            QString name;
            if (KERN_SUCCESS == kr)
                name = QString::fromLocal8Bit(deviceName);
            qCDebug(detectLog) << "ios device " << name << " in deviceDisconnectedCallback";

            CFStringRef cfUid = static_cast<CFStringRef>(
                IORegistryEntryCreateCFProperty(usbDevice,
                                                CFSTR(kUSBSerialNumberString),
                                                kCFAllocatorDefault,
                                                0));
            if (cfUid) {
                QString uid = CFStringRef2QString(cfUid);
                CFRelease(cfUid);
                IosDeviceManager::instance()->deviceDisconnected(uid);
            } else {
                qCDebug(detectLog) << "failed to retrieve device's UID";
            }

            // Done with this USB device; release the reference added by IOIteratorNext
            kr = IOObjectRelease(usbDevice);
        }
    }
    catch (const std::exception &e) {
        qCWarning(detectLog) << "Exception " << e.what() << " in iosdevice.cpp deviceDisconnectedCallback";
    }
    catch (...) {
        qCWarning(detectLog) << "Exception in iosdevice.cpp deviceDisconnectedCallback";
        throw;
    }
}

} // extern C

} // anonymous namespace
#endif

void IosDeviceManager::monitorAvailableDevices()
{
#ifdef Q_OS_MAC
    CFMutableDictionaryRef  matchingDictionary =
                                        IOServiceMatching("IOUSBDevice" );
    {
        UInt32 vendorId = 0x05ac;
        CFNumberRef cfVendorValue = CFNumberCreate( kCFAllocatorDefault, kCFNumberSInt32Type,
                                                    &vendorId );
        CFDictionaryAddValue( matchingDictionary, CFSTR( kUSBVendorID ), cfVendorValue);
        CFRelease( cfVendorValue );
        UInt32 productId = 0x1280;
        CFNumberRef cfProductIdValue = CFNumberCreate( kCFAllocatorDefault, kCFNumberSInt32Type,
                                                       &productId );
        CFDictionaryAddValue( matchingDictionary, CFSTR( kUSBProductID ), cfProductIdValue);
        CFRelease( cfProductIdValue );
        UInt32 productIdMask = 0xFFC0;
        CFNumberRef cfProductIdMaskValue = CFNumberCreate( kCFAllocatorDefault, kCFNumberSInt32Type,
                                                           &productIdMask );
        CFDictionaryAddValue( matchingDictionary, CFSTR( kUSBProductIDMask ), cfProductIdMaskValue);
        CFRelease( cfProductIdMaskValue );
    }

#if QT_MACOS_DEPLOYMENT_TARGET_BELOW(120000)
    const mach_port_t port = kIOMasterPortDefault; // deprecated in macOS 12
#else
    const mach_port_t port = kIOMainPortDefault; // available since macOS 12
#endif
    IONotificationPortRef notificationPort = IONotificationPortCreate(port);
    CFRunLoopSourceRef runLoopSource = IONotificationPortGetRunLoopSource(notificationPort);

    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);

    // IOServiceAddMatchingNotification releases this, so we retain for the next call
    CFRetain(matchingDictionary);

    // Now set up a notification to be called when a device is first matched by I/O Kit.
    IOServiceAddMatchingNotification(notificationPort,
                                     kIOMatchedNotification,
                                     matchingDictionary,
                                     deviceConnectedCallback,
                                     NULL,
                                     &gAddedIter);

    IOServiceAddMatchingNotification(notificationPort,
                                     kIOTerminatedNotification,
                                     matchingDictionary,
                                     deviceDisconnectedCallback,
                                     NULL,
                                     &gRemovedIter);

    // Iterate once to get already-present devices and arm the notification
    deviceConnectedCallback(NULL, gAddedIter);
    deviceDisconnectedCallback(NULL, gRemovedIter);
#endif
}

bool IosDeviceManager::isDeviceCtlOutputSupported()
{
    if (qtcEnvironmentVariableIsSet("QTC_FORCE_POLLINGIOSRUNNER"))
        return false;
    // Theoretically the devicectl from Xcode 15.4 already has the required `--console` option,
    // but that is broken for some (newer?) devices (QTCREATORBUG-32637).
    return instance()->m_deviceCtlVersion
           && instance()->m_deviceCtlVersion >= QVersionNumber(397, 21); // Xcode 16.0
}

bool IosDeviceManager::isDeviceCtlDebugSupported()
{
    if (qtcEnvironmentVariableIsSet("QTC_FORCE_POLLINGIOSRUNNER"))
        return false;
    // TODO this actually depends on a kit with LLDB >= lldb-1600.0.36.3 (Xcode 16.0)
    // and devicectl >= 355.28 (Xcode 15.4) already has the devicectl requirements
    // In principle users could install Xcode 16, and get devicectl >= 397.21 from that
    // (it is globally installed in /Library/...)
    // but then switch to an Xcode 15 installation with xcode-select, and use lldb-1500 which does
    // not support the required commands.
    return instance()->m_deviceCtlVersion
           && instance()->m_deviceCtlVersion >= QVersionNumber(397, 21); // Xcode 16.0
}

IosDeviceManager::IosDeviceManager(QObject *parent) :
    QObject(parent)
{
    m_userModeDevicesTimer.setSingleShot(true);
    m_userModeDevicesTimer.setInterval(8000);
    connect(&m_userModeDevicesTimer, &QTimer::timeout,
            this, &IosDeviceManager::updateUserModeDevices);
}

void IosDeviceManager::updateUserModeDevices()
{
    for (const QString &uid : std::as_const(m_userModeDeviceIds))
        updateInfo(uid);
}

IosDeviceManager *IosDeviceManager::instance()
{
    static IosDeviceManager *theInstance = new IosDeviceManager(Utils::shutdownGuard());
    return theInstance;
}

void IosDeviceManager::updateAvailableDevices(const QStringList &devices)
{
    for (const QString &uid : devices)
        deviceConnected(uid);

    for (int iDevice = 0; iDevice < DeviceManager::deviceCount(); ++iDevice) {
        IDevice::ConstPtr dev = DeviceManager::deviceAt(iDevice);
        Utils::Id devType(Constants::IOS_DEVICE_TYPE);
        if (!dev || dev->type() != devType)
            continue;
        auto iosDev = static_cast<const IosDevice *>(dev.get());
        if (devices.contains(iosDev->uniqueDeviceID()))
            continue;
        if (iosDev->deviceState() != IDevice::DeviceDisconnected) {
            qCDebug(detectLog) << "disconnecting device " << iosDev->uniqueDeviceID();
            DeviceManager::setDeviceState(iosDev->id(), IDevice::DeviceDisconnected);
        }
    }
}

// Factory

class IosDeviceFactory final : public IDeviceFactory
{
public:
    IosDeviceFactory()
        : IDeviceFactory(Constants::IOS_DEVICE_TYPE)
    {
        setDisplayName(IosDevice::name());
        setCombinedIcon(":/ios/images/iosdevicesmall.png",
                        ":/ios/images/iosdevice.png");
        setConstructionFunction([] { return IDevice::Ptr(new IosDevice); });
        setExecutionTypeId(Constants::IOS_EXECUTION_TYPE_ID);
    }

    bool canRestore(const Utils::Store &map) const override
    {
        Store vMap = map.value(Constants::EXTRA_INFO_KEY).value<Store>();
        if (vMap.isEmpty() || vMap.value(kDeviceName).toString() == QLatin1String("*unknown*"))
            return false; // transient device (probably generated during an activation)
        return true;
    }
};

void setupIosDevice()
{
    static IosDeviceFactory theIosDeviceFactory;
}

} // Ios::Internal
