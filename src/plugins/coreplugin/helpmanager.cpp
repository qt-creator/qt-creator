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

#include "helpmanager.h"

#include <coreplugin/icore.h>
#include <utils/algorithm.h>
#include <utils/filesystemwatcher.h>
#include <utils/qtcassert.h>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QUrl>

#ifdef QT_HELP_LIB

#include <QHelpEngineCore>

#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>

static const char kUserDocumentationKey[] = "Help/UserDocumentation";

namespace Core {

struct HelpManagerPrivate
{
    HelpManagerPrivate() :
       m_needsSetup(true), m_helpEngine(0), m_collectionWatcher(0)
    {}

    QStringList documentationFromInstaller();
    void readSettings();
    void writeSettings();
    void cleanUpDocumentation();

    bool m_needsSetup;
    QHelpEngineCore *m_helpEngine;
    Utils::FileSystemWatcher *m_collectionWatcher;

    // data for delayed initialization
    QSet<QString> m_filesToRegister;
    QSet<QString> m_nameSpacesToUnregister;
    QHash<QString, QVariant> m_customValues;

    QSet<QString> m_userRegisteredFiles;
};

static HelpManager *m_instance = 0;
static HelpManagerPrivate *d;

static const char linksForKeyQuery[] = "SELECT d.Title, f.Name, e.Name, "
    "d.Name, a.Anchor FROM IndexTable a, FileNameTable d, FolderTable e, "
    "NamespaceTable f WHERE a.FileId=d.FileId AND d.FolderId=e.Id AND "
    "a.NamespaceId=f.Id AND a.Name='%1'";

// -- DbCleaner

struct DbCleaner
{
    DbCleaner(const QString &dbName) : name(dbName) {}
    ~DbCleaner() { QSqlDatabase::removeDatabase(name); }
    QString name;
};

// -- HelpManager

HelpManager::HelpManager(QObject *parent) :
    QObject(parent)
{
    QTC_CHECK(!m_instance);
    m_instance = this;
    d = new HelpManagerPrivate;
}

HelpManager::~HelpManager()
{
    d->writeSettings();
    delete d->m_helpEngine;
    d->m_helpEngine = 0;
    m_instance = 0;
    delete d;
}

HelpManager *HelpManager::instance()
{
    Q_ASSERT(m_instance);
    return m_instance;
}

QString HelpManager::collectionFilePath()
{
    return QDir::cleanPath(ICore::userResourcePath()
        + QLatin1String("/helpcollection.qhc"));
}

void HelpManager::registerDocumentation(const QStringList &files)
{
    if (d->m_needsSetup) {
        foreach (const QString &filePath, files)
            d->m_filesToRegister.insert(filePath);
        return;
    }

    bool docsChanged = false;
    foreach (const QString &file, files) {
        const QString &nameSpace = d->m_helpEngine->namespaceName(file);
        if (nameSpace.isEmpty())
            continue;
        if (!d->m_helpEngine->registeredDocumentations().contains(nameSpace)) {
            if (d->m_helpEngine->registerDocumentation(file)) {
                docsChanged = true;
            } else {
                qWarning() << "Error registering namespace '" << nameSpace
                    << "' from file '" << file << "':" << d->m_helpEngine->error();
            }
        } else {
            const QLatin1String key("CreationDate");
            const QString &newDate = d->m_helpEngine->metaData(file, key).toString();
            const QString &oldDate = d->m_helpEngine->metaData(
                d->m_helpEngine->documentationFileName(nameSpace), key).toString();
            if (QDateTime::fromString(newDate, Qt::ISODate)
                > QDateTime::fromString(oldDate, Qt::ISODate)) {
                if (d->m_helpEngine->unregisterDocumentation(nameSpace)) {
                    docsChanged = true;
                    d->m_helpEngine->registerDocumentation(file);
                }
            }
        }
    }
    if (docsChanged)
        emit m_instance->documentationChanged();
}

void HelpManager::unregisterDocumentation(const QStringList &nameSpaces)
{
    if (d->m_needsSetup) {
        foreach (const QString &name, nameSpaces)
            d->m_nameSpacesToUnregister.insert(name);
        return;
    }

    bool docsChanged = false;
    foreach (const QString &nameSpace, nameSpaces) {
        const QString filePath = d->m_helpEngine->documentationFileName(nameSpace);
        if (d->m_helpEngine->unregisterDocumentation(nameSpace)) {
            docsChanged = true;
            d->m_userRegisteredFiles.remove(filePath);
        } else {
            qWarning() << "Error unregistering namespace '" << nameSpace
                << "' from file '" << filePath
                << "': " << d->m_helpEngine->error();
        }
    }
    if (docsChanged)
        emit m_instance->documentationChanged();
}

void HelpManager::registerUserDocumentation(const QStringList &filePaths)
{
    foreach (const QString &filePath, filePaths)
        d->m_userRegisteredFiles.insert(filePath);
    registerDocumentation(filePaths);
}

QSet<QString> HelpManager::userDocumentationPaths()
{
    return d->m_userRegisteredFiles;
}

static QUrl buildQUrl(const QString &ns, const QString &folder,
    const QString &relFileName, const QString &anchor)
{
    QUrl url;
    url.setScheme(QLatin1String("qthelp"));
    url.setAuthority(ns);
    url.setPath(QLatin1Char('/') + folder + QLatin1Char('/') + relFileName);
    url.setFragment(anchor);
    return url;
}

// This should go into Qt 4.8 once we start using it for Qt Creator
QMap<QString, QUrl> HelpManager::linksForKeyword(const QString &key)
{
    QMap<QString, QUrl> links;
    QTC_ASSERT(!d->m_needsSetup, return links);

    const QLatin1String sqlite("QSQLITE");
    const QLatin1String name("HelpManager::linksForKeyword");

    DbCleaner cleaner(name);
    QSqlDatabase db = QSqlDatabase::addDatabase(sqlite, name);
    if (db.driver() && db.driver()->lastError().type() == QSqlError::NoError) {
        const QStringList &registeredDocs = d->m_helpEngine->registeredDocumentations();
        foreach (const QString &nameSpace, registeredDocs) {
            db.setDatabaseName(d->m_helpEngine->documentationFileName(nameSpace));
            if (db.open()) {
                QSqlQuery query = QSqlQuery(db);
                query.setForwardOnly(true);
                query.exec(QString::fromLatin1(linksForKeyQuery).arg(key));
                while (query.next()) {
                    QString title = query.value(0).toString();
                    if (title.isEmpty()) // generate a title + corresponding path
                        title = key + QLatin1String(" : ") + query.value(3).toString();
                    links.insertMulti(title, buildQUrl(query.value(1).toString(),
                        query.value(2).toString(), query.value(3).toString(),
                        query.value(4).toString()));
                }
            }
        }
    }
    return links;
}

QMap<QString, QUrl> HelpManager::linksForIdentifier(const QString &id)
{
    QMap<QString, QUrl> empty;
    QTC_ASSERT(!d->m_needsSetup, return empty);
    return d->m_helpEngine->linksForIdentifier(id);
}

QUrl HelpManager::findFile(const QUrl &url)
{
    QTC_ASSERT(!d->m_needsSetup, return QUrl());
    return d->m_helpEngine->findFile(url);
}

QByteArray HelpManager::fileData(const QUrl &url)
{
    QTC_ASSERT(!d->m_needsSetup, return QByteArray());
    return d->m_helpEngine->fileData(url);
}

void HelpManager::handleHelpRequest(const QUrl &url, HelpManager::HelpViewerLocation location)
{
    emit m_instance->helpRequested(url, location);
}

void HelpManager::handleHelpRequest(const QString &url, HelpViewerLocation location)
{
    handleHelpRequest(QUrl(url), location);
}

QStringList HelpManager::registeredNamespaces()
{
    QTC_ASSERT(!d->m_needsSetup, return QStringList());
    return d->m_helpEngine->registeredDocumentations();
}

QString HelpManager::namespaceFromFile(const QString &file)
{
    QTC_ASSERT(!d->m_needsSetup, return QString());
    return d->m_helpEngine->namespaceName(file);
}

QString HelpManager::fileFromNamespace(const QString &nameSpace)
{
    QTC_ASSERT(!d->m_needsSetup, return QString());
    return d->m_helpEngine->documentationFileName(nameSpace);
}

void HelpManager::setCustomValue(const QString &key, const QVariant &value)
{
    if (d->m_needsSetup) {
        d->m_customValues.insert(key, value);
        return;
    }
    if (d->m_helpEngine->setCustomValue(key, value))
        emit m_instance->collectionFileChanged();
}

QVariant HelpManager::customValue(const QString &key, const QVariant &value)
{
    QTC_ASSERT(!d->m_needsSetup, return QVariant());
    return d->m_helpEngine->customValue(key, value);
}

HelpManager::Filters HelpManager::filters()
{
    QTC_ASSERT(!d->m_needsSetup, return Filters());

    Filters filters;
    const QStringList &customFilters = d->m_helpEngine->customFilters();
    foreach (const QString &filter, customFilters)
        filters.insert(filter, d->m_helpEngine->filterAttributes(filter));
    return filters;
}

HelpManager::Filters HelpManager::fixedFilters()
{
    Filters fixedFilters;
    QTC_ASSERT(!d->m_needsSetup, return fixedFilters);

    const QLatin1String sqlite("QSQLITE");
    const QLatin1String name("HelpManager::fixedCustomFilters");

    DbCleaner cleaner(name);
    QSqlDatabase db = QSqlDatabase::addDatabase(sqlite, name);
    if (db.driver() && db.driver()->lastError().type() == QSqlError::NoError) {
        const QStringList &registeredDocs = d->m_helpEngine->registeredDocumentations();
        foreach (const QString &nameSpace, registeredDocs) {
            db.setDatabaseName(d->m_helpEngine->documentationFileName(nameSpace));
            if (db.open()) {
                QSqlQuery query = QSqlQuery(db);
                query.setForwardOnly(true);
                query.exec(QLatin1String("SELECT Name FROM FilterNameTable"));
                while (query.next()) {
                    const QString &filter = query.value(0).toString();
                    fixedFilters.insert(filter, d->m_helpEngine->filterAttributes(filter));
                }
            }
        }
    }
    return fixedFilters;
}

HelpManager::Filters HelpManager::userDefinedFilters()
{
    QTC_ASSERT(!d->m_needsSetup, return Filters());

    Filters all = filters();
    const Filters &fixed = fixedFilters();
    for (Filters::const_iterator it = fixed.constBegin(); it != fixed.constEnd(); ++it)
        all.remove(it.key());
    return all;
}

void HelpManager::removeUserDefinedFilter(const QString &filter)
{
    QTC_ASSERT(!d->m_needsSetup, return);

    if (d->m_helpEngine->removeCustomFilter(filter))
        emit m_instance->collectionFileChanged();
}

void HelpManager::addUserDefinedFilter(const QString &filter, const QStringList &attr)
{
    QTC_ASSERT(!d->m_needsSetup, return);

    if (d->m_helpEngine->addCustomFilter(filter, attr))
        emit m_instance->collectionFileChanged();
}

// -- private

void HelpManager::setupHelpManager()
{
    if (!d->m_needsSetup)
        return;
    d->m_needsSetup = false;

    d->readSettings();

    // create the help engine
    d->m_helpEngine = new QHelpEngineCore(collectionFilePath(), m_instance);
    d->m_helpEngine->setupData();

    foreach (const QString &filePath, d->documentationFromInstaller())
        d->m_filesToRegister.insert(filePath);

    d->cleanUpDocumentation();

    if (!d->m_nameSpacesToUnregister.isEmpty()) {
        unregisterDocumentation(d->m_nameSpacesToUnregister.toList());
        d->m_nameSpacesToUnregister.clear();
    }

    if (!d->m_filesToRegister.isEmpty()) {
        registerDocumentation(d->m_filesToRegister.toList());
        d->m_filesToRegister.clear();
    }

    QHash<QString, QVariant>::const_iterator it;
    for (it = d->m_customValues.constBegin(); it != d->m_customValues.constEnd(); ++it)
        setCustomValue(it.key(), it.value());

    emit m_instance->setupFinished();
}

void HelpManagerPrivate::cleanUpDocumentation()
{
    // mark documentation for removal for which there is no documentation file anymore
    // mark documentation for removal that is neither user registered, nor marked for registration
    const QStringList &registeredDocs = m_helpEngine->registeredDocumentations();
    foreach (const QString &nameSpace, registeredDocs) {
        const QString filePath = m_helpEngine->documentationFileName(nameSpace);
        if (!QFileInfo::exists(filePath)
                || (!m_filesToRegister.contains(filePath)
                    && !m_userRegisteredFiles.contains(filePath))) {
            m_nameSpacesToUnregister.insert(nameSpace);
        }
    }
}

QStringList HelpManagerPrivate::documentationFromInstaller()
{
    QSettings *installSettings = ICore::settings();
    QStringList documentationPaths = installSettings->value(QLatin1String("Help/InstalledDocumentation"))
            .toStringList();
    QStringList documentationFiles;
    foreach (const QString &path, documentationPaths) {
        QFileInfo pathInfo(path);
        if (pathInfo.isFile() && pathInfo.isReadable()) {
            documentationFiles << pathInfo.absoluteFilePath();
        } else if (pathInfo.isDir()) {
            QDir dir(path);
            foreach (const QFileInfo &fileInfo, dir.entryInfoList(QStringList(QLatin1String("*.qch")),
                                                              QDir::Files | QDir::Readable)) {
                documentationFiles << fileInfo.absoluteFilePath();
            }
        }
    }
    return documentationFiles;
}

void HelpManagerPrivate::readSettings()
{
    m_userRegisteredFiles = ICore::settings()->value(QLatin1String(kUserDocumentationKey))
            .toStringList().toSet();
}

void HelpManagerPrivate::writeSettings()
{
    const QStringList list = m_userRegisteredFiles.toList();
    ICore::settings()->setValue(QLatin1String(kUserDocumentationKey), list);
}

}   // Core

#else // QT_HELP_LIB

namespace Core {

HelpManager *HelpManager::instance() { return 0; }

QString HelpManager::collectionFilePath() { return QString(); }

void HelpManager::registerDocumentation(const QStringList &) {}
void HelpManager::unregisterDocumentation(const QStringList &) {}

void HelpManager::registerUserDocumentation(const QStringList &) {}
QSet<QString> HelpManager::userDocumentationPaths() { return {}; }

QMap<QString, QUrl> HelpManager::linksForKeyword(const QString &) { return {}; }
QMap<QString, QUrl> HelpManager::linksForIdentifier(const QString &) { return {}; }

QUrl HelpManager::findFile(const QUrl &) { return QUrl();}
QByteArray HelpManager::fileData(const QUrl &) { return QByteArray();}

QStringList HelpManager::registeredNamespaces() { return {}; }
QString HelpManager::namespaceFromFile(const QString &) { return QString(); }
QString HelpManager::fileFromNamespace(const QString &) { return QString(); }

void HelpManager::setCustomValue(const QString &, const QVariant &) {}
QVariant HelpManager::customValue(const QString &, const QVariant &) { return QVariant(); }

HelpManager::Filters filters() { return {}; }
HelpManager::Filters fixedFilters() { return {}; }

HelpManager::Filters userDefinedFilters() { return {}; }

void HelpManager::removeUserDefinedFilter(const QString &) {}
void HelpManager::addUserDefinedFilter(const QString &, const QStringList &) {}

void HelpManager::handleHelpRequest(const QUrl &, HelpManager::HelpViewerLocation) {}
void HelpManager::handleHelpRequest(const QString &, HelpViewerLocation) {}

HelpManager::HelpManager(QObject *) {}
HelpManager::~HelpManager() {}

void HelpManager::setupHelpManager() {}

} // namespace Core

#endif // QT_HELP_LIB
