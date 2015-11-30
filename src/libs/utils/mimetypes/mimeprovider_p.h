/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MIMEPROVIDER_P_H
#define MIMEPROVIDER_P_H

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
#include "mimemagicrule_p.h"

#include <QtCore/qdatetime.h>
#include <QtCore/qset.h>

namespace Utils {
namespace Internal {

class MimeMagicRuleMatcher;

class MimeProviderBase
{
public:
    MimeProviderBase(MimeDatabasePrivate *db);
    virtual ~MimeProviderBase() {}

    virtual bool isValid() = 0;
    virtual MimeType mimeTypeForName(const QString &name) = 0;
    virtual QStringList findByFileName(const QString &fileName, QString *foundSuffix) = 0;
    virtual QStringList parents(const QString &mime) = 0;
    virtual QString resolveAlias(const QString &name) = 0;
    virtual QStringList listAliases(const QString &name) = 0;
    virtual MimeType findByMagic(const QByteArray &data, int *accuracyPtr) = 0;
    virtual QList<MimeType> allMimeTypes() = 0;
    virtual void loadMimeTypePrivate(MimeTypePrivate &) {}
    virtual void loadIcon(MimeTypePrivate &) {}
    virtual void loadGenericIcon(MimeTypePrivate &) {}

    // Qt Creator additions
    virtual QMap<int, QList<MimeMagicRule> > magicRulesForMimeType(const MimeType &mimeType) = 0;
    virtual void setGlobPatternsForMimeType(const MimeType &mimeType, const QStringList &patterns) = 0;
    virtual void setMagicRulesForMimeType(const MimeType &mimeType, const QMap<int, QList<MimeMagicRule> > &rules) = 0;

    MimeDatabasePrivate *m_db;
protected:
    bool shouldCheck();
    QDateTime m_lastCheck;
};

///*
//   Parses the files 'mime.cache' and 'types' on demand
// */
//class MimeBinaryProvider : public MimeProviderBase
//{
//public:
//    MimeBinaryProvider(MimeDatabasePrivate *db);
//    virtual ~MimeBinaryProvider();

//    virtual bool isValid();
//    virtual MimeType mimeTypeForName(const QString &name);
//    virtual QStringList findByFileName(const QString &fileName, QString *foundSuffix);
//    virtual QStringList parents(const QString &mime);
//    virtual QString resolveAlias(const QString &name);
//    virtual QStringList listAliases(const QString &name);
//    virtual MimeType findByMagic(const QByteArray &data, int *accuracyPtr);
//    virtual QList<MimeType> allMimeTypes();
//    virtual void loadMimeTypePrivate(MimeTypePrivate &);
//    virtual void loadIcon(MimeTypePrivate &);
//    virtual void loadGenericIcon(MimeTypePrivate &);

//private:
//    struct CacheFile;

//    void matchGlobList(MimeGlobMatchResult &result, CacheFile *cacheFile, int offset, const QString &fileName);
//    bool matchSuffixTree(MimeGlobMatchResult &result, CacheFile *cacheFile, int numEntries, int firstOffset, const QString &fileName, int charPos, bool caseSensitiveCheck);
//    bool matchMagicRule(CacheFile *cacheFile, int numMatchlets, int firstOffset, const QByteArray &data);
//    QString iconForMime(CacheFile *cacheFile, int posListOffset, const QByteArray &inputMime);
//    void loadMimeTypeList();
//    void checkCache();

//    class CacheFileList : public QList<CacheFile *>
//    {
//    public:
//        CacheFile *findCacheFile(const QString &fileName) const;
//        bool checkCacheChanged();
//    };
//    CacheFileList m_cacheFiles;
//    QStringList m_cacheFileNames;
//    QSet<QString> m_mimetypeNames;
//    bool m_mimetypeListLoaded;
//};

/*
   Parses the raw XML files (slower)
 */
class MimeXMLProvider : public MimeProviderBase
{
public:
    MimeXMLProvider(MimeDatabasePrivate *db);

    virtual bool isValid();
    virtual MimeType mimeTypeForName(const QString &name);
    virtual QStringList findByFileName(const QString &fileName, QString *foundSuffix);
    virtual QStringList parents(const QString &mime);
    virtual QString resolveAlias(const QString &name);
    virtual QStringList listAliases(const QString &name);
    virtual MimeType findByMagic(const QByteArray &data, int *accuracyPtr);
    virtual QList<MimeType> allMimeTypes();

    bool load(const QString &fileName, QString *errorMessage);

    // Called by the mimetype xml parser
    void addMimeType(const MimeType &mt);
    void addGlobPattern(const MimeGlobPattern &glob);
    void addParent(const QString &child, const QString &parent);
    void addAlias(const QString &alias, const QString &name);
    void addMagicMatcher(const MimeMagicRuleMatcher &matcher);

    // Qt Creator additions
    void addFile(const QString &filePath);
    QMap<int, QList<MimeMagicRule> > magicRulesForMimeType(const MimeType &mimeType);
    void setGlobPatternsForMimeType(const MimeType &mimeType, const QStringList &patterns);
    void setMagicRulesForMimeType(const MimeType &mimeType, const QMap<int, QList<MimeMagicRule> > &rules);

private:
    void ensureLoaded();
    void load(const QString &fileName);

    bool m_loaded;

    typedef QHash<QString, MimeType> NameMimeTypeMap;
    NameMimeTypeMap m_nameMimeTypeMap;

    typedef QHash<QString, QString> AliasHash;
    AliasHash m_aliases;

    typedef QHash<QString, QStringList> ParentsHash;
    ParentsHash m_parents;
    MimeAllGlobPatterns m_mimeTypeGlobs;

    QList<MimeMagicRuleMatcher> m_magicMatchers;
    QStringList m_allFiles;

    // Qt Creator additions
    QStringList m_additionalFiles;
};

} // Internal
} // Utils

#endif // MIMEPROVIDER_P_H
