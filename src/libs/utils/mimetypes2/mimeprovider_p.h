// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2015 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author David Faure <david.faure@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "mimedatabase_p.h"

#include "mimeglobpattern_p.h"
#include <QtCore/qdatetime.h>
#include <QtCore/qset.h>
#include <QtCore/qmap.h>

namespace Utils {

class MimeMagicRuleMatcher;
class MimeTypeXMLData;
class MimeProviderBase;

struct MimeMagicResult
{
    bool isValid() const { return !candidate.isEmpty(); }

    QString candidate;
    int accuracy = 0;
};

class MimeProviderBase
{
    Q_DISABLE_COPY(MimeProviderBase)

public:
    MimeProviderBase(MimeDatabasePrivate *db, const QString &directory);
    virtual ~MimeProviderBase() {}

    virtual bool isValid() = 0;
    virtual bool isInternalDatabase() const = 0;
    virtual bool knowsMimeType(const QString &name) = 0;
    virtual void addFileNameMatches(const QString &fileName, MimeGlobMatchResult &result) = 0;
    virtual void addParents(const QString &mime, QStringList &result) = 0;
    virtual QString resolveAlias(const QString &name) = 0;
    virtual void addAliases(const QString &name, QStringList &result) = 0;
    virtual void findByMagic(const QByteArray &data, MimeMagicResult &result) = 0;
    virtual void addAllMimeTypes(QList<MimeType> &result) = 0;
    virtual MimeTypePrivate::LocaleHash localeComments(const QString &name) = 0;
    virtual bool hasGlobDeleteAll(const QString &name) = 0;
    virtual QStringList globPatterns(const QString &name) = 0;
    virtual QString icon(const QString &name) = 0;
    virtual QString genericIcon(const QString &name) = 0;
    virtual void ensureLoaded() { }

    QString directory() const { return m_directory; }

    MimeProviderBase *overrideProvider() const;
    void setOverrideProvider(MimeProviderBase *provider);
    bool isMimeTypeGlobsExcluded(const QString &name) const;

    // added for Qt Creator
    virtual bool hasMimeTypeForName(const QString &name) = 0;
    virtual QStringList allMimeTypeNames() = 0;
    virtual QMap<int, QList<MimeMagicRule>> magicRulesForMimeType(const MimeType &mimeType) const = 0;
    virtual void setMagicRulesForMimeType(const MimeType &mimeType,
                                          const QMap<int, QList<MimeMagicRule>> &rules) = 0;
    virtual void setGlobPatternsForMimeType(const MimeType &mimeType, const QStringList &patterns)
        = 0;

    MimeDatabasePrivate *m_db;
    QString m_directory;

    MimeProviderBase *m_overrideProvider = nullptr; // more "local" than this one

    // for Qt Creator
    QSet<QString> m_overriddenMimeTypes;
};

/*
   Parses the files 'mime.cache' and 'types' on demand
 */
class MimeBinaryProvider : public MimeProviderBase
{
public:
    MimeBinaryProvider(MimeDatabasePrivate *db, const QString &directory);
    virtual ~MimeBinaryProvider();

    bool isValid() override;
    bool isInternalDatabase() const override;
    bool knowsMimeType(const QString &name) override;
    void addFileNameMatches(const QString &fileName, MimeGlobMatchResult &result) override;
    void addParents(const QString &mime, QStringList &result) override;
    QString resolveAlias(const QString &name) override;
    void addAliases(const QString &name, QStringList &result) override;
    void findByMagic(const QByteArray &data, MimeMagicResult &result) override;
    void addAllMimeTypes(QList<MimeType> &result) override;
    MimeTypePrivate::LocaleHash localeComments(const QString &name) override;
    bool hasGlobDeleteAll(const QString &name) override;
    QStringList globPatterns(const QString &name) override;
    QString icon(const QString &name) override;
    QString genericIcon(const QString &name) override;
    void ensureLoaded() override;

    // added for Qt Creator
    bool hasMimeTypeForName(const QString &name) override;
    QStringList allMimeTypeNames() override;
    QMap<int, QList<MimeMagicRule>> magicRulesForMimeType(const MimeType &mimeType) const override;
    void setMagicRulesForMimeType(const MimeType &mimeType,
                                  const QMap<int, QList<MimeMagicRule>> &rules) override;
    void setGlobPatternsForMimeType(const MimeType &mimeType, const QStringList &patterns) override;

private:
    struct CacheFile;

    int matchGlobList(MimeGlobMatchResult &result, CacheFile *cacheFile, int offset,
                      const QString &fileName);
    bool matchSuffixTree(MimeGlobMatchResult &result,
                         CacheFile *cacheFile,
                         int numEntries,
                         int firstOffset,
                         const QString &fileName,
                         qsizetype charPos,
                         bool caseSensitiveCheck);
    bool matchMagicRule(CacheFile *cacheFile, int numMatchlets, int firstOffset,
                        const QByteArray &data);
         QLatin1StringView iconForMime(CacheFile *cacheFile, int posListOffset, const QByteArray &inputMime);
    void loadMimeTypeList();
    bool checkCacheChanged();

    std::unique_ptr<CacheFile> m_cacheFile;
    QStringList m_cacheFileNames;
    QSet<QString> m_mimetypeNames;
    bool m_mimetypeListLoaded;
    struct MimeTypeExtra
    {
        QHash<QString, QString> localeComments;
        QStringList globPatterns;
        bool hasGlobDeleteAll = false;
    };
    using MimeTypeExtraMap = std::map<QString, MimeTypeExtra>;
    MimeTypeExtraMap m_mimetypeExtra;

    MimeTypeExtraMap::const_iterator loadMimeTypeExtra(const QString &mimeName);
};

/*
   Parses the raw XML files (slower)
 */
class MimeXMLProvider : public MimeProviderBase
{
public:
    enum InternalDatabaseEnum { InternalDatabase };
#if 1 // QT_CONFIG(mimetype_database)
    enum : bool { InternalDatabaseAvailable = true };
#else
    enum : bool { InternalDatabaseAvailable = false };
#endif
    MimeXMLProvider(MimeDatabasePrivate *db, InternalDatabaseEnum);
    MimeXMLProvider(MimeDatabasePrivate *db, const QString &directory);
    // added for Qt Creator
    MimeXMLProvider(MimeDatabasePrivate *db, const QString &directory, const QByteArray &data);
    ~MimeXMLProvider();

    bool isValid() override;
    bool isInternalDatabase() const override;
    bool knowsMimeType(const QString &name) override;
    void addFileNameMatches(const QString &fileName, MimeGlobMatchResult &result) override;
    void addParents(const QString &mime, QStringList &result) override;
    QString resolveAlias(const QString &name) override;
    void addAliases(const QString &name, QStringList &result) override;
    void findByMagic(const QByteArray &data, MimeMagicResult &result) override;
    void addAllMimeTypes(QList<MimeType> &result) override;
    void ensureLoaded() override;
    MimeTypePrivate::LocaleHash localeComments(const QString &name) override;
    bool hasGlobDeleteAll(const QString &name) override;
    QStringList globPatterns(const QString &name) override;
    QString icon(const QString &name) override;
    QString genericIcon(const QString &name) override;

    bool load(const QString &fileName, QString *errorMessage);

    // Called by the mimetype xml parser
    void addMimeType(const MimeTypeXMLData &mt);
    void addGlobPattern(const MimeGlobPattern &glob);
    void addParent(const QString &child, const QString &parent);
    void addAlias(const QString &alias, const QString &name);
    void addMagicMatcher(const MimeMagicRuleMatcher &matcher);

    // added for Qt Creator
    bool hasMimeTypeForName(const QString &name) override;
    QStringList allMimeTypeNames() override;
    QMap<int, QList<MimeMagicRule>> magicRulesForMimeType(const MimeType &mimeType) const override;
    void setMagicRulesForMimeType(const MimeType &mimeType,
                                  const QMap<int, QList<MimeMagicRule>> &rules) override;
    void setGlobPatternsForMimeType(const MimeType &mimeType, const QStringList &patterns) override;

private:
    void load(const QString &fileName);
    void load(const char *data, qsizetype len);

    typedef QHash<QString, MimeTypeXMLData> NameMimeTypeMap;
    NameMimeTypeMap m_nameMimeTypeMap;

    typedef QHash<QString, QString> AliasHash;
    AliasHash m_aliases;

    typedef QHash<QString, QStringList> ParentsHash;
    ParentsHash m_parents;
    MimeAllGlobPatterns m_mimeTypeGlobs;

    QList<MimeMagicRuleMatcher> m_magicMatchers;
    QStringList m_allFiles;
};

} // namespace Utils
