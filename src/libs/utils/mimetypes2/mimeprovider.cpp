// Copyright (C) 2018 The Qt Company Ltd.
// Copyright (C) 2018 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author David Faure <david.faure@kdab.com>
// Copyright (C) 2019 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "mimeprovider_p.h"

#include "mimetypeparser_p.h"
#include "mimemagicrulematcher_p.h"

#include <qstandardpaths.h>
#include <QXmlStreamReader>
#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QByteArrayMatcher>
#include <QDebug>
#include <QDateTime>
#include <QtEndian>

#if 0 // QT_CONFIG(mimetype_database)
#  if defined(Q_CC_MSVC_ONLY)
#    pragma section(".qtmimedatabase", read, shared)
__declspec(allocate(".qtmimedatabase")) __declspec(align(4096))
#  elif defined(Q_OS_DARWIN)
__attribute__((section("__TEXT,.qtmimedatabase"), aligned(4096)))
#  elif (defined(Q_OF_ELF) || defined(Q_OS_WIN)) && defined(Q_CC_GNU)
__attribute__((section(".qtmimedatabase"), aligned(4096)))
#  endif

#  include "qmimeprovider_database.cpp"

#  ifdef MIME_DATABASE_IS_ZSTD
#    if !QT_CONFIG(zstd)
#      error "MIME database is zstd but no support compiled in!"
#    endif
#    include <zstd.h>
#  endif
#  ifdef MIME_DATABASE_IS_GZIP
#    ifdef QT_NO_COMPRESS
#      error "MIME database is zlib but no support compiled in!"
#    endif
#    define ZLIB_CONST
#    include <zconf.h>
#    include <zlib.h>
#  endif
#endif

using namespace Qt::StringLiterals;

namespace Utils {

struct MimeBinaryProvider::CacheFile
{
    CacheFile(const QString &fileName);
    ~CacheFile();

    bool isValid() const { return m_valid; }
    inline quint16 getUint16(int offset) const
    {
        return qFromBigEndian(*reinterpret_cast<quint16 *>(data + offset));
    }
    inline quint32 getUint32(int offset) const
    {
        return qFromBigEndian(*reinterpret_cast<quint32 *>(data + offset));
    }
    inline const char *getCharStar(int offset) const
    {
        return reinterpret_cast<const char *>(data + offset);
    }
    bool load();
    bool reload();

    QFile file;
    uchar *data;
    QDateTime m_mtime;
    bool m_valid;
};

static inline void appendIfNew(QStringList &list, const QString &str)
{
    if (!list.contains(str))
        list.push_back(str);
}

MimeProviderBase::MimeProviderBase(MimeDatabasePrivate *db, const QString &directory)
    : m_db(db), m_directory(directory)
{
}

MimeProviderBase *MimeProviderBase::overrideProvider() const
{
    return m_overrideProvider;
}

void MimeProviderBase::setOverrideProvider(MimeProviderBase *provider)
{
    m_overrideProvider = provider;
}

bool MimeProviderBase::isMimeTypeGlobsExcluded(const QString &name) const
{
    if (m_overrideProvider) {
        if (m_overrideProvider->hasGlobDeleteAll(name))
            return true;
        return m_overrideProvider->isMimeTypeGlobsExcluded(name);
    }
    return false;
}

MimeBinaryProvider::MimeBinaryProvider(MimeDatabasePrivate *db, const QString &directory)
    : MimeProviderBase(db, directory), m_mimetypeListLoaded(false)
{
    ensureLoaded();
}

MimeBinaryProvider::CacheFile::CacheFile(const QString &fileName)
    : file(fileName), m_valid(false)
{
    load();
}

MimeBinaryProvider::CacheFile::~CacheFile()
{
}

bool MimeBinaryProvider::CacheFile::load()
{
    if (!file.open(QIODevice::ReadOnly))
        return false;
    data = file.map(0, file.size());
    if (data) {
        const int major = getUint16(0);
        const int minor = getUint16(2);
        m_valid = (major == 1 && minor >= 1 && minor <= 2);
    }
    m_mtime = QFileInfo(file).lastModified();
    return m_valid;
}

bool MimeBinaryProvider::CacheFile::reload()
{
    m_valid = false;
    if (file.isOpen()) {
        file.close();
    }
    data = nullptr;
    return load();
}

MimeBinaryProvider::~MimeBinaryProvider() = default;

bool MimeBinaryProvider::isValid()
{
    return m_cacheFile != nullptr;
}

bool MimeBinaryProvider::isInternalDatabase() const
{
    return false;
}

// Position of the "list offsets" values, at the beginning of the mime.cache file
enum {
    PosAliasListOffset = 4,
    PosParentListOffset = 8,
    PosLiteralListOffset = 12,
    PosReverseSuffixTreeOffset = 16,
    PosGlobListOffset = 20,
    PosMagicListOffset = 24,
    // PosNamespaceListOffset = 28,
    PosIconsListOffset = 32,
    PosGenericIconsListOffset = 36
};

bool MimeBinaryProvider::checkCacheChanged()
{
    QFileInfo fileInfo(m_cacheFile->file);
    if (fileInfo.lastModified() > m_cacheFile->m_mtime) {
        // Deletion can't happen by just running update-mime-database.
        // But the user could use rm -rf :-)
        m_cacheFile->reload(); // will mark itself as invalid on failure
        return true;
    }
    return false;
}

void MimeBinaryProvider::ensureLoaded()
{
    if (!m_cacheFile) {
        const QString cacheFileName = m_directory + "/mime.cache"_L1;
        m_cacheFile = std::make_unique<CacheFile>(cacheFileName);
        m_mimetypeListLoaded = false;
        m_mimetypeExtra.clear();
    } else {
        if (checkCacheChanged()) {
            m_mimetypeListLoaded = false;
            m_mimetypeExtra.clear();
        } else {
            return; // nothing to do
        }
    }
    if (!m_cacheFile->isValid()) // verify existence and version
        m_cacheFile.reset();
}

bool MimeBinaryProvider::knowsMimeType(const QString &name)
{
    if (!m_mimetypeListLoaded)
        loadMimeTypeList();
    return m_mimetypeNames.contains(name);
}

void MimeBinaryProvider::addFileNameMatches(const QString &fileName, MimeGlobMatchResult &result)
{
    // TODO checkedMimeTypes
    if (fileName.isEmpty())
        return;
    Q_ASSERT(m_cacheFile);
    int numMatches = 0;
    // Check literals (e.g. "Makefile")
    numMatches = matchGlobList(result, m_cacheFile.get(),
                               m_cacheFile->getUint32(PosLiteralListOffset), fileName);
    // Check the very common *.txt cases with the suffix tree
    if (numMatches == 0) {
        const QString lowerFileName = fileName.toLower();
        const int reverseSuffixTreeOffset = m_cacheFile->getUint32(PosReverseSuffixTreeOffset);
        const int numRoots = m_cacheFile->getUint32(reverseSuffixTreeOffset);
        const int firstRootOffset = m_cacheFile->getUint32(reverseSuffixTreeOffset + 4);
        if (matchSuffixTree(result, m_cacheFile.get(), numRoots, firstRootOffset, lowerFileName,
                            lowerFileName.size() - 1, false)) {
            ++numMatches;
        } else if (matchSuffixTree(result, m_cacheFile.get(), numRoots, firstRootOffset, fileName,
                                   fileName.size() - 1, true)) {
            ++numMatches;
        }
    }
    // Check complex globs (e.g. "callgrind.out[0-9]*" or "README*")
    if (numMatches == 0)
        matchGlobList(result, m_cacheFile.get(), m_cacheFile->getUint32(PosGlobListOffset),
                      fileName);
}

int MimeBinaryProvider::matchGlobList(MimeGlobMatchResult &result, CacheFile *cacheFile, int off,
                                      const QString &fileName)
{
    int numMatches = 0;
    const int numGlobs = cacheFile->getUint32(off);
    //qDebug() << "Loading" << numGlobs << "globs from" << cacheFile->file.fileName() << "at offset" << cacheFile->globListOffset;
    for (int i = 0; i < numGlobs; ++i) {
        const int globOffset = cacheFile->getUint32(off + 4 + 12 * i);
        const int mimeTypeOffset = cacheFile->getUint32(off + 4 + 12 * i + 4);
        const int flagsAndWeight = cacheFile->getUint32(off + 4 + 12 * i + 8);
        const int weight = flagsAndWeight & 0xff;
        const bool caseSensitive = flagsAndWeight & 0x100;
        const Qt::CaseSensitivity qtCaseSensitive = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
        const QString pattern = QLatin1StringView(cacheFile->getCharStar(globOffset));

        const QLatin1StringView mimeType(cacheFile->getCharStar(mimeTypeOffset));
        if (m_overriddenMimeTypes.contains(QLatin1String(mimeType)))
            continue;
        //qDebug() << pattern << mimeType << weight << caseSensitive;
        if (isMimeTypeGlobsExcluded(mimeType))
            continue;

        MimeGlobPattern glob(pattern, QString() /*unused*/, weight, qtCaseSensitive);
        if (glob.matchFileName(fileName)) {
            result.addMatch(mimeType, weight, pattern);
            ++numMatches;
        }
    }
    return numMatches;
}

bool MimeBinaryProvider::matchSuffixTree(MimeGlobMatchResult &result,
                                         MimeBinaryProvider::CacheFile *cacheFile,
                                         int numEntries,
                                         int firstOffset,
                                         const QString &fileName,
                                         qsizetype charPos,
                                         bool caseSensitiveCheck)
{
    QChar fileChar = fileName[charPos];
    int min = 0;
    int max = numEntries - 1;
    while (min <= max) {
        const int mid = (min + max) / 2;
        const int off = firstOffset + 12 * mid;
        const QChar ch = char16_t(cacheFile->getUint32(off));
        if (ch < fileChar)
            min = mid + 1;
        else if (ch > fileChar)
            max = mid - 1;
        else {
            --charPos;
            int numChildren = cacheFile->getUint32(off + 4);
            int childrenOffset = cacheFile->getUint32(off + 8);
            bool success = false;
            if (charPos > 0)
                success = matchSuffixTree(result,
                                          cacheFile,
                                          numChildren,
                                          childrenOffset,
                                          fileName,
                                          charPos,
                                          caseSensitiveCheck);
            if (!success) {
                for (int i = 0; i < numChildren; ++i) {
                    const int childOff = childrenOffset + 12 * i;
                    const int mch = cacheFile->getUint32(childOff);
                    if (mch != 0)
                        break;
                    const int mimeTypeOffset = cacheFile->getUint32(childOff + 4);
                    const QLatin1StringView mimeType(cacheFile->getCharStar(mimeTypeOffset));
                    if (isMimeTypeGlobsExcluded(mimeType))
                        continue;
                    if (m_overriddenMimeTypes.contains(QLatin1String(mimeType)))
                        continue;
                    const int flagsAndWeight = cacheFile->getUint32(childOff + 8);
                    const int weight = flagsAndWeight & 0xff;
                    const bool caseSensitive = flagsAndWeight & 0x100;
                    if (caseSensitiveCheck || !caseSensitive) {
                        result.addMatch(mimeType, weight,
                                        u'*' + QStringView{ fileName }.mid(charPos + 1),
                                        fileName.size() - charPos - 2);
                        success = true;
                    }
                }
            }
            return success;
        }
    }
    return false;
}

bool MimeBinaryProvider::matchMagicRule(MimeBinaryProvider::CacheFile *cacheFile, int numMatchlets, int firstOffset, const QByteArray &data)
{
    const char *dataPtr = data.constData();
    const qsizetype dataSize = data.size();
    for (int matchlet = 0; matchlet < numMatchlets; ++matchlet) {
        const int off = firstOffset + matchlet * 32;
        const int rangeStart = cacheFile->getUint32(off);
        const int rangeLength = cacheFile->getUint32(off + 4);
        //const int wordSize = cacheFile->getUint32(off + 8);
        const int valueLength = cacheFile->getUint32(off + 12);
        const int valueOffset = cacheFile->getUint32(off + 16);
        const int maskOffset = cacheFile->getUint32(off + 20);
        const char *mask = maskOffset ? cacheFile->getCharStar(maskOffset) : nullptr;

        if (!MimeMagicRule::matchSubstring(dataPtr, dataSize, rangeStart, rangeLength, valueLength, cacheFile->getCharStar(valueOffset), mask))
            continue;

        const int numChildren = cacheFile->getUint32(off + 24);
        const int firstChildOffset = cacheFile->getUint32(off + 28);
        if (numChildren == 0) // No submatch? Then we are done.
            return true;
        // Check that one of the submatches matches too
        if (matchMagicRule(cacheFile, numChildren, firstChildOffset, data))
            return true;
    }
    return false;
}

void MimeBinaryProvider::findByMagic(const QByteArray &data, MimeMagicResult &result)
{
    const int magicListOffset = m_cacheFile->getUint32(PosMagicListOffset);
    const int numMatches = m_cacheFile->getUint32(magicListOffset);
    //const int maxExtent = cacheFile->getUint32(magicListOffset + 4);
    const int firstMatchOffset = m_cacheFile->getUint32(magicListOffset + 8);

    for (int i = 0; i < numMatches; ++i) {
        const int off = firstMatchOffset + i * 16;
        const int numMatchlets = m_cacheFile->getUint32(off + 8);
        const int firstMatchletOffset = m_cacheFile->getUint32(off + 12);
        if (matchMagicRule(m_cacheFile.get(), numMatchlets, firstMatchletOffset, data)) {
            const int mimeTypeOffset = m_cacheFile->getUint32(off + 4);
            const char *mimeType = m_cacheFile->getCharStar(mimeTypeOffset);
            if (m_overriddenMimeTypes.contains(QLatin1String(mimeType)))
                continue;
            const int accuracy = static_cast<int>(m_cacheFile->getUint32(off));
            if (accuracy > result.accuracy) {
                result.accuracy = accuracy;
                result.candidate = QString::fromLatin1(mimeType);
                // Return the first match, mime.cache is sorted
                return;
            }
        }
    }
}

void MimeBinaryProvider::addParents(const QString &mime, QStringList &result)
{
    const QByteArray mimeStr = mime.toLatin1();
    const int parentListOffset = m_cacheFile->getUint32(PosParentListOffset);
    const int numEntries = m_cacheFile->getUint32(parentListOffset);

    int begin = 0;
    int end = numEntries - 1;
    while (begin <= end) {
        const int medium = (begin + end) / 2;
        const int off = parentListOffset + 4 + 8 * medium;
        const int mimeOffset = m_cacheFile->getUint32(off);
        const char *aMime = m_cacheFile->getCharStar(mimeOffset);
        const int cmp = qstrcmp(aMime, mimeStr);
        if (cmp < 0) {
            begin = medium + 1;
        } else if (cmp > 0) {
            end = medium - 1;
        } else {
            const int parentsOffset = m_cacheFile->getUint32(off + 4);
            const int numParents = m_cacheFile->getUint32(parentsOffset);
            for (int i = 0; i < numParents; ++i) {
                const int parentOffset = m_cacheFile->getUint32(parentsOffset + 4 + 4 * i);
                const char *aParent = m_cacheFile->getCharStar(parentOffset);
                const QString strParent = QString::fromLatin1(aParent);
                appendIfNew(result, strParent);
            }
            break;
        }
    }
}

QString MimeBinaryProvider::resolveAlias(const QString &name)
{
    const QByteArray input = name.toLatin1();
    const int aliasListOffset = m_cacheFile->getUint32(PosAliasListOffset);
    const int numEntries = m_cacheFile->getUint32(aliasListOffset);
    int begin = 0;
    int end = numEntries - 1;
    while (begin <= end) {
        const int medium = (begin + end) / 2;
        const int off = aliasListOffset + 4 + 8 * medium;
        const int aliasOffset = m_cacheFile->getUint32(off);
        const char *alias = m_cacheFile->getCharStar(aliasOffset);
        const int cmp = qstrcmp(alias, input);
        if (cmp < 0) {
            begin = medium + 1;
        } else if (cmp > 0) {
            end = medium - 1;
        } else {
            const int mimeOffset = m_cacheFile->getUint32(off + 4);
            const char *mimeType = m_cacheFile->getCharStar(mimeOffset);
            return QLatin1StringView(mimeType);
        }
    }
    return QString();
}

void MimeBinaryProvider::addAliases(const QString &name, QStringList &result)
{
    const QByteArray input = name.toLatin1();
    const int aliasListOffset = m_cacheFile->getUint32(PosAliasListOffset);
    const int numEntries = m_cacheFile->getUint32(aliasListOffset);
    for (int pos = 0; pos < numEntries; ++pos) {
        const int off = aliasListOffset + 4 + 8 * pos;
        const int mimeOffset = m_cacheFile->getUint32(off + 4);
        const char *mimeType = m_cacheFile->getCharStar(mimeOffset);

        if (input == mimeType) {
            const int aliasOffset = m_cacheFile->getUint32(off);
            const char *alias = m_cacheFile->getCharStar(aliasOffset);
            const QString strAlias = QString::fromLatin1(alias);
            appendIfNew(result, strAlias);
        }
    }
}

void MimeBinaryProvider::loadMimeTypeList()
{
    if (!m_mimetypeListLoaded) {
        m_mimetypeListLoaded = true;
        m_mimetypeNames.clear();
        // Unfortunately mime.cache doesn't have a full list of all mimetypes.
        // So we have to parse the plain-text files called "types".
        QFile file(m_directory + QStringView(u"/types"));
        if (file.open(QIODevice::ReadOnly)) {
            while (!file.atEnd()) {
                const QByteArray line = file.readLine();
                auto lineView = QByteArrayView(line);
                if (lineView.endsWith('\n'))
                    lineView.chop(1);
                m_mimetypeNames.insert(QString::fromLatin1(lineView));
            }
        }
    }
}

void MimeBinaryProvider::addAllMimeTypes(QList<MimeType> &result)
{
    loadMimeTypeList();
    if (result.isEmpty()) {
        result.reserve(m_mimetypeNames.size());
        for (const QString &name : std::as_const(m_mimetypeNames))
            result.append(MimeType(MimeTypePrivate(name)));
    } else {
        for (const QString &name : std::as_const(m_mimetypeNames))
            if (std::find_if(result.constBegin(), result.constEnd(), [name](const MimeType &mime) -> bool { return mime.name() == name; })
                    == result.constEnd())
                result.append(MimeType(MimeTypePrivate(name)));
    }
}

MimeTypePrivate::LocaleHash MimeBinaryProvider::localeComments(const QString &name)
{
    MimeTypeExtraMap::const_iterator it = loadMimeTypeExtra(name);
    if (it != m_mimetypeExtra.cend())
        return it->second.localeComments;
    return {};
}

bool MimeBinaryProvider::hasGlobDeleteAll(const QString &name)
{
    MimeTypeExtraMap::const_iterator it = loadMimeTypeExtra(name);
    if (it != m_mimetypeExtra.cend())
        return it->second.hasGlobDeleteAll;
    return {};
}

QStringList MimeBinaryProvider::globPatterns(const QString &name)
{
    MimeTypeExtraMap::const_iterator it = loadMimeTypeExtra(name);
    if (it != m_mimetypeExtra.cend())
        return it->second.globPatterns;
    return {};
}

MimeBinaryProvider::MimeTypeExtraMap::const_iterator
MimeBinaryProvider::loadMimeTypeExtra(const QString &mimeName)
{
#if QT_CONFIG(xmlstreamreader)
    auto it = m_mimetypeExtra.find(mimeName);
    if (it == m_mimetypeExtra.cend()) {
        // load comment and globPatterns

        // shared-mime-info since 1.3 lowercases the xml files
        QString mimeFile = m_directory + u'/' + mimeName.toLower() + ".xml"_L1;
        if (!QFileInfo::exists(mimeFile))
            mimeFile = m_directory + u'/' + mimeName + ".xml"_L1; // pre-1.3

        QFile qfile(mimeFile);
        if (!qfile.open(QFile::ReadOnly))
            return m_mimetypeExtra.cend();

        it = m_mimetypeExtra.try_emplace(mimeName).first;
        MimeTypeExtra &extra = it->second;
        QString mainPattern;

        QXmlStreamReader xml(&qfile);
        if (xml.readNextStartElement()) {
            if (xml.name() != "mime-type"_L1) {
                return m_mimetypeExtra.cend();
            }
            const auto name = xml.attributes().value("type"_L1);
            if (name.isEmpty())
                return m_mimetypeExtra.cend();
            if (name.compare(mimeName, Qt::CaseInsensitive))
                qWarning() << "Got name" << name << "in file" << mimeFile << "expected" << mimeName;

            while (xml.readNextStartElement()) {
                const auto tag = xml.name();
                if (tag == "comment"_L1) {
                    QString lang = xml.attributes().value("xml:lang"_L1).toString();
                    const QString text = xml.readElementText();
                    if (lang.isEmpty()) {
                        lang = "default"_L1; // no locale attribute provided, treat it as default.
                    }
                    extra.localeComments.insert(lang, text);
                    continue; // we called readElementText, so we're at the EndElement already.
                } else if (tag == "glob-deleteall"_L1) { // as written out by shared-mime-info >= 0.70
                    extra.hasGlobDeleteAll = true;
                } else if (tag == "glob"_L1) { // as written out by shared-mime-info >= 0.70
                    const QString pattern = xml.attributes().value("pattern"_L1).toString();
                    if (mainPattern.isEmpty() && pattern.startsWith(u'*')) {
                        mainPattern = pattern;
                    }
                    appendIfNew(extra.globPatterns, pattern);
                }
                xml.skipCurrentElement();
            }
            Q_ASSERT(xml.name() == "mime-type"_L1);
        }

        // Let's assume that shared-mime-info is at least version 0.70
        // Otherwise we would need 1) a version check, and 2) code for parsing patterns from the globs file.
        if (!mainPattern.isEmpty() &&
                (extra.globPatterns.isEmpty() || extra.globPatterns.constFirst() != mainPattern)) {
            // ensure it's first in the list of patterns
            extra.globPatterns.removeAll(mainPattern);
            extra.globPatterns.prepend(mainPattern);
        }
    }
    return it;
#else
    Q_UNUSED(mimeName);
    qWarning("Cannot load mime type since QXmlStreamReader is not available.");
    return m_mimetypeExtra.cend();
#endif // feature xmlstreamreader
}

// Binary search in the icons or generic-icons list
QLatin1StringView MimeBinaryProvider::iconForMime(CacheFile *cacheFile, int posListOffset,
                                                  const QByteArray &inputMime)
{
    const int iconsListOffset = cacheFile->getUint32(posListOffset);
    const int numIcons = cacheFile->getUint32(iconsListOffset);
    int begin = 0;
    int end = numIcons - 1;
    while (begin <= end) {
        const int medium = (begin + end) / 2;
        const int off = iconsListOffset + 4 + 8 * medium;
        const int mimeOffset = cacheFile->getUint32(off);
        const char *mime = cacheFile->getCharStar(mimeOffset);
        const int cmp = qstrcmp(mime, inputMime);
        if (cmp < 0)
            begin = medium + 1;
        else if (cmp > 0)
            end = medium - 1;
        else {
            const int iconOffset = cacheFile->getUint32(off + 4);
            return QLatin1StringView(cacheFile->getCharStar(iconOffset));
        }
    }
    return QLatin1StringView();
}

QString MimeBinaryProvider::icon(const QString &name)
{
    const QByteArray inputMime = name.toLatin1();
    return iconForMime(m_cacheFile.get(), PosIconsListOffset, inputMime);
}

QString MimeBinaryProvider::genericIcon(const QString &name)
{
    const QByteArray inputMime = name.toLatin1();
    return iconForMime(m_cacheFile.get(), PosGenericIconsListOffset, inputMime);
}

////

#if 0
#if QT_CONFIG(mimetype_database)
static QString internalMimeFileName()
{
    return QStringLiteral("<internal MIME data>");
}

QMimeXMLProvider::QMimeXMLProvider(QMimeDatabasePrivate *db, InternalDatabaseEnum)
    : QMimeProviderBase(db, internalMimeFileName())
{
    static_assert(sizeof(mimetype_database), "Bundled MIME database is empty");
    static_assert(sizeof(mimetype_database) <= MimeTypeDatabaseOriginalSize,
                      "Compressed MIME database is larger than the original size");
    static_assert(MimeTypeDatabaseOriginalSize <= 16*1024*1024,
                      "Bundled MIME database is too big");
    const char *data = reinterpret_cast<const char *>(mimetype_database);
    qsizetype size = MimeTypeDatabaseOriginalSize;

#ifdef MIME_DATABASE_IS_ZSTD
    // uncompress with libzstd
    std::unique_ptr<char []> uncompressed(new char[size]);
    size = ZSTD_decompress(uncompressed.get(), size, mimetype_database, sizeof(mimetype_database));
    Q_ASSERT(!ZSTD_isError(size));
    data = uncompressed.get();
#elif defined(MIME_DATABASE_IS_GZIP)
    std::unique_ptr<char []> uncompressed(new char[size]);
    z_stream zs = {};
    zs.next_in = const_cast<Bytef *>(mimetype_database);
    zs.avail_in = sizeof(mimetype_database);
    zs.next_out = reinterpret_cast<Bytef *>(uncompressed.get());
    zs.avail_out = size;

    int res = inflateInit2(&zs, MAX_WBITS | 32);
    Q_ASSERT(res == Z_OK);
    res = inflate(&zs, Z_FINISH);
    Q_ASSERT(res == Z_STREAM_END);
    res = inflateEnd(&zs);
    Q_ASSERT(res == Z_OK);

    data = uncompressed.get();
    size = zs.total_out;
#endif

    load(data, size);
}
#else // !QT_CONFIG(mimetype_database)
// never called in release mode, but some debug builds may need
// this to be defined.
MimeXMLProvider::MimeXMLProvider(MimeDatabasePrivate *db, InternalDatabaseEnum)
    : MimeProviderBase(db, QString())
{
    Q_UNREACHABLE();
}
#endif // QT_CONFIG(mimetype_database)
#endif

static const char internalMimeFileName[] = ":/utils/mimetypes/tika-mimetypes.xml";

// for Qt Creator: internal database from resources
MimeXMLProvider::MimeXMLProvider(MimeDatabasePrivate *db, InternalDatabaseEnum)
    : MimeProviderBase(db, internalMimeFileName)
{
    load(internalMimeFileName);
}

MimeXMLProvider::MimeXMLProvider(MimeDatabasePrivate *db, const QString &directory)
    : MimeProviderBase(db, directory)
{
    ensureLoaded();
}

MimeXMLProvider::~MimeXMLProvider()
{
}

bool MimeXMLProvider::isValid()
{
    // If you change this method, adjust the logic in MimeDatabasePrivate::loadProviders,
    // which assumes isValid==false is only possible in MimeBinaryProvider.
    return true;
}

bool MimeXMLProvider::isInternalDatabase() const
{
#if 1 //QT_CONFIG(mimetype_database)
    return m_directory == internalMimeFileName;
#else
    return false;
#endif
}

bool MimeXMLProvider::knowsMimeType(const QString &name)
{
    return m_nameMimeTypeMap.contains(name);
}

void MimeXMLProvider::addFileNameMatches(const QString &fileName, MimeGlobMatchResult &result)
{
    auto filterFunc = [this](const QString &name) {
        return !isMimeTypeGlobsExcluded(name) && !m_overriddenMimeTypes.contains(name);
    };
    m_mimeTypeGlobs.matchingGlobs(fileName, result, filterFunc);
}

void MimeXMLProvider::findByMagic(const QByteArray &data, MimeMagicResult &result)
{
    for (const MimeMagicRuleMatcher &matcher : std::as_const(m_magicMatchers)) {
        if (m_overriddenMimeTypes.contains(matcher.mimetype()))
            continue;
        if (matcher.matches(data)) {
            const int priority = matcher.priority();
            if (priority > result.accuracy) {
                result.accuracy = priority;
                result.candidate = matcher.mimetype();
            }
        }
    }
}

void MimeXMLProvider::ensureLoaded()
{
    QStringList allFiles;
    const QString packageDir = m_directory + QStringView(u"/packages");
    QDir dir(packageDir);
    const QStringList files = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    allFiles.reserve(files.size());
    for (const QString &xmlFile : files)
        allFiles.append(packageDir + u'/' + xmlFile);

    if (m_allFiles == allFiles)
        return;
    m_allFiles = allFiles;

    m_nameMimeTypeMap.clear();
    m_aliases.clear();
    m_parents.clear();
    m_mimeTypeGlobs.clear();
    m_magicMatchers.clear();

    //qDebug() << "Loading" << m_allFiles;

    for (const QString &file : std::as_const(allFiles))
        load(file);
}

MimeTypePrivate::LocaleHash MimeXMLProvider::localeComments(const QString &name)
{
    return m_nameMimeTypeMap.value(name).localeComments;
}

bool MimeXMLProvider::hasGlobDeleteAll(const QString &name)
{
    return m_nameMimeTypeMap.value(name).hasGlobDeleteAll;
}

QStringList MimeXMLProvider::globPatterns(const QString &name)
{
    return m_nameMimeTypeMap.value(name).globPatterns;
}

QString MimeXMLProvider::icon(const QString &name)
{
    return m_nameMimeTypeMap.value(name).iconName;
}

QString MimeXMLProvider::genericIcon(const QString &name)
{
    return m_nameMimeTypeMap.value(name).genericIconName;
}

void MimeXMLProvider::load(const QString &fileName)
{
    QString errorMessage;
    if (!load(fileName, &errorMessage))
        qWarning("MimeDatabase: Error loading %ls\n%ls", qUtf16Printable(fileName), qUtf16Printable(errorMessage));
}

bool MimeXMLProvider::load(const QString &fileName, QString *errorMessage)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage)
            *errorMessage = "Cannot open "_L1 + fileName + ": "_L1 + file.errorString();
        return false;
    }

    if (errorMessage)
        errorMessage->clear();

    MimeTypeParser parser(*this);
    return parser.parse(&file, fileName, errorMessage);
}

#if 0 //QT_CONFIG(mimetype_database)
void MimeXMLProvider::load(const char *data, qsizetype len)
{
    QBuffer buffer;
    buffer.setData(QByteArray::fromRawData(data, len));
    buffer.open(QIODevice::ReadOnly);
    QString errorMessage;
    MimeTypeParser parser(*this);
    if (!parser.parse(&buffer, internalMimeFileName(), &errorMessage))
        qWarning("MimeDatabase: Error loading internal MIME data\n%s", qPrintable(errorMessage));
}
#endif

void MimeXMLProvider::addGlobPattern(const MimeGlobPattern &glob)
{
    m_mimeTypeGlobs.addGlob(glob);
}

void MimeXMLProvider::addMimeType(const MimeTypeXMLData &mt)
{
    m_nameMimeTypeMap.insert(mt.name, mt);
}

void MimeXMLProvider::addParents(const QString &mime, QStringList &result)
{
    for (const QString &parent : m_parents.value(mime)) {
        if (!result.contains(parent))
            result.append(parent);
    }
}

void MimeXMLProvider::addParent(const QString &child, const QString &parent)
{
    m_parents[child].append(parent);
}

void MimeXMLProvider::addAliases(const QString &name, QStringList &result)
{
    // Iterate through the whole hash. This method is rarely used.
    for (const auto &[alias, mimeName] : std::as_const(m_aliases).asKeyValueRange()) {
        if (mimeName == name)
            appendIfNew(result, alias);
    }
}

QString MimeXMLProvider::resolveAlias(const QString &name)
{
    return m_aliases.value(name);
}

void MimeXMLProvider::addAlias(const QString &alias, const QString &name)
{
    m_aliases.insert(alias, name);
}

void MimeXMLProvider::addAllMimeTypes(QList<MimeType> &result)
{
    if (result.isEmpty()) { // fast path
        for (auto it = m_nameMimeTypeMap.constBegin(), end = m_nameMimeTypeMap.constEnd();
             it != end; ++it) {
            result.append(MimeType(MimeTypePrivate(it.value().name)));
        }
    } else {
        for (auto it = m_nameMimeTypeMap.constBegin(), end = m_nameMimeTypeMap.constEnd() ; it != end ; ++it) {
            const QString newMime = it.key();
            if (std::find_if(result.constBegin(), result.constEnd(), [newMime](const MimeType &mime) -> bool { return mime.name() == newMime; })
                    == result.constEnd())
                result.append(MimeType(MimeTypePrivate(it.value().name)));
        }
    }
}

void MimeXMLProvider::addMagicMatcher(const MimeMagicRuleMatcher &matcher)
{
    m_magicMatchers.append(matcher);
}

// added for Qt Creator
MimeXMLProvider::MimeXMLProvider(MimeDatabasePrivate *db,
                                 const QString &directory,
                                 const QByteArray &data)
    : MimeProviderBase(db, directory)
{
    QString errorMessage;
    MimeTypeParser parser(*this);
    QByteArray bufferData(data);
    QBuffer buffer(&bufferData);
    if (!buffer.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("MimeDatabase: Error creating buffer for data with ID %ls.",
                 qUtf16Printable(directory));
    }
    if (!parser.parse(&buffer, directory, &errorMessage)) {
        qWarning("MimeDatabase: Error loading data for ID %ls\n%ls",
                 qUtf16Printable(directory),
                 qUtf16Printable(errorMessage));
    }
}

bool MimeBinaryProvider::hasMimeTypeForName(const QString &name)
{
    loadMimeTypeList();
    return m_mimetypeNames.contains(name);
}

QStringList MimeBinaryProvider::allMimeTypeNames()
{
    // similar to addAllMimeTypes
    loadMimeTypeList();
    return QList(m_mimetypeNames.cbegin(), m_mimetypeNames.cend());
}

bool MimeXMLProvider::hasMimeTypeForName(const QString &name)
{
    return m_nameMimeTypeMap.contains(name);
}

QStringList MimeXMLProvider::allMimeTypeNames()
{
    // similar to addAllMimeTypes
    return m_nameMimeTypeMap.keys();
}

QMap<int, QList<MimeMagicRule>> MimeBinaryProvider::magicRulesForMimeType(const MimeType &mimeType) const
{
    Q_UNUSED(mimeType)
    qWarning("Mimetypes: magicRulesForMimeType not implemented for binary provider");
    return {};
}

void MimeBinaryProvider::setMagicRulesForMimeType(const MimeType &mimeType,
                                                  const QMap<int, QList<MimeMagicRule>> &rules)
{
    Q_UNUSED(mimeType)
    Q_UNUSED(rules)
    qWarning("Mimetypes: setMagicRulesForMimeType not implemented for binary provider");
}

void MimeBinaryProvider::setGlobPatternsForMimeType(const MimeType &mimeType,
                                                    const QStringList &patterns)
{
    Q_UNUSED(mimeType)
    Q_UNUSED(patterns)
    qWarning("Mimetypes: setGlobPatternsForMimeType not implemented for binary provider");
}

QMap<int, QList<MimeMagicRule>> MimeXMLProvider::magicRulesForMimeType(const MimeType &mimeType) const
{
    QMap<int, QList<MimeMagicRule>> result;
    for (const MimeMagicRuleMatcher &matcher : m_magicMatchers) {
        if (mimeType.name() == matcher.mimetype())
            result[matcher.priority()].append(matcher.magicRules());
    }
    return result;
}

void MimeXMLProvider::setMagicRulesForMimeType(const MimeType &mimeType,
                                               const QMap<int, QList<MimeMagicRule>> &rules)
{
    // remove previous rules
    m_magicMatchers.erase(std::remove_if(m_magicMatchers.begin(),
                                         m_magicMatchers.end(),
                                         [mimeType](const MimeMagicRuleMatcher &matcher) {
                                             return mimeType.name() == matcher.mimetype();
                                         }),
                          m_magicMatchers.end());
    // add new rules
    for (auto it = rules.cbegin(); it != rules.cend(); ++it) {
        MimeMagicRuleMatcher matcher(mimeType.name(), it.key() /*priority*/);
        matcher.addRules(it.value());
        addMagicMatcher(matcher);
    }
}

void MimeXMLProvider::setGlobPatternsForMimeType(const MimeType &mimeType,
                                                 const QStringList &patterns)
{
    // remove all previous globs
    m_mimeTypeGlobs.removeMimeType(mimeType.name());
    // add new patterns as case-insensitive default-weight patterns
    for (const QString &pattern : patterns)
        addGlobPattern(MimeGlobPattern(pattern, mimeType.name()));
    m_nameMimeTypeMap[mimeType.name()].globPatterns = patterns;
}

} // namespace Utils
