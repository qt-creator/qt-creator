// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2015 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author David Faure <david.faure@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qplatformdefs.h> // always first

#include "mimedatabase.h"
#include "mimedatabase_p.h"

#include "mimeprovider_p.h"
#include "mimetype_p.h"
#include "mimeutils.h"

#include "algorithm.h"

#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QBuffer>
#include <QUrl>
#include <QDebug>

#include <algorithm>
#include <functional>
#include <stack>

namespace Utils {

Q_GLOBAL_STATIC(MimeDatabasePrivate, staticMimeDatabase)

MimeDatabasePrivate *MimeDatabasePrivate::instance()
{
    return staticMimeDatabase();
}

MimeDatabasePrivate::MimeDatabasePrivate()
    : m_defaultMimeType(QLatin1String("application/octet-stream"))
{
}

MimeDatabasePrivate::~MimeDatabasePrivate()
{
}

#if 0
#if 0 //def QT_BUILD_INTERNAL
Q_CORE_EXPORT
#else
static const
#endif
int mime_secondsBetweenChecks = 5;
#endif

bool MimeDatabasePrivate::shouldCheck()
{
#if 0
    if (m_lastCheck.isValid() && m_lastCheck.elapsed() < mime_secondsBetweenChecks * 1000)
        return false;
    m_lastCheck.start();
    return true;
#endif
    // Qt Creator forces reload manually
    return m_forceLoad;
}

#if defined(Q_OS_UNIX) && !defined(Q_OS_NACL) && !defined(Q_OS_INTEGRITY)
#  define QT_USE_MMAP
#endif

static void updateOverriddenMimeTypes(std::vector<std::unique_ptr<MimeProviderBase>> &providers)
{
    // If a provider earlier in the list already defines a mimetype, it should override the
    // mimetype definition of following providers. Go through everything once here, telling each
    // provider which mimetypes are overridden by earlier providers.
    QList<MimeProviderBase *> handledProviders;
    for (std::unique_ptr<MimeProviderBase> &provider : providers) {
        provider->m_overriddenMimeTypes.clear();
        const QStringList ownMimetypes = provider->allMimeTypeNames();
        for (MimeProviderBase *other : handledProviders) {
            const QStringList overridden = Utils::filtered(ownMimetypes,
                                                           [other](const QString &type) {
                                                               return other->hasMimeTypeForName(
                                                                   type);
                                                           });
            provider->m_overriddenMimeTypes.unite(QSet(overridden.cbegin(), overridden.cend()));
        }
        handledProviders.append(provider.get());
    }
}

void MimeDatabasePrivate::loadProviders()
{
#if 0
    // We use QStandardPaths every time to check if new files appeared
    const QStringList mimeDirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QLatin1String("mime"), QStandardPaths::LocateDirectory);
#else
    // Qt Creator never uses the standard paths, they can conflict with our setup
    const QStringList mimeDirs;
#endif
    const auto fdoIterator = std::find_if(mimeDirs.constBegin(), mimeDirs.constEnd(), [](const QString &mimeDir) -> bool {
        return QFileInfo::exists(mimeDir + QStringLiteral("/packages/freedesktop.org.xml")); }
    );
    const bool needInternalDB = MimeXMLProvider::InternalDatabaseAvailable && fdoIterator == mimeDirs.constEnd();
    //qDebug() << "mime dirs:" << mimeDirs;

    Providers currentProviders;
    std::swap(m_providers, currentProviders);

    m_providers.reserve(m_additionalData.size() + mimeDirs.size() + (needInternalDB ? 1 : 0));

    // added for Qt Creator: additional mime data
    for (auto dataIt = m_additionalData.cbegin(); dataIt != m_additionalData.cend(); ++dataIt) {
        // Check if we already have a provider for this data
        const QString id = dataIt.key();
        const auto it = std::find_if(currentProviders.begin(), currentProviders.end(),
                                     [id](const std::unique_ptr<MimeProviderBase> &prov) {
                                         return prov && prov->directory() == id;
                                     });
        std::unique_ptr<MimeProviderBase> provider;
        if (it != currentProviders.end())
            provider = std::move(*it); // take provider out of the vector
        provider.reset(new MimeXMLProvider(this, id, dataIt.value()));
        m_providers.push_back(std::move(provider));
    }


    for (const QString &mimeDir : mimeDirs) {
        const QString cacheFile = mimeDir + QStringLiteral("/mime.cache");
        // Check if we already have a provider for this dir
        const auto predicate = [mimeDir](const std::unique_ptr<MimeProviderBase> &prov)
        {
            return prov && prov->directory() == mimeDir;
        };
        const auto it = std::find_if(currentProviders.begin(), currentProviders.end(), predicate);
        if (it == currentProviders.end()) {
            std::unique_ptr<MimeProviderBase> provider;
#if defined(QT_USE_MMAP)
            if (qEnvironmentVariableIsEmpty("QT_NO_MIME_CACHE") && QFileInfo::exists(cacheFile)) {
                provider.reset(new MimeBinaryProvider(this, mimeDir));
                //qDebug() << "Created binary provider for" << mimeDir;
                if (!provider->isValid()) {
                    provider.reset();
                }
            }
#endif
            if (!provider) {
                provider.reset(new MimeXMLProvider(this, mimeDir));
                //qDebug() << "Created XML provider for" << mimeDir;
            }
            m_providers.push_back(std::move(provider));
        } else {
            auto provider = std::move(*it); // take provider out of the vector
            provider->ensureLoaded();
            if (!provider->isValid()) {
                provider.reset(new MimeXMLProvider(this, mimeDir));
                //qDebug() << "Created XML provider to replace binary provider for" << mimeDir;
            }
            m_providers.push_back(std::move(provider));
        }
    }
    // mimeDirs is sorted "most local first, most global last"
    // so the internal XML DB goes at the end
    if (needInternalDB) {
        // Check if we already have a provider for the InternalDatabase
        const auto isInternal = [](const std::unique_ptr<MimeProviderBase> &prov)
        {
            return prov && prov->isInternalDatabase();
        };
        const auto it = std::find_if(currentProviders.begin(), currentProviders.end(), isInternal);
        if (it == currentProviders.end()) {
            m_providers.push_back(Providers::value_type(new MimeXMLProvider(this, MimeXMLProvider::InternalDatabase)));
        } else {
            m_providers.push_back(std::move(*it));
        }
    }

    updateOverriddenMimeTypes(m_providers);
}

const MimeDatabasePrivate::Providers &MimeDatabasePrivate::providers()
{
#ifndef Q_OS_WASM // stub implementation always returns true
    Q_ASSERT(!mutex.tryLock()); // caller should have locked mutex
#endif
    if (m_providers.empty()) {
        loadProviders();
        // Qt Creator forces reload manually
        // m_lastCheck.start();
    } else {
        if (shouldCheck())
            loadProviders();
    }
    // Qt Creator forces reload manually
    m_forceLoad = false;
    return m_providers;
}

QString MimeDatabasePrivate::resolveAlias(const QString &nameOrAlias)
{
    for (const auto &provider : providers()) {
        const QString ret = provider->resolveAlias(nameOrAlias);
        if (!ret.isEmpty())
            return ret;
    }
    return nameOrAlias;
}

/*!
    \internal
    Returns a MIME type or an invalid one if none found
 */
MimeType MimeDatabasePrivate::mimeTypeForName(const QString &nameOrAlias)
{
    const QString mimeName = resolveAlias(nameOrAlias);
    for (const auto &provider : providers()) {
        const MimeType mime = provider->mimeTypeForName(mimeName);
        if (mime.isValid())
            return mime;
    }
    return {};
}

QStringList MimeDatabasePrivate::mimeTypeForFileName(const QString &fileName)
{
    if (fileName.endsWith(QLatin1Char('/')))
        return QStringList() << QLatin1String("inode/directory");

    const MimeGlobMatchResult result = findByFileName(fileName);
    QStringList matchingMimeTypes = result.m_matchingMimeTypes;
    matchingMimeTypes.sort(); // make it deterministic
    return matchingMimeTypes;
}

MimeGlobMatchResult MimeDatabasePrivate::findByFileName(const QString &fileName)
{
    MimeGlobMatchResult result;
    const QString fileNameExcludingPath = QFileInfo(fileName).fileName();
    for (const auto &provider : providers())
        provider->addFileNameMatches(fileNameExcludingPath, result);
    return result;
}

void MimeDatabasePrivate::loadMimeTypePrivate(MimeTypePrivate &mimePrivate)
{
    QMutexLocker locker(&mutex);
    if (mimePrivate.name.isEmpty())
        return; // invalid mimetype
    if (!mimePrivate.loaded) { // XML provider sets loaded=true, binary provider does this on demand
        Q_ASSERT(mimePrivate.fromCache);
        bool found = false;
        for (const auto &provider : providers()) {
            if (provider->loadMimeTypePrivate(mimePrivate)) {
                found = true;
                break;
            }
        }
        if (!found) {
            const QString file = mimePrivate.name + QLatin1String(".xml");
            qWarning() << "No file found for" << file << ", even though update-mime-info said it would exist.\n"
                          "Either it was just removed, or the directory doesn't have executable permission..."
                       << QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QLatin1String("mime"), QStandardPaths::LocateDirectory);
        }
        mimePrivate.loaded = true;
    }
}

void MimeDatabasePrivate::loadGenericIcon(MimeTypePrivate &mimePrivate)
{
    QMutexLocker locker(&mutex);
    if (mimePrivate.fromCache) {
        mimePrivate.genericIconName.clear();
        for (const auto &provider : providers()) {
            provider->loadGenericIcon(mimePrivate);
            if (!mimePrivate.genericIconName.isEmpty())
                break;
        }
    }
}

void MimeDatabasePrivate::loadIcon(MimeTypePrivate &mimePrivate)
{
    QMutexLocker locker(&mutex);
    if (mimePrivate.fromCache) {
        mimePrivate.iconName.clear();
        for (const auto &provider : providers()) {
            provider->loadIcon(mimePrivate);
            if (!mimePrivate.iconName.isEmpty())
                break;
        }
    }
}

static QString fallbackParent(const QString &mimeTypeName)
{
    const QStringView myGroup = QStringView{mimeTypeName}.left(mimeTypeName.indexOf(QLatin1Char('/')));
    // All text/* types are subclasses of text/plain.
    if (myGroup == QLatin1String("text") && mimeTypeName != QLatin1String("text/plain"))
        return QLatin1String("text/plain");
    // All real-file mimetypes implicitly derive from application/octet-stream
    if (myGroup != QLatin1String("inode") &&
        // ignore non-file extensions
        myGroup != QLatin1String("all") && myGroup != QLatin1String("fonts") && myGroup != QLatin1String("print") && myGroup != QLatin1String("uri")
        && mimeTypeName != QLatin1String("application/octet-stream")) {
        return QLatin1String("application/octet-stream");
    }
    return QString();
}

QStringList MimeDatabasePrivate::mimeParents(const QString &mimeName)
{
    QMutexLocker locker(&mutex);
    return parents(mimeName);
}

QStringList MimeDatabasePrivate::parents(const QString &mimeName)
{
    Q_ASSERT(!mutex.tryLock());
    QStringList result;
    for (const auto &provider : providers()) {
        if (provider->hasMimeTypeForName(mimeName)) {
            provider->addParents(mimeName, result);
            break;
        }
    }
    if (result.isEmpty()) {
        const QString parent = fallbackParent(mimeName);
        if (!parent.isEmpty())
            result.append(parent);
    }
    return result;
}

QStringList MimeDatabasePrivate::listAliases(const QString &mimeName)
{
    QMutexLocker locker(&mutex);
    QStringList result;
    for (const auto &provider : providers()) {
        if (provider->hasMimeTypeForName(mimeName)) {
            provider->addAliases(mimeName, result);
            return result;
        }
    }
    return result;
}

bool MimeDatabasePrivate::mimeInherits(const QString &mime, const QString &parent)
{
    QMutexLocker locker(&mutex);
    return inherits(mime, parent);
}

static inline bool isTextFile(const QByteArray &data)
{
    // UTF16 byte order marks
    static const char bigEndianBOM[] = "\xFE\xFF";
    static const char littleEndianBOM[] = "\xFF\xFE";
    if (data.startsWith(bigEndianBOM) || data.startsWith(littleEndianBOM))
        return true;

    // Check the first 128 bytes (see shared-mime spec)
    const char *p = data.constData();
    const char *e = p + qMin(128, data.size());
    for ( ; p < e; ++p) {
        if (static_cast<unsigned char>(*p) < 32 && *p != 9 && *p !=10 && *p != 13)
            return false;
    }

    return true;
}

MimeType MimeDatabasePrivate::findByData(const QByteArray &data, int *accuracyPtr)
{
#if 0
    if (data.isEmpty()) {
        *accuracyPtr = 100;
        return mimeTypeForName(QLatin1String("application/x-zerosize"));
    }
#endif

    *accuracyPtr = 0;
    MimeType candidate;
    for (const auto &provider : providers())
        provider->findByMagic(data, accuracyPtr, candidate);

    if (candidate.isValid())
        return candidate;

    if (isTextFile(data)) {
        *accuracyPtr = 5;
        return mimeTypeForName(QLatin1String("text/plain"));
    }

    return mimeTypeForName(defaultMimeType());
}

MimeType MimeDatabasePrivate::mimeTypeForFileNameAndData(const QString &fileName, QIODevice *device, int *accuracyPtr)
{
    // First, glob patterns are evaluated. If there is a match with max weight,
    // this one is selected and we are done. Otherwise, the file contents are
    // evaluated and the match with the highest value (either a magic priority or
    // a glob pattern weight) is selected. Matching starts from max level (most
    // specific) in both cases, even when there is already a suffix matching candidate.
    *accuracyPtr = 0;

    // Pass 1) Try to match on the file name
    MimeGlobMatchResult candidatesByName = findByFileName(fileName);
    if (candidatesByName.m_allMatchingMimeTypes.count() == 1) {
        *accuracyPtr = 100;
        const MimeType mime = mimeTypeForName(candidatesByName.m_matchingMimeTypes.at(0));
        if (mime.isValid())
            return mime;
        candidatesByName = {};
    }

    // Extension is unknown, or matches multiple mimetypes.
    // Pass 2) Match on content, if we can read the data
    const auto matchOnContent = [this, accuracyPtr, &candidatesByName](QIODevice *device) {
        if (device->isOpen()) {
            // Read 16K in one go (QIODEVICE_BUFFERSIZE in qiodevice_p.h).
            // This is much faster than seeking back and forth into QIODevice.
            const QByteArray data = device->peek(16384);

            int magicAccuracy = 0;
            MimeType candidateByData(findByData(data, &magicAccuracy));

            // Disambiguate conflicting extensions (if magic matching found something)
            if (candidateByData.isValid() && magicAccuracy > 0) {
                const QString sniffedMime = candidateByData.name();
                // If the sniffedMime matches a highest-weight glob match, use it
                if (candidatesByName.m_matchingMimeTypes.contains(sniffedMime)) {
                    *accuracyPtr = 100;
                    return candidateByData;
                }
                for (const QString &m : std::as_const(candidatesByName.m_allMatchingMimeTypes)) {
                    if (inherits(m, sniffedMime)) {
                        // We have magic + pattern pointing to this, so it's a pretty good match
                        *accuracyPtr = 100;
                        return mimeTypeForName(m);
                    }
                }
                if (candidatesByName.m_allMatchingMimeTypes.isEmpty()) {
                    // No glob, use magic
                    *accuracyPtr = magicAccuracy;
                    return candidateByData;
                }
            }
        }

        if (candidatesByName.m_allMatchingMimeTypes.count() > 1) {
            candidatesByName.m_matchingMimeTypes.sort(); // make it deterministic
            *accuracyPtr = 20;
            const MimeType mime = mimeTypeForName(candidatesByName.m_matchingMimeTypes.at(0));
            if (mime.isValid())
                return mime;
        }

        return mimeTypeForName(defaultMimeType());
    };

    if (device)
        return matchOnContent(device);

    QFile fallbackFile(fileName);
    fallbackFile.open(QIODevice::ReadOnly); // error handling: matchOnContent() will check isOpen()
    return matchOnContent(&fallbackFile);
}

QList<MimeType> MimeDatabasePrivate::allMimeTypes()
{
    QList<MimeType> result;
    for (const auto &provider : providers())
        provider->addAllMimeTypes(result);
    return result;
}

bool MimeDatabasePrivate::inherits(const QString &mime, const QString &parent)
{
    const QString resolvedParent = resolveAlias(parent);
    std::stack<QString, QStringList> toCheck;
    toCheck.push(mime);
    while (!toCheck.empty()) {
        if (toCheck.top() == resolvedParent)
            return true;
        const QString mimeName = toCheck.top();
        toCheck.pop();
        const auto parentList = parents(mimeName);
        for (const QString &par : parentList)
            toCheck.push(resolveAlias(par));
    }
    return false;
}

/*!
    \class MimeDatabase
    \inmodule QtCore
    \brief The MimeDatabase class maintains a database of MIME types.

    \since 5.0

    The MIME type database is provided by the freedesktop.org shared-mime-info
    project. If the MIME type database cannot be found on the system, as is the case
    on most Windows, \macos, and iOS systems, Qt will use its own copy of it.

    Applications which want to define custom MIME types need to install an
    XML file into the locations searched for MIME definitions.
    These locations can be queried with
    \snippet code/src_corelib_mimetype_mimedatabase.cpp 1
    On a typical Unix system, this will be /usr/share/mime/packages/, but it is also
    possible to extend the list of directories by setting the environment variable
    \c XDG_DATA_DIRS. For instance adding /opt/myapp/share to \c XDG_DATA_DIRS will result
    in /opt/myapp/share/mime/packages/ being searched for MIME definitions.

    Here is an example of MIME XML:
    \snippet code/src_corelib_mimetype_mimedatabase.cpp 2

    For more details about the syntax of XML MIME definitions, including defining
    "magic" in order to detect MIME types based on data as well, read the
    Shared Mime Info specification at
    http://standards.freedesktop.org/shared-mime-info-spec/shared-mime-info-spec-latest.html

    On Unix systems, a binary cache is used for more performance. This cache is generated
    by the command "update-mime-database path", where path would be /opt/myapp/share/mime
    in the above example. Make sure to run this command when installing the MIME type
    definition file.

    \threadsafe

    \snippet code/src_corelib_mimetype_mimedatabase.cpp 0

    \sa MimeType, {MIME Type Browser Example}
 */

/*!
    \fn MimeDatabase::MimeDatabase();
    Constructs a MimeDatabase object.

    It is perfectly OK to create an instance of MimeDatabase every time you need to
    perform a lookup.
    The parsing of mimetypes is done on demand (when shared-mime-info is installed)
    or when the very first instance is constructed (when parsing XML files directly).
 */
MimeDatabase::MimeDatabase() :
        d(staticMimeDatabase())
{
}

/*!
    \fn MimeDatabase::~MimeDatabase();
    Destroys the MimeDatabase object.
 */
MimeDatabase::~MimeDatabase()
{
    d = nullptr;
}

/*!
    \fn MimeType MimeDatabase::mimeTypeForName(const QString &nameOrAlias) const;
    Returns a MIME type for \a nameOrAlias or an invalid one if none found.
 */
MimeType MimeDatabase::mimeTypeForName(const QString &nameOrAlias) const
{
    QMutexLocker locker(&d->mutex);

    if (d->m_startupPhase <= int(MimeStartupPhase::PluginsInitializing))
        qWarning("Accessing MimeDatabase for %s before plugins are initialized",
                 qPrintable(nameOrAlias));

    return d->mimeTypeForName(nameOrAlias);
}

/*!
    Returns a MIME type for \a fileInfo.

    A valid MIME type is always returned.

    The default matching algorithm looks at both the file name and the file
    contents, if necessary. The file extension has priority over the contents,
    but the contents will be used if the file extension is unknown, or
    matches multiple MIME types.
    If \a fileInfo is a Unix symbolic link, the file that it refers to
    will be used instead.
    If the file doesn't match any known pattern or data, the default MIME type
    (application/octet-stream) is returned.

    When \a mode is set to MatchExtension, only the file name is used, not
    the file contents. The file doesn't even have to exist. If the file name
    doesn't match any known pattern, the default MIME type (application/octet-stream)
    is returned.
    If multiple MIME types match this file, the first one (alphabetically) is returned.

    When \a mode is set to MatchContent, and the file is readable, only the
    file contents are used to determine the MIME type. This is equivalent to
    calling mimeTypeForData with a QFile as input device.

    \a fileInfo may refer to an absolute or relative path.

    \sa MimeType::isDefault(), mimeTypeForData()
*/
MimeType MimeDatabase::mimeTypeForFile(const QFileInfo &fileInfo, MatchMode mode) const
{
    QMutexLocker locker(&d->mutex);

    if (d->m_startupPhase <= int(MimeStartupPhase::PluginsInitializing))
        qWarning("Accessing MimeDatabase for %s before plugins are initialized",
                 qPrintable(fileInfo.filePath()));

    if (fileInfo.isDir())
        return d->mimeTypeForName(QLatin1String("inode/directory"));

    const QString filePath = fileInfo.filePath();

#ifdef Q_OS_UNIX
    // Cannot access statBuf.st_mode from the filesystem engine, so we have to stat again.
    // In addition we want to follow symlinks.
    const QByteArray nativeFilePath = QFile::encodeName(filePath);
    QT_STATBUF statBuffer;
    if (QT_STAT(nativeFilePath.constData(), &statBuffer) == 0) {
        if (S_ISCHR(statBuffer.st_mode))
            return d->mimeTypeForName(QLatin1String("inode/chardevice"));
        if (S_ISBLK(statBuffer.st_mode))
            return d->mimeTypeForName(QLatin1String("inode/blockdevice"));
        if (S_ISFIFO(statBuffer.st_mode))
            return d->mimeTypeForName(QLatin1String("inode/fifo"));
        if (S_ISSOCK(statBuffer.st_mode))
            return d->mimeTypeForName(QLatin1String("inode/socket"));
    }
#endif

    int priority = 0;
    switch (mode) {
    case MatchDefault:
        return d->mimeTypeForFileNameAndData(filePath, nullptr, &priority);
    case MatchExtension:
        locker.unlock();
        return mimeTypeForFile(filePath, mode);
    case MatchContent: {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly)) {
            locker.unlock();
            return mimeTypeForData(&file);
        } else {
            return d->mimeTypeForName(d->defaultMimeType());
        }
    }
    default:
        Q_ASSERT(false);
    }
    return d->mimeTypeForName(d->defaultMimeType());
}

/*!
    Returns a MIME type for the file named \a fileName using \a mode.

    \overload
*/
MimeType MimeDatabase::mimeTypeForFile(const QString &fileName, MatchMode mode) const
{
    if (mode == MatchExtension) {
        QMutexLocker locker(&d->mutex);

        if (d->m_startupPhase <= int(MimeStartupPhase::PluginsInitializing))
            qWarning("Accessing MimeDatabase for %s before plugins are initialized",
                     qPrintable(fileName));

        const QStringList matches = d->mimeTypeForFileName(fileName);
        const int matchCount = matches.count();
        if (matchCount == 0) {
            return d->mimeTypeForName(d->defaultMimeType());
        } else if (matchCount == 1) {
            return d->mimeTypeForName(matches.first());
        } else {
            // We have to pick one.
            return d->mimeTypeForName(matches.first());
        }
    } else {
        // Implemented as a wrapper around mimeTypeForFile(QFileInfo), so no mutex.
        QFileInfo fileInfo(fileName);
        return mimeTypeForFile(fileInfo, mode);
    }
}

/*!
    Returns the MIME types for the file name \a fileName.

    If the file name doesn't match any known pattern, an empty list is returned.
    If multiple MIME types match this file, they are all returned.

    This function does not try to open the file. To also use the content
    when determining the MIME type, use mimeTypeForFile() or
    mimeTypeForFileNameAndData() instead.

    \sa mimeTypeForFile()
*/
QList<MimeType> MimeDatabase::mimeTypesForFileName(const QString &fileName) const
{
    QMutexLocker locker(&d->mutex);

    if (d->m_startupPhase <= int(MimeStartupPhase::PluginsInitializing))
        qWarning("Accessing MimeDatabase for %s before plugins are initialized",
                 qPrintable(fileName));

    const QStringList matches = d->mimeTypeForFileName(fileName);
    QList<MimeType> mimes;
    mimes.reserve(matches.count());
    for (const QString &mime : matches)
        mimes.append(d->mimeTypeForName(mime));
    return mimes;
}
/*!
    Returns the suffix for the file \a fileName, as known by the MIME database.

    This allows to pre-select "tar.bz2" for foo.tar.bz2, but still only
    "txt" for my.file.with.dots.txt.
*/
QString MimeDatabase::suffixForFileName(const QString &fileName) const
{
    QMutexLocker locker(&d->mutex);
    const int suffixLength = d->findByFileName(fileName).m_knownSuffixLength;
    return fileName.right(suffixLength);
}

/*!
    Returns a MIME type for \a data.

    A valid MIME type is always returned. If \a data doesn't match any
    known MIME type data, the default MIME type (application/octet-stream)
    is returned.
*/
MimeType MimeDatabase::mimeTypeForData(const QByteArray &data) const
{
    QMutexLocker locker(&d->mutex);

    if (d->m_startupPhase <= int(MimeStartupPhase::PluginsInitializing))
        qWarning("Accessing MimeDatabase for data before plugins are initialized");

    int accuracy = 0;
    return d->findByData(data, &accuracy);
}

/*!
    Returns a MIME type for the data in \a device.

    A valid MIME type is always returned. If the data in \a device doesn't match any
    known MIME type data, the default MIME type (application/octet-stream)
    is returned.
*/
MimeType MimeDatabase::mimeTypeForData(QIODevice *device) const
{
    QMutexLocker locker(&d->mutex);

    int accuracy = 0;
    const bool openedByUs = !device->isOpen() && device->open(QIODevice::ReadOnly);
    if (device->isOpen()) {
        // Read 16K in one go (QIODEVICE_BUFFERSIZE in qiodevice_p.h).
        // This is much faster than seeking back and forth into QIODevice.
        const QByteArray data = device->peek(16384);
        const MimeType result = d->findByData(data, &accuracy);
        if (openedByUs)
            device->close();
        return result;
    }
    return d->mimeTypeForName(d->defaultMimeType());
}

/*!
    Returns a MIME type for \a url.

    If the URL is a local file, this calls mimeTypeForFile.

    Otherwise the matching is done based on the file name only,
    except for schemes where file names don't mean much, like HTTP.
    This method always returns the default mimetype for HTTP URLs,
    use QNetworkAccessManager to handle HTTP URLs properly.

    A valid MIME type is always returned. If \a url doesn't match any
    known MIME type data, the default MIME type (application/octet-stream)
    is returned.
*/
MimeType MimeDatabase::mimeTypeForUrl(const QUrl &url) const
{
    if (url.isLocalFile())
        return mimeTypeForFile(url.toLocalFile());

    const QString scheme = url.scheme();
    if (scheme.startsWith(QLatin1String("http")) || scheme == QLatin1String("mailto"))
        return mimeTypeForName(d->defaultMimeType());

    return mimeTypeForFile(url.path(), MatchExtension);
}

/*!
    Returns a MIME type for the given \a fileName and \a device data.

    This overload can be useful when the file is remote, and we started to
    download some of its data in a device. This allows to do full MIME type
    matching for remote files as well.

    If the device is not open, it will be opened by this function, and closed
    after the MIME type detection is completed.

    A valid MIME type is always returned. If \a device data doesn't match any
    known MIME type data, the default MIME type (application/octet-stream)
    is returned.

    This method looks at both the file name and the file contents,
    if necessary. The file extension has priority over the contents,
    but the contents will be used if the file extension is unknown, or
    matches multiple MIME types.
*/
MimeType MimeDatabase::mimeTypeForFileNameAndData(const QString &fileName, QIODevice *device) const
{
    QMutexLocker locker(&d->mutex);

    if (fileName.endsWith(QLatin1Char('/')))
        return d->mimeTypeForName(QLatin1String("inode/directory"));

    int accuracy = 0;
    const bool openedByUs = !device->isOpen() && device->open(QIODevice::ReadOnly);
    const MimeType result = d->mimeTypeForFileNameAndData(fileName, device, &accuracy);
    if (openedByUs)
        device->close();
    return result;
}

/*!
    Returns a MIME type for the given \a fileName and device \a data.

    This overload can be useful when the file is remote, and we started to
    download some of its data. This allows to do full MIME type matching for
    remote files as well.

    A valid MIME type is always returned. If \a data doesn't match any
    known MIME type data, the default MIME type (application/octet-stream)
    is returned.

    This method looks at both the file name and the file contents,
    if necessary. The file extension has priority over the contents,
    but the contents will be used if the file extension is unknown, or
    matches multiple MIME types.
*/
MimeType MimeDatabase::mimeTypeForFileNameAndData(const QString &fileName, const QByteArray &data) const
{
    QMutexLocker locker(&d->mutex);

    if (fileName.endsWith(QLatin1Char('/')))
        return d->mimeTypeForName(QLatin1String("inode/directory"));

    QBuffer buffer(const_cast<QByteArray *>(&data));
    buffer.open(QIODevice::ReadOnly);
    int accuracy = 0;
    return d->mimeTypeForFileNameAndData(fileName, &buffer, &accuracy);
}

/*!
    Returns the list of all available MIME types.

    This can be useful for showing all MIME types to the user, for instance
    in a MIME type editor. Do not use unless really necessary in other cases
    though, prefer using the  \l {mimeTypeForData()}{mimeTypeForXxx()} methods for performance reasons.
*/
QList<MimeType> MimeDatabase::allMimeTypes() const
{
    QMutexLocker locker(&d->mutex);

    if (d->m_startupPhase <= int(MimeStartupPhase::PluginsInitializing))
        qWarning("Accessing MimeDatabase for all mime types before plugins are initialized");

    return d->allMimeTypes();
}

/*!
    \enum MimeDatabase::MatchMode

    This enum specifies how matching a file to a MIME type is performed.

    \value MatchDefault Both the file name and content are used to look for a match

    \value MatchExtension Only the file name is used to look for a match

    \value MatchContent The file content is used to look for a match
*/

// added for Qt Creator
void MimeDatabasePrivate::addMimeData(const QString &id, const QByteArray &data)
{
    if (m_additionalData.contains(id))
        qWarning("Overwriting data in mime database, id '%s'", qPrintable(id));

    m_additionalData.insert(id, data);
    m_forceLoad = true;
}

QMap<int, QList<MimeMagicRule>> MimeDatabasePrivate::magicRulesForMimeType(const MimeType &mimeType)
{
    for (const auto &provider : providers()) {
        if (provider->hasMimeTypeForName(mimeType.name()))
            return provider->magicRulesForMimeType(mimeType);
    }
    return {};
}

void MimeDatabasePrivate::setMagicRulesForMimeType(const MimeType &mimeType,
                                                   const QMap<int, QList<MimeMagicRule>> &rules)
{
    for (const auto &provider : providers()) {
        if (provider->hasMimeTypeForName(mimeType.name())) {
            provider->setMagicRulesForMimeType(mimeType, rules);
            return;
        }
    }
}

void MimeDatabasePrivate::setGlobPatternsForMimeType(const MimeType &mimeType,
                                                     const QStringList &patterns)
{
    for (const auto &provider : providers()) {
        if (provider->hasMimeTypeForName(mimeType.name())) {
            provider->setGlobPatternsForMimeType(mimeType, patterns);
            return;
        }
    }
}

} // namespace Utils
