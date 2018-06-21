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

#include "qmljsqrcparser.h"
#include "qmljsconstants.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QDomDocument>
#include <QLocale>
#include <QMutex>
#include <QSet>
#include <QMutexLocker>
#include <QMultiHash>
#include <QCoreApplication>
#include <utils/qtcassert.h>

namespace QmlJS {

namespace Internal {
/*!
 * \class QrcParser
 * \brief Parses one or more qrc files, and keeps their content cached
 *
 * A Qrc resource contains files read from the filesystem but organized in a possibly different way.
 *
 * To easily describe that with a simple structure we use a map from qrc paths to the paths in the
 * filesystem.
 * By using a map we can easily find all qrc paths that start with a given prefix, and thus loop
 * on a qrc directory.
 *
 * Qrc files also support languages, those are mapped to a prefix of the qrc path.
 * For example the french /image/bla.png (lang=fr) will have the path "fr/image/bla.png".
 * The empty language represent the default resource.
 * Languages are looked up using the locale uiLanguages() property
 *
 * For a single qrc a given path maps to a single file, but when one has multiple
 * (platform specific exclusive) qrc files, then multiple files match, so QStringList are used.
 *
 * Especially the collect* functions are thought as low level interface.
 */
class QrcParserPrivate
{
    Q_DECLARE_TR_FUNCTIONS(QmlJS::QrcParser)
public:
    typedef QMap<QString,QStringList> SMap;
    QrcParserPrivate(QrcParser *q);
    bool parseFile(const QString &path, const QString &contents);
    QString firstFileAtPath(const QString &path, const QLocale &locale) const;
    void collectFilesAtPath(const QString &path, QStringList *res, const QLocale *locale = 0) const;
    bool hasDirAtPath(const QString &path, const QLocale *locale = 0) const;
    void collectFilesInPath(const QString &path, QMap<QString,QStringList> *res, bool addDirs = false,
                            const QLocale *locale = 0) const;
    void collectResourceFilesForSourceFile(const QString &sourceFile, QStringList *res,
                                           const QLocale *locale = 0) const;

    QStringList errorMessages() const;
    QStringList languages() const;
private:
    static QString fixPrefix(const QString &prefix);
    QStringList allUiLanguages(const QLocale *locale) const;

    SMap m_resources;
    SMap m_files;
    QStringList m_languages;
    QStringList m_errorMessages;
};

class QrcCachePrivate
{
    Q_DECLARE_TR_FUNCTIONS(QmlJS::QrcCachePrivate)
public:
    QrcCachePrivate(QrcCache *q);
    QrcParser::Ptr addPath(const QString &path, const QString &contents);
    void removePath(const QString &path);
    QrcParser::Ptr updatePath(const QString &path, const QString &contents);
    QrcParser::Ptr parsedPath(const QString &path);
    void clear();
private:
    QHash<QString, QPair<QrcParser::Ptr,int> > m_cache;
    QMutex m_mutex;
};
} // namespace Internal

/*! \brief normalizes the path to a file in a qrc resource by dropping the "qrc:/" or ":" and
 *         any extra slashes at the beginning
 */
QString QrcParser::normalizedQrcFilePath(const QString &path) {
    QString normPath = path;
    int endPrefix = 0;
    if (path.startsWith(QLatin1String("qrc:/")))
        endPrefix = 4;
    else if (path.startsWith(QLatin1String(":/")))
        endPrefix = 1;
    if (endPrefix < path.size() && path.at(endPrefix) == QLatin1Char('/'))
        while (endPrefix + 1 < path.size() && path.at(endPrefix+1) == QLatin1Char('/'))
            ++endPrefix;
    normPath = path.right(path.size()-endPrefix);
    if (!normPath.startsWith(QLatin1Char('/')))
        normPath.insert(0, QLatin1Char('/'));
    return normPath;
}

/*! \brief normalizes the path to a directory in a qrc resource by dropping the "qrc:/" or ":" and
 *         any extra slashes at the beginning, and ensuring it ends with a slash
 */
QString QrcParser::normalizedQrcDirectoryPath(const QString &path) {
    QString normPath = normalizedQrcFilePath(path);
    if (!normPath.endsWith(QLatin1Char('/')))
        normPath.append(QLatin1Char('/'));
    return normPath;
}

QString QrcParser::qrcDirectoryPathForQrcFilePath(const QString &file)
{
    return file.left(file.lastIndexOf(QLatin1Char('/')));
}

QrcParser::QrcParser()
{
    d = new Internal::QrcParserPrivate(this);
}

QrcParser::~QrcParser()
{
    delete d;
}

bool QrcParser::parseFile(const QString &path, const QString &contents)
{
    return d->parseFile(path, contents);
}

/*! \brief returns fs path of the first (active) file at the given qrc path
 */
QString QrcParser::firstFileAtPath(const QString &path, const QLocale &locale) const
{
    return d->firstFileAtPath(path, locale);
}

/*! \brief adds al the fs paths for the given qrc path to *res
 * If locale is null all possible files are added, otherwise just the first match
 * using that locale.
 */
void QrcParser::collectFilesAtPath(const QString &path, QStringList *res, const QLocale *locale) const
{
    d->collectFilesAtPath(path, res, locale);
}

/*! \brief returns true if the given path is a non empty directory
 */
bool QrcParser::hasDirAtPath(const QString &path, const QLocale *locale) const
{
    return d->hasDirAtPath(path, locale);
}

/*! \brief adds the directory contents of the given qrc path to res
 *
 * adds the qrcFileName => fs paths associations contained in the given qrc path
 * to res. If addDirs is true directories are also added.
 * If locale is null all possible files are added, otherwise just the first match
 * using that locale.
 */
void QrcParser::collectFilesInPath(const QString &path, QMap<QString,QStringList> *res, bool addDirs,
                                   const QLocale *locale) const
{
    d->collectFilesInPath(path, res, addDirs, locale);
}

void QrcParser::collectResourceFilesForSourceFile(const QString &sourceFile, QStringList *res,
                                                  const QLocale *locale) const
{
    d->collectResourceFilesForSourceFile(sourceFile, res, locale);
}

/*! \brief returns the errors found while parsing
 */
QStringList QrcParser::errorMessages() const
{
    return d->errorMessages();
}

/*! \brief returns all languages used in this qrc resource
 */
QStringList QrcParser::languages() const
{
    return d->languages();
}

/*! \brief if the contents are valid
 */
bool QrcParser::isValid() const
{
    return errorMessages().isEmpty();
}

QrcParser::Ptr QrcParser::parseQrcFile(const QString &path, const QString &contents)
{
    Ptr res(new QrcParser);
    if (!path.isEmpty())
        res->parseFile(path, contents);
    return res;
}

// ----------------

QrcCache::QrcCache()
{
    d = new Internal::QrcCachePrivate(this);
}

QrcCache::~QrcCache()
{
    delete d;
}

QrcParser::ConstPtr QrcCache::addPath(const QString &path, const QString &contents)
{
    return d->addPath(path, contents);
}

void QrcCache::removePath(const QString &path)
{
    d->removePath(path);
}

QrcParser::ConstPtr QrcCache::updatePath(const QString &path, const QString &contents)
{
    return d->updatePath(path, contents);
}

QrcParser::ConstPtr QrcCache::parsedPath(const QString &path)
{
    return d->parsedPath(path);
}

void QrcCache::clear()
{
    d->clear();
}

// --------------------

namespace Internal {

QrcParserPrivate::QrcParserPrivate(QrcParser *)
{ }

bool QrcParserPrivate::parseFile(const QString &path, const QString &contents)
{
    QDomDocument doc;
    QDir baseDir(QFileInfo(path).path());

    if (contents.isEmpty()) {
        // Regular file
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            m_errorMessages.append(file.errorString());
            return false;
        }

        QString error_msg;
        int error_line, error_col;
        if (!doc.setContent(&file, &error_msg, &error_line, &error_col)) {
            m_errorMessages.append(tr("XML error on line %1, col %2: %3")
                                   .arg(error_line).arg(error_col).arg(error_msg));
            return false;
        }
    } else {
        // Virtual file from qmake evaluator
        QString error_msg;
        int error_line, error_col;
        if (!doc.setContent(contents, &error_msg, &error_line, &error_col)) {
            m_errorMessages.append(tr("XML error on line %1, col %2: %3")
                                   .arg(error_line).arg(error_col).arg(error_msg));
            return false;
        }
    }

    QDomElement root = doc.firstChildElement(QLatin1String("RCC"));
    if (root.isNull()) {
        m_errorMessages.append(tr("The <RCC> root element is missing."));
        return false;
    }

    QDomElement relt = root.firstChildElement(QLatin1String("qresource"));
    for (; !relt.isNull(); relt = relt.nextSiblingElement(QLatin1String("qresource"))) {

        QString prefix = fixPrefix(relt.attribute(QLatin1String("prefix")));
        const QString language = relt.attribute(QLatin1String("lang"));
        if (!m_languages.contains(language))
            m_languages.append(language);

        QDomElement felt = relt.firstChildElement(QLatin1String("file"));
        for (; !felt.isNull(); felt = felt.nextSiblingElement(QLatin1String("file"))) {
            const QString fileName = felt.text();
            const QString alias = felt.attribute(QLatin1String("alias"));
            QString filePath = baseDir.absoluteFilePath(fileName);
            QString accessPath;
            if (!alias.isEmpty())
                accessPath = language + prefix + alias;
            else
                accessPath = language + prefix + fileName;
            QStringList &resources = m_resources[accessPath];
            if (!resources.contains(filePath))
                resources.append(filePath);
            QStringList &files = m_files[filePath];
            if (!files.contains(accessPath))
                files.append(accessPath);
        }
    }
    return true;
}

// path is assumed to be a normalized absolute path
QString QrcParserPrivate::firstFileAtPath(const QString &path, const QLocale &locale) const
{
    QTC_CHECK(path.startsWith(QLatin1Char('/')));
    QStringList langs = allUiLanguages(&locale);
    foreach (const QString &language, langs) {
        if (m_languages.contains(language)) {
            SMap::const_iterator res = m_resources.find(language + path);
            if (res != m_resources.end())
                return res.value().at(0);
        }
    }
    return QString();
}

void QrcParserPrivate::collectFilesAtPath(const QString &path, QStringList *files,
                                          const QLocale *locale) const
{
    QTC_CHECK(path.startsWith(QLatin1Char('/')));
    QStringList langs = allUiLanguages(locale);
    foreach (const QString &language, langs) {
        if (m_languages.contains(language)) {
            SMap::const_iterator res = m_resources.find(language + path);
            if (res != m_resources.end())
                (*files) << res.value();
        }
    }
}

// path is expected to be normalized and start and end with a slash
bool QrcParserPrivate::hasDirAtPath(const QString &path, const QLocale *locale) const
{
    QTC_CHECK(path.startsWith(QLatin1Char('/')));
    QTC_CHECK(path.endsWith(QLatin1Char('/')));
    QStringList langs = allUiLanguages(locale);
    foreach (const QString &language, langs) {
        if (m_languages.contains(language)) {
            QString key = language + path;
            SMap::const_iterator res = m_resources.lowerBound(key);
            if (res != m_resources.end() && res.key().startsWith(key))
                return true;
        }
    }
    return false;
}

void QrcParserPrivate::collectFilesInPath(const QString &path, QMap<QString,QStringList> *contents,
                                          bool addDirs, const QLocale *locale) const
{
    QTC_CHECK(path.startsWith(QLatin1Char('/')));
    QTC_CHECK(path.endsWith(QLatin1Char('/')));
    SMap::const_iterator end = m_resources.end();
    QStringList langs = allUiLanguages(locale);
    foreach (const QString &language, langs) {
        QString key = language + path;
        SMap::const_iterator res = m_resources.lowerBound(key);
        while (res != end && res.key().startsWith(key)) {
            const QString &actualKey = res.key();
            int endDir = actualKey.indexOf(QLatin1Char('/'), key.size());
            if (endDir == -1) {
                QString fileName = res.key().right(res.key().size()-key.size());
                QStringList &els = (*contents)[fileName];
                foreach (const QString &val, res.value())
                    if (!els.contains(val))
                        els << val;
                ++res;
            } else {
                QString dirName = res.key().mid(key.size(), endDir - key.size() + 1);
                if (addDirs)
                    contents->insert(dirName, QStringList());
                QString key2 = key + dirName;
                do {
                    ++res;
                } while (res != end && res.key().startsWith(key2));
            }
        }
    }
}

void QrcParserPrivate::collectResourceFilesForSourceFile(const QString &sourceFile,
                                                         QStringList *results,
                                                         const QLocale *locale) const
{
    // TODO: use FileName from fileutils for file pathes

    QStringList langs = allUiLanguages(locale);
    SMap::const_iterator file = m_files.find(sourceFile);
    if (file == m_files.end())
        return;
    foreach (const QString &resource, file.value()) {
        foreach (const QString &language, langs) {
            if (resource.startsWith(language) && !results->contains(resource))
                results->append(resource);
        }
    }
}

QStringList QrcParserPrivate::errorMessages() const
{
    return m_errorMessages;
}

QStringList QrcParserPrivate::languages() const
{
    return m_languages;
}

QString QrcParserPrivate::fixPrefix(const QString &prefix)
{
    const QChar slash = QLatin1Char('/');
    QString result = QString(slash);
    for (int i = 0; i < prefix.size(); ++i) {
        const QChar c = prefix.at(i);
        if (c == slash && result.at(result.size() - 1) == slash)
            continue;
        result.append(c);
    }

    if (!result.endsWith(slash))
        result.append(slash);

    return result;
}

QStringList QrcParserPrivate::allUiLanguages(const QLocale *locale) const
{
    if (!locale)
        return languages();
    QStringList langs = locale->uiLanguages();
    foreach (const QString &language, langs) { // qt4 support
        if (language.contains(QLatin1Char('_')) || language.contains(QLatin1Char('-'))) {
            QStringList splits = QString(language).replace(QLatin1Char('_'), QLatin1Char('-'))
                    .split(QLatin1Char('-'));
            if (splits.size() > 1 && !langs.contains(splits.at(0)))
                langs.append(splits.at(0));
        }
    }
    if (!langs.contains(QString()))
        langs.append(QString());
    return langs;
}

// ----------------

QrcCachePrivate::QrcCachePrivate(QrcCache *)
{ }

QrcParser::Ptr QrcCachePrivate::addPath(const QString &path, const QString &contents)
{
    QPair<QrcParser::Ptr,int> currentValue;
    {
        QMutexLocker l(&m_mutex);
        currentValue = m_cache.value(path, {QrcParser::Ptr(0), 0});
        currentValue.second += 1;
        if (currentValue.second > 1) {
            m_cache.insert(path, currentValue);
            return currentValue.first;
        }
    }
    QrcParser::Ptr newParser = QrcParser::parseQrcFile(path, contents);
    if (!newParser->isValid())
        qCWarning(qmljsLog) << "adding invalid qrc " << path << " to the cache:" << newParser->errorMessages();
    {
        QMutexLocker l(&m_mutex);
        QPair<QrcParser::Ptr,int> currentValue = m_cache.value(path, {QrcParser::Ptr(0), 0});
        if (currentValue.first.isNull())
            currentValue.first = newParser;
        currentValue.second += 1;
        m_cache.insert(path, currentValue);
        return currentValue.first;
    }
}

void QrcCachePrivate::removePath(const QString &path)
{
    QPair<QrcParser::Ptr,int> currentValue;
    {
        QMutexLocker l(&m_mutex);
        currentValue = m_cache.value(path, {QrcParser::Ptr(0), 0});
        if (currentValue.second == 1) {
            m_cache.remove(path);
        } else if (currentValue.second > 1) {
            currentValue.second -= 1;
            m_cache.insert(path, currentValue);
        } else {
            QTC_CHECK(!m_cache.contains(path));
        }
    }
}

QrcParser::Ptr QrcCachePrivate::updatePath(const QString &path, const QString &contents)
{
    QrcParser::Ptr newParser = QrcParser::parseQrcFile(path, contents);
    {
        QMutexLocker l(&m_mutex);
        QPair<QrcParser::Ptr,int> currentValue = m_cache.value(path, {QrcParser::Ptr(0), 0});
        currentValue.first = newParser;
        if (currentValue.second == 0)
            currentValue.second = 1; // add qrc files that are not in the resources of a project
        m_cache.insert(path, currentValue);
        return currentValue.first;
    }
}

QrcParser::Ptr QrcCachePrivate::parsedPath(const QString &path)
{
    QMutexLocker l(&m_mutex);
    QPair<QrcParser::Ptr,int> currentValue = m_cache.value(path, {QrcParser::Ptr(0), 0});
    return currentValue.first;
}

void QrcCachePrivate::clear()
{
    QMutexLocker l(&m_mutex);
    m_cache.clear();
}

} // namespace Internal
} // namespace QmlJS
