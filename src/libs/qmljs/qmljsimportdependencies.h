// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljsconstants.h"
#include "qmljsdialect.h"

#include <languageutils/componentversion.h>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>
#include <QSet>
#include <QSharedPointer>

#include <functional>

QT_BEGIN_NAMESPACE
class QCryptographicHash;
QT_END_NAMESPACE

namespace QmlJS {
class ImportInfo;
struct ViewerContext;
namespace Internal { class ImportDependenciesPrivate; }
class ImportDependencies;

// match strenght wrt to the selectors of a ViewerContext
// this is valid only within a ViewerContext
class QMLJS_EXPORT ImportMatchStrength
{
public:
    explicit ImportMatchStrength() {}
    ImportMatchStrength(const QList<int> &match);

    int compareMatch(const ImportMatchStrength &o) const;

    bool hasNoMatch();

    bool hasMatch();
private:
    friend bool operator ==(const ImportMatchStrength &m1, const ImportMatchStrength &m2);
    QList<int> m_match;
};
bool operator ==(const ImportMatchStrength &m1, const ImportMatchStrength &m2);
bool operator !=(const ImportMatchStrength &m1, const ImportMatchStrength &m2);
bool operator <(const ImportMatchStrength &m1, const ImportMatchStrength &m2);

/*!
 * \brief The ImportKey class represent an import (or export), and can be used as hash key
 *
 * This represent only what is to be imported, *not* how (i.e. no as clause)
 *
 * Order is defined so that files in the same directory are contiguous, and different
 * ImportKind are separated.
 * This is used to efficiently iterate just on library imports, or just on a directory
 * while preserving space.
 */
class QMLJS_EXPORT ImportKey
{
public:
    enum DirCompareInfo {
        SameDir,
        FirstInSecond,
        SecondInFirst,
        Different,
        Incompatible
    };

    explicit ImportKey();
    explicit ImportKey(const ImportInfo &info);
    ImportKey(ImportType::Enum type, const QString &path,
              int majorVersion = LanguageUtils::ComponentVersion::NoVersion,
              int minorVersion = LanguageUtils::ComponentVersion::NoVersion);

    ImportType::Enum type;
    QStringList splitPath;
    int majorVersion;
    int minorVersion;

    QString path() const;
    QString libraryQualifiedPath() const;

    void addToHash(QCryptographicHash &hash) const;
    ImportKey flatKey() const;

    // wrap QList in a special type?
    ImportMatchStrength matchImport(const ImportKey &o, const ViewerContext &vContext) const;
    int compare(const ImportKey &other) const;
    bool isDirectoryLike() const;
    DirCompareInfo compareDir(const ImportKey &other) const;
    QString toString() const;
};

size_t qHash(const ImportKey &info);
bool operator ==(const ImportKey &i1, const ImportKey &i2);
bool operator !=(const ImportKey &i1, const ImportKey &i2);
bool operator <(const ImportKey &i1, const ImportKey &i2);

class QMLJS_EXPORT Export
{
public:
    static QString libraryTypeName();
    Export();
    Export(ImportKey exportName,
           const Utils::FilePath &pathRequired,
           bool intrinsic = false,
           const QString &typeName = libraryTypeName());
    ImportKey exportName;
    Utils::FilePath pathRequired;
    QString typeName;
    bool intrinsic;
    bool visibleInVContext(const ViewerContext &vContext) const;


    friend bool operator==(const Export &i1, const Export &i2)
    {
        return i1.exportName == i2.exportName
                && i1.pathRequired == i2.pathRequired
                && i1.intrinsic == i2.intrinsic
                && i1.typeName == i2.typeName;
    }

    friend bool operator!=(const Export &i1, const Export &i2)
    {
        return !(i1 == i2);
    }

    friend bool operator<(const Export &i1, const Export &i2)
    {
        return std::tie(i1.intrinsic, i1.pathRequired, i1.typeName, i1.exportName)
            < std::tie(i2.intrinsic, i2.pathRequired, i2.typeName, i2.exportName);
    }
};

class QMLJS_EXPORT CoreImport
{
public:
    CoreImport();
    CoreImport(const QString &importId, const QList<Export> &possibleExports = QList<Export>(),
               Dialect language = Dialect::Qml, const QByteArray &fingerprint = QByteArray());
    template<typename E>
    void addPossibleExport(E &&e)
    {
        auto found = std::lower_bound(possibleExports.begin(), possibleExports.end(), e);
        if (found == possibleExports.end() || e != *found)
            possibleExports.insert(found, std::forward<E>(e));
    }

    QString importId;
    QList<Export> possibleExports;
    Dialect language;
    QByteArray fingerprint;
    bool valid();
};

class QMLJS_EXPORT DependencyInfo
{
public:
    typedef QSharedPointer<const DependencyInfo> ConstPtr;
    typedef QSharedPointer<DependencyInfo> Ptr;

    QByteArray calculateFingerprint(const ImportDependencies &deps);

    ImportKey rootImport;
    QSet<QString> allCoreImports;
    QSet<ImportKey> allImports;
    QByteArray fingerprint;
};

class QMLJS_EXPORT MatchedImport
{
public:
    MatchedImport();
    MatchedImport(ImportMatchStrength matchStrength, ImportKey importKey,
                  const QString &coreImportId);

    ImportMatchStrength matchStrength;
    ImportKey importKey;
    QString coreImportId;
    int compare(const MatchedImport &o) const;
};
bool operator ==(const MatchedImport &m1, const MatchedImport &m2);
bool operator !=(const MatchedImport &m1, const MatchedImport &m2);
bool operator <(const MatchedImport &m1, const MatchedImport &m2);

class QMLJS_EXPORT ImportDependencies
{
public:
    typedef QMap<ImportKey, QList<MatchedImport> > ImportElements;

    explicit ImportDependencies();
    ~ImportDependencies();

    void filter(const ViewerContext &vContext);

    CoreImport coreImport(const QString &importId) const;
    void iterateOnCandidateImports(const ImportKey &key, const ViewerContext &vContext,
                                   std::function<bool(const ImportMatchStrength &,
                                                        const Export &,
                                                        const CoreImport &)> const &iterF) const;
    ImportElements candidateImports(const ImportKey &key, const ViewerContext &vContext) const;

    QList<DependencyInfo::ConstPtr> createDependencyInfos(const ImportKey &mainDoc,
                                                          const ViewerContext &vContext) const;

    void addCoreImport(const CoreImport &import);
    void removeCoreImport(const QString &importId);

    void addExport(const QString &importId,
                   const ImportKey &importKey,
                   const Utils::FilePath &requiredPath,
                   const QString &typeName = Export::libraryTypeName());
    void removeExport(const QString &importId,
                      const ImportKey &importKey,
                      const Utils::FilePath &requiredPath,
                      const QString &typeName = Export::libraryTypeName());

    void iterateOnLibraryImports(const ViewerContext &vContext,
                                 std::function<bool(const ImportMatchStrength &,
                                                      const Export &,
                                                      const CoreImport &)> const &iterF) const;
    void iterateOnSubImports(const ImportKey &baseKey, const ViewerContext &vContext,
                             std::function<bool(const ImportMatchStrength &,
                                                  const Export &,
                                                  const CoreImport &)> const &iterF) const;

    QSet<ImportKey> libraryImports(const ViewerContext &viewContext) const;
    QSet<ImportKey> subdirImports(const ImportKey &baseKey, const ViewerContext &viewContext) const;
    void checkConsistency() const;
private:
    void removeImportCacheEntry(const ImportKey &importKey, const QString &importId);

    QMap<ImportKey, QStringList> m_importCache;
    QMap<QString, CoreImport> m_coreImports;
};

} // namespace QmlJS
