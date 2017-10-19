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

#include "qmljsimportdependencies.h"
#include "qmljsinterpreter.h"
#include "qmljsqrcparser.h"
#include "qmljsviewercontext.h"

#include <utils/qtcassert.h>

#include <QCryptographicHash>
#include <QLoggingCategory>

#include <algorithm>

static Q_LOGGING_CATEGORY(importsLog, "qtc.qmljs.imports")

namespace QmlJS {

ImportKind::Enum toImportKind(ImportType::Enum type)
{
    switch (type) {
    case ImportType::Invalid:
        break;
    case ImportType::Library:
        return ImportKind::Library;
    case ImportType::ImplicitDirectory:
    case ImportType::File:
    case ImportType::Directory:
    case ImportType::UnknownFile:
        return ImportKind::Path;
    case ImportType::QrcFile:
    case ImportType::QrcDirectory:
        return ImportKind::QrcPath;
    }
    return ImportKind::Invalid;
}

ImportMatchStrength::ImportMatchStrength(const QList<int> &match)
    : m_match(match)
{ }

int ImportMatchStrength::compareMatch(const ImportMatchStrength &o) const
{
    int len1 = m_match.size();
    int len2 = o.m_match.size();
    int len = ((len1 < len2) ? len1 : len2);
    for (int i = 0; i < len; ++ i) {
        int v1 = m_match.at(i);
        int v2 = o.m_match.at(i);
        if (v1 < v2)
            return -1;
        if (v2 > v1)
            return 1;
    }
    if (len1 < len2)
        return -1;
    if (len1 > len2)
        return 1;
    return 0;
}

bool ImportMatchStrength::hasNoMatch()
{
    return m_match.isEmpty();
}

bool ImportMatchStrength::hasMatch()
{
    return !m_match.isEmpty();
}

bool operator ==(const ImportMatchStrength &m1, const ImportMatchStrength &m2)
{
    return m1.m_match == m2.m_match;
}

bool operator !=(const ImportMatchStrength &m1, const ImportMatchStrength &m2)
{
    return !(m1 == m2);
}

bool operator <(const ImportMatchStrength &m1, const ImportMatchStrength &m2)
{
    return m1.compareMatch(m2) < 0;
}

ImportKey::ImportKey()
    : type(ImportType::Invalid),
      majorVersion(LanguageUtils::ComponentVersion::NoVersion),
      minorVersion(LanguageUtils::ComponentVersion::NoVersion)
{ }

ImportKey::ImportKey(const ImportInfo &info)
    : type(info.type())
    , majorVersion(info.version().majorVersion())
    , minorVersion(info.version().minorVersion())
{
    splitPath = QFileInfo(info.path()).canonicalFilePath().split(QLatin1Char('/'),
                                                                 QString::KeepEmptyParts);
}

ImportKey::ImportKey(ImportType::Enum type, const QString &path, int majorVersion, int minorVersion)
    : type(type)
    , majorVersion(majorVersion)
    , minorVersion(minorVersion)
{
    switch (type) {
    case ImportType::Library:
        splitPath = path.split(QLatin1Char('.'));
        break;
    case ImportType::ImplicitDirectory:
    case ImportType::Directory:
        splitPath = path.split(QLatin1Char('/'));
        if (splitPath.length() > 1 && splitPath.last().isEmpty())
            splitPath.removeLast();
        break;
    case ImportType::File:
    case ImportType::QrcFile:
        splitPath = QrcParser::normalizedQrcFilePath(path).split(QLatin1Char('/'));
        break;
    case ImportType::QrcDirectory:
        splitPath = QrcParser::normalizedQrcDirectoryPath(path).split(QLatin1Char('/'));
        if (splitPath.length() > 1 && splitPath.last().isEmpty())
            splitPath.removeLast();
        break;
    case ImportType::Invalid:
    case ImportType::UnknownFile:
        splitPath = path.split(QLatin1Char('/'));
        break;
    }
}

void ImportKey::addToHash(QCryptographicHash &hash) const
{
    hash.addData(reinterpret_cast<const char *>(&type), sizeof(type));
    hash.addData(reinterpret_cast<const char *>(&majorVersion), sizeof(majorVersion));
    hash.addData(reinterpret_cast<const char *>(&minorVersion), sizeof(minorVersion));
    foreach (const QString &s, splitPath) {
        hash.addData("/", 1);
        hash.addData(reinterpret_cast<const char *>(s.constData()), sizeof(QChar) * s.size());
    }
    hash.addData("/", 1);
}

ImportKey ImportKey::flatKey() const {
    switch (type) {
    case ImportType::Invalid:
        return *this;
    case ImportType::ImplicitDirectory:
    case ImportType::Library:
    case ImportType::File:
    case ImportType::Directory:
    case ImportType::QrcFile:
    case ImportType::QrcDirectory:
    case ImportType::UnknownFile:
        break;
    }
    QStringList flatPath = splitPath;
    int i = 0;
    while (i < flatPath.size()) {
        if (flatPath.at(i).startsWith(QLatin1Char('+')))
            flatPath.removeAt(i);
        else
            ++i;
    }
    if (flatPath.size() == splitPath.size())
        return *this;
    ImportKey res = *this;
    res.splitPath = flatPath;
    return res;
}

QString ImportKey::libraryQualifiedPath() const
{
    QString res = splitPath.join(QLatin1Char('.'));
    if (res.isEmpty() && !splitPath.isEmpty())
        return QLatin1String("");
    return res;
}

QString ImportKey::path() const
{
    QString res = splitPath.join(QLatin1Char('/'));
    if (res.isEmpty() && !splitPath.isEmpty())
        return QLatin1String("/");
    return res;
}

ImportMatchStrength ImportKey::matchImport(const ImportKey &o, const ViewerContext &vContext) const
{
    if (majorVersion != o.majorVersion || minorVersion > o.minorVersion)
        return ImportMatchStrength();
    bool dirToFile = false;
    switch (o.type) {
    case ImportType::Invalid:
        return ImportMatchStrength();
    case ImportType::ImplicitDirectory:
    case ImportType::Directory:
        switch (type) {
        case ImportType::File:
        case ImportType::UnknownFile:
            dirToFile = true;
            break;
        case ImportType::ImplicitDirectory:
        case ImportType::Directory:
            break;
        default:
            return ImportMatchStrength();
        }
        break;
    case ImportType::Library:
        if (type != ImportType::Library)
            return ImportMatchStrength();
        break;
    case ImportType::QrcDirectory:
        switch (type) {
        case ImportType::QrcFile:
            dirToFile = true;
            break;
        case ImportType::QrcDirectory:
            break;
        default:
            return ImportMatchStrength();
        }
        break;
    case ImportType::QrcFile:
        if (type != ImportType::QrcFile)
            return ImportMatchStrength();
        break;
    case ImportType::UnknownFile:
    case ImportType::File:
        switch (type) {
        case ImportType::UnknownFile:
        case ImportType::File:
            break;
        default:
            return ImportMatchStrength();
        }
        break;
    }

    QList<int> res;
    int iPath1 = 0;
    int lenPath1 = splitPath.size();
    int iPath2 = 0;
    int lenPath2 = o.splitPath.size();
    if (dirToFile)
        --lenPath1;
    int iSelector = 0;
    int nSelectors = vContext.selectors.size();
    while (iPath1 < lenPath1) {
        if (lenPath2 - iPath2 > lenPath1 - iPath1)
            return ImportMatchStrength();
        QString p1 = splitPath.at(iPath1);
        if (iPath2 < lenPath2) {
            QString p2 = splitPath.at(iPath2);
            if (p1 == p2) {
                ++iPath1;
                ++iPath2;
                continue;
            }
        }
        if (!p1.startsWith(QLatin1Char('+')))
            return QList<int>();
        QStringRef selectorAtt(&p1, 1, p1.size()-1);
        while (iSelector < nSelectors) {
            if (selectorAtt == vContext.selectors.at(iSelector))
                break;
            ++iSelector;
        }
        if (iSelector == nSelectors)
            return QList<int>();
        res << (nSelectors - iSelector);
        ++iSelector;
        ++iPath1;
    }
    if (iPath2 != lenPath2)
        return QList<int>();
    if (res.isEmpty())
        res << 0;
    return  ImportMatchStrength(res);
}

int ImportKey::compare(const ImportKey &other) const
{
    ImportKind::Enum k1 = toImportKind(type);
    ImportKind::Enum k2 = toImportKind(other.type);
    if (k1 < k2)
        return -1;
    if (k1 > k2)
        return 1;
    int len1 = splitPath.size();
    int len2 = other.splitPath.size();
    int len = ((len1 < len2) ? len1 : len2);
    for (int i = 0; i < len; ++ i) {
        QString v1 = splitPath.at(i);
        QString v2 = other.splitPath.at(i);
        if (v1 < v2)
            return -1;
        if (v1 > v2)
            return 1;
    }
    if (len1 < len2)
        return -1;
    if (len1 > len2)
        return 1;
    if (majorVersion < other.majorVersion)
        return -1;
    if (majorVersion > other.majorVersion)
        return 1;
    if (minorVersion < other.minorVersion)
        return -1;
    if (minorVersion > other.minorVersion)
        return 1;
    if (type < other.type)
        return -1;
    if (type > other.type)
        return 1;
    return 0;
}

bool ImportKey::isDirectoryLike() const
{
    switch (type) {
    case ImportType::Directory:
    case ImportType::ImplicitDirectory:
    case ImportType::QrcDirectory:
        return true;
    default:
        return false;
    }
}

ImportKey::DirCompareInfo ImportKey::compareDir(const ImportKey &superDir) const
{
    // assumes dir/+selectors/file (i.e. no directories inside selectors)
    switch (superDir.type) {
    case ImportType::UnknownFile:
    case ImportType::File:
    case ImportType::Directory:
    case ImportType::ImplicitDirectory:
        if (type != ImportType::File && type != ImportType::ImplicitDirectory
                && type != ImportType::Directory && type != ImportType::UnknownFile)
            return Incompatible;
        break;
    case ImportType::QrcDirectory:
    case ImportType::QrcFile:
        if (type != ImportType::QrcDirectory && type != ImportType::QrcFile)
            return Incompatible;
        break;
    case ImportType::Invalid:
    case ImportType::Library:
        return Incompatible;
    }
    bool isDir1 = isDirectoryLike();
    bool isDir2 = superDir.isDirectoryLike();
    int len1 = splitPath.size();
    int len2 = superDir.splitPath.size();
    if (isDir1 && len1 > 0)
        --len1;
    if (isDir2 && len2 > 0)
        --len2;

    int i1 = 0;
    int i2 = 0;
    while (i1 < len1 && i2 < len2) {
        QString p1 = splitPath.at(i1);
        QString p2 = superDir.splitPath.at(i2);
        if (p1 == p2) {
            ++i1;
            ++i2;
            continue;
        }
        if (p1.startsWith(QLatin1Char('+'))) {
            if (p2.startsWith(QLatin1Char('+')))
                return SameDir;
            return SecondInFirst;
        }
        if (p2.startsWith(QLatin1Char('+')))
            return FirstInSecond;
        return Different;
    }
    if (i1 < len1) {
        if (splitPath.at(i1).startsWith(QLatin1Char('+')))
            return SameDir;
        return SecondInFirst;
    }
    if (i2 < len2) {
        if (superDir.splitPath.at(i2).startsWith(QLatin1Char('+')))
            return SameDir;
        return SecondInFirst;
    }
    return SameDir;
}

QString ImportKey::toString() const
{
    QString res;
    switch (type) {
    case ImportType::UnknownFile:
    case ImportType::File:
        res = path();
        break;
    case ImportType::Directory:
    case ImportType::ImplicitDirectory:
        res = path() + QLatin1Char('/');
        break;
    case ImportType::QrcDirectory:
        res = QLatin1String("qrc:") + path() + QLatin1Char('/');
        break;
    case ImportType::QrcFile:
        res = QLatin1String("qrc:") + path() + QLatin1Char('/');
        break;
    case ImportType::Invalid:
        res = path();
        break;
    case ImportType::Library:
        res = splitPath.join(QLatin1Char('.'));
        break;
    }

    if (majorVersion != LanguageUtils::ComponentVersion::NoVersion
            || minorVersion != LanguageUtils::ComponentVersion::NoVersion)
        return res + QLatin1Char(' ') + QString::number(majorVersion)
                + QLatin1Char('.') + QString::number(minorVersion);

    return res;
}

uint qHash(const ImportKey &info)
{
    uint res = ::qHash(info.type) ^
            ::qHash(info.majorVersion) ^ ::qHash(info.minorVersion);
    foreach (const QString &s, info.splitPath)
        res = res ^ ::qHash(s);
    return res;
}

bool operator==(const ImportKey &i1, const ImportKey &i2)
{
    return i1.type == i2.type
            && i1.splitPath == i2.splitPath
            && i1.majorVersion == i2.majorVersion
            && i1.minorVersion == i2.minorVersion;
}

bool operator !=(const ImportKey &i1, const ImportKey &i2)
{
    return ! (i1 == i2);
}

bool operator <(const ImportKey &i1, const ImportKey &i2)
{
    return i1.compare(i2) < 0;
}

QString Export::libraryTypeName() { return QStringLiteral("%Library%"); }

Export::Export()
    : intrinsic(false)
{ }

Export::Export(ImportKey exportName, const QString &pathRequired, bool intrinsic, const QString &typeName)
    : exportName(exportName), pathRequired(pathRequired), typeName(typeName), intrinsic(intrinsic)
{ }

bool Export::visibleInVContext(const ViewerContext &vContext) const
{
    return pathRequired.isEmpty() || vContext.paths.contains(pathRequired);
}

bool operator ==(const Export &i1, const Export &i2)
{
    return i1.exportName == i2.exportName
            && i1.pathRequired == i2.pathRequired
            && i1.intrinsic == i2.intrinsic
            && i1.typeName == i2.typeName;
}

bool operator !=(const Export &i1, const Export &i2)
{
    return !(i1 == i2);
}

CoreImport::CoreImport() : language(Dialect::Qml) { }

CoreImport::CoreImport(const QString &importId, const QList<Export> &possibleExports,
                       Dialect language, const QByteArray &fingerprint)
    : importId(importId), possibleExports(possibleExports), language(language),
      fingerprint(fingerprint)
{ }

bool CoreImport::valid() {
    return !fingerprint.isEmpty();
}

QByteArray DependencyInfo::calculateFingerprint(const ImportDependencies &deps)
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    rootImport.addToHash(hash);
    QStringList coreImports = allCoreImports.toList();
    coreImports.sort();
    foreach (const QString importId, coreImports) {
        hash.addData(reinterpret_cast<const char*>(importId.constData()), importId.size() * sizeof(QChar));
        QByteArray coreImportFingerprint = deps.coreImport(importId).fingerprint;
        hash.addData(coreImportFingerprint);
    }
    hash.addData("/", 1);
    QList<ImportKey> imports(allImports.toList());
    std::sort(imports.begin(), imports.end());
    foreach (const ImportKey &k, imports)
        k.addToHash(hash);
    return hash.result();
}

MatchedImport::MatchedImport()
{ }

MatchedImport::MatchedImport(ImportMatchStrength matchStrength, ImportKey importKey,
                             const QString &coreImportId)
    : matchStrength(matchStrength), importKey(importKey), coreImportId(coreImportId)
{ }

int MatchedImport::compare(const MatchedImport &o) const {
    int res = matchStrength.compareMatch(o.matchStrength);
    if (res != 0)
        return res;
    res = importKey.compare(o.importKey);
    if (res != 0)
        return res;
    if (coreImportId < o.coreImportId)
        return -1;
    if (coreImportId > o.coreImportId)
        return 1;
    return 0;
}

bool operator ==(const MatchedImport &m1, const MatchedImport &m2)
{
    return m1.compare(m2) == 0;
}

bool operator !=(const MatchedImport &m1, const MatchedImport &m2)
{
    return m1.compare(m2) != 0;
}

bool operator <(const MatchedImport &m1, const MatchedImport &m2)
{
    return m1.compare(m2) < 0;
}

ImportDependencies::ImportDependencies()
{ }

ImportDependencies::~ImportDependencies()
{ }

void ImportDependencies::filter(const ViewerContext &vContext)
{
    QMap<QString, CoreImport> newCoreImports;
    QMap<ImportKey, QStringList> newImportCache;
    QMapIterator<QString, CoreImport> j(m_coreImports);
    bool hasChanges = false;
    while (j.hasNext()) {
        j.next();
        const CoreImport &cImport = j.value();
        if (vContext.languageIsCompatible(cImport.language)) {
            QList<Export> newExports;
            foreach (const Export &e, cImport.possibleExports) {
                if (e.visibleInVContext(vContext)) {
                    newExports.append(e);
                    QStringList &candidateImports = newImportCache[e.exportName];
                    if (!candidateImports.contains(cImport.importId))
                        candidateImports.append(cImport.importId);
                }
            }
            if (newExports.size() == cImport.possibleExports.size()) {
                newCoreImports.insert(cImport.importId, cImport);
            } else if (newExports.length() > 0) {
                CoreImport newCImport = cImport;
                newCImport.possibleExports = newExports;
                newCoreImports.insert(newCImport.importId, newCImport);
                hasChanges = true;
            } else {
                hasChanges = true;
            }
        } else {
            hasChanges = true;
        }
    }
    if (!hasChanges)
        return;
    m_coreImports = newCoreImports;
    m_importCache = newImportCache;
}

CoreImport ImportDependencies::coreImport(const QString &importId) const
{
    return m_coreImports.value(importId);
}

void ImportDependencies::iterateOnCandidateImports(
        const ImportKey &key, const ViewerContext &vContext,
        std::function<bool (const ImportMatchStrength &,const Export &,const CoreImport &)>
        const &iterF) const
{
    switch (key.type) {
    case ImportType::Directory:
    case ImportType::QrcDirectory:
    case ImportType::ImplicitDirectory:
        break;
    default:
    {
        const QStringList imp = m_importCache.value(key.flatKey());
        foreach (const QString &cImportName, imp) {
            CoreImport cImport = coreImport(cImportName);
            if (vContext.languageIsCompatible(cImport.language)) {
                foreach (const Export e, cImport.possibleExports) {
                    if (e.visibleInVContext(vContext)) {
                        ImportMatchStrength m = e.exportName.matchImport(key, vContext);
                        if (m.hasMatch()) {
                            if (!iterF(m, e, cImport))
                                return;
                        }
                    }
                }
            }
        }
        return;
    }
    }
    QMap<ImportKey, QStringList>::const_iterator lb = m_importCache.lowerBound(key.flatKey());
    QMap<ImportKey, QStringList>::const_iterator end = m_importCache.constEnd();
    while (lb != end) {
        ImportKey::DirCompareInfo c = key.compareDir(lb.key());
        if (c == ImportKey::SameDir) {
            foreach (const QString &cImportName, lb.value()) {
                CoreImport cImport = coreImport(cImportName);
                if (vContext.languageIsCompatible(cImport.language)) {
                    foreach (const Export e, cImport.possibleExports) {
                        if (e.visibleInVContext(vContext)) {
                            ImportMatchStrength m = e.exportName.matchImport(key, vContext);
                            if (m.hasMatch()) {
                                if (!iterF(m, e, cImport))
                                    return;
                            }
                        }
                    }
                }
            }
        } else if (c != ImportKey::SecondInFirst) {
            break;
        }
        ++lb;
    }
}

class CollectCandidateImports
{
public:
    ImportDependencies::ImportElements &res;

    CollectCandidateImports(ImportDependencies::ImportElements & res)
        : res(res)
    { }

    bool operator ()(const ImportMatchStrength &m, const Export &e, const CoreImport &cI) const
    {
        ImportKey flatName = e.exportName.flatKey();
        res[flatName].append(MatchedImport(m, e.exportName, cI.importId));
        return true;
    }
};

ImportDependencies::ImportElements ImportDependencies::candidateImports(
        const ImportKey &key,
        const ViewerContext &vContext) const
{
    ImportDependencies::ImportElements res;
    CollectCandidateImports collector(res);
    iterateOnCandidateImports(key, vContext, collector);
    typedef QMap<ImportKey, QList<MatchedImport> >::iterator iter_t;
    iter_t i = res.begin();
    iter_t end = res.end();
    while (i != end) {
        std::sort(i.value().begin(), i.value().end());
        ++i;
    }
    return res;
}

QList<DependencyInfo::ConstPtr> ImportDependencies::createDependencyInfos(
        const ImportKey &mainDoc, const ViewerContext &vContext) const
{
    Q_UNUSED(mainDoc);
    Q_UNUSED(vContext);
    QList<DependencyInfo::ConstPtr> res;
    QTC_CHECK(false);
    return res;
}

void ImportDependencies::addCoreImport(const CoreImport &import)
{
    CoreImport newImport = import;
    if (m_coreImports.contains(import.importId)) {
        CoreImport oldVal = m_coreImports.value(import.importId);
        foreach (const Export &e, oldVal.possibleExports) {
            if (e.intrinsic)
                removeImportCacheEntry(e.exportName, import.importId);
            else
                newImport.possibleExports.append(e);
        }
    }
    foreach (const Export &e, import.possibleExports)
        m_importCache[e.exportName].append(import.importId);
    m_coreImports.insert(newImport.importId, newImport);
    if (importsLog().isDebugEnabled()) {
        QString msg = QString::fromLatin1("added import %1 for").arg(newImport.importId);
        foreach (const Export &e, newImport.possibleExports)
            msg += QString::fromLatin1("\n %1(%2)").arg(e.exportName.toString(), e.pathRequired);
        qCDebug(importsLog) << msg;
    }
}

void ImportDependencies::removeCoreImport(const QString &importId)
{
    if (!m_coreImports.contains(importId)) {
        qCWarning(importsLog) << "missing importId in removeCoreImport(" << importId << ")";
        return;
    }
    CoreImport &cImport = m_coreImports[importId];
    QList<Export> newExports;
    foreach (const Export &e, cImport.possibleExports)
        if (e.intrinsic)
            removeImportCacheEntry(e.exportName, importId);
        else
            newExports.append(e);
    if (newExports.size()>0)
        cImport.possibleExports = newExports;
    else
        m_coreImports.remove(importId);

    qCDebug(importsLog) << "removed import with id:"<< importId;
}

void ImportDependencies::removeImportCacheEntry(const ImportKey &importKey, const QString &importId)
{
    QStringList &cImp = m_importCache[importKey];
    if (!cImp.removeOne(importId)) {
        qCWarning(importsLog) << "missing possibleExport backpointer for " << importKey.toString() << " to "
                              << importId;
    }
    if (cImp.isEmpty())
        m_importCache.remove(importKey);
}

void ImportDependencies::addExport(const QString &importId, const ImportKey &importKey,
                                   const QString &requiredPath, const QString &typeName)
{
    if (!m_coreImports.contains(importId)) {
        CoreImport newImport(importId);
        newImport.language = Dialect::AnyLanguage;
        newImport.possibleExports.append(Export(importKey, requiredPath, false, typeName));
        m_coreImports.insert(newImport.importId, newImport);
        m_importCache[importKey].append(importId);
        return;
    }
    CoreImport &importValue = m_coreImports[importId];
    importValue.possibleExports.append(Export(importKey, requiredPath, false, typeName));
    m_importCache[importKey].append(importId);
    qCDebug(importsLog) << "added export "<< importKey.toString() << " for id " <<importId
                        << " (" << requiredPath << ")";
}

void ImportDependencies::removeExport(const QString &importId, const ImportKey &importKey,
                                      const QString &requiredPath, const QString &typeName)
{
    if (!m_coreImports.contains(importId)) {
        qCWarning(importsLog) << "non existing core import for removeExport(" << importId << ", "
                              << importKey.toString() << ")";
    } else {
        CoreImport &importValue = m_coreImports[importId];
        if (!importValue.possibleExports.removeOne(Export(importKey, requiredPath, false, typeName))) {
            qCWarning(importsLog) << "non existing export for removeExport(" << importId << ", "
                                  << importKey.toString() << ")";
        }
        if (importValue.possibleExports.isEmpty() && importValue.fingerprint.isEmpty())
            m_coreImports.remove(importId);
    }
    if (!m_importCache.contains(importKey)) {
        qCWarning(importsLog) << "missing possibleExport for " << importKey.toString()
                            << " when removing export of " << importId;
    } else {
        removeImportCacheEntry(importKey, importId);
    }
    qCDebug(importsLog) << "removed export "<< importKey.toString() << " for id " << importId
                        << " (" << requiredPath << ")";
}

void ImportDependencies::iterateOnLibraryImports(
        const ViewerContext &vContext,
        std::function<bool (const ImportMatchStrength &,
                            const Export &,
                            const CoreImport &)> const &iterF) const
{
    typedef QMap<ImportKey, QStringList>::const_iterator iter_t;
    ImportKey firstLib;
    firstLib.type = ImportType::Library;
    iter_t i = m_importCache.lowerBound(firstLib);
    iter_t end = m_importCache.constEnd();
    while (i != end && i.key().type == ImportType::Library) {
        qCDebug(importsLog) << "libloop:" << i.key().toString() << i.value();
        foreach (const QString &cImportName, i.value()) {
            CoreImport cImport = coreImport(cImportName);
            if (vContext.languageIsCompatible(cImport.language)) {
                foreach (const Export &e, cImport.possibleExports) {
                    if (e.visibleInVContext(vContext) && e.exportName.type == ImportType::Library) {
                        ImportMatchStrength m = e.exportName.matchImport(i.key(), vContext);
                        if (m.hasMatch()) {
                            qCDebug(importsLog) << "import iterate:" << e.exportName.toString()
                                                << " (" << e.pathRequired << "), id:" << cImport.importId;
                            if (!iterF(m, e, cImport))
                                return;
                        }
                    }
                }
            }
        }
        ++i;
    }
}

void ImportDependencies::iterateOnSubImports(
        const ImportKey &baseKey,
        const ViewerContext &vContext,
        std::function<bool (const ImportMatchStrength &,
                            const Export &,
                            const CoreImport &)> const &iterF) const
{
    typedef QMap<ImportKey, QStringList>::const_iterator iter_t;
    iter_t i = m_importCache.lowerBound(baseKey);
    iter_t end = m_importCache.constEnd();
    while (i != end) {
        ImportKey::DirCompareInfo c = baseKey.compareDir(i.key());
        if (c != ImportKey::SameDir && c != ImportKey::SecondInFirst)
            break;
        foreach (const QString &cImportName, i.value()) {
            CoreImport cImport = coreImport(cImportName);
            if (vContext.languageIsCompatible(cImport.language)) {
                foreach (const Export &e, cImport.possibleExports) {
                    if (e.visibleInVContext(vContext)) {
                        ImportMatchStrength m = e.exportName.matchImport(i.key(), vContext);
                        if (m.hasMatch()) {
                            if (!iterF(m, e, cImport))
                                return;
                        }
                    }
                }
            }
        }
        ++i;
    }
}

class CollectImportKeys {
public:
    QSet<ImportKey> &imports;
    CollectImportKeys(QSet<ImportKey> &imports)
        : imports(imports)
    { }
    bool operator()(const ImportMatchStrength &m,
                    const Export &e,
                    const CoreImport &cI) const
    {
        Q_UNUSED(m);
        Q_UNUSED(cI);
        imports.insert(e.exportName.flatKey());
        return true;
    }
};

QSet<ImportKey> ImportDependencies::libraryImports(const ViewerContext &viewContext) const
{
    QSet<ImportKey> res;
    CollectImportKeys importCollector(res);
    iterateOnLibraryImports(viewContext, importCollector);
    return res;
}

QSet<ImportKey> ImportDependencies::subdirImports(
        const ImportKey &baseKey, const ViewerContext &viewContext) const
{
    QSet<ImportKey> res;
    CollectImportKeys importCollector(res);
    iterateOnSubImports(baseKey, viewContext, importCollector);
    return res;
}

void ImportDependencies::checkConsistency() const
{
    QMapIterator<ImportKey, QStringList> j(m_importCache);
    while (j.hasNext()) {
        j.next();
        foreach (const QString &s, j.value()) {
            bool found = false;
            foreach (const Export &e, m_coreImports.value(s).possibleExports)
                if (e.exportName == j.key())
                    found = true;
            Q_ASSERT(found); Q_UNUSED(found);
        }
    }
    QMapIterator<QString,CoreImport> i(m_coreImports);
    while (i.hasNext()) {
        i.next();
        foreach (const Export &e, i.value().possibleExports) {
            if (!m_importCache.value(e.exportName).contains(i.key())) {
                qCWarning(importsLog) << e.exportName.toString();
                qCWarning(importsLog) << i.key();

                QMapIterator<ImportKey, QStringList> j(m_importCache);
                while (j.hasNext()) {
                    j.next();
                    qCWarning(importsLog) << j.key().toString() << j.value();
                }
                qCWarning(importsLog) << m_importCache.contains(e.exportName);
                qCWarning(importsLog) << m_importCache.value(e.exportName);
            }
            Q_ASSERT(m_importCache.value(e.exportName).contains(i.key()));
        }
    }
}

} // namespace QmlJS
