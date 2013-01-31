/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "helpmanager.h"

#include <coreplugin/icore.h>
#include <utils/filesystemwatcher.h>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QStringList>

#include <QHelpEngineCore>

#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>

namespace Core {

struct HelpManagerPrivate
{
    HelpManagerPrivate() :
       m_needsSetup(true), m_helpEngine(0), m_collectionWatcher(0)
    {}

    QStringList documentationFromInstaller();

    bool m_needsSetup;
    QHelpEngineCore *m_helpEngine;
    Utils::FileSystemWatcher *m_collectionWatcher;

    QStringList m_filesToRegister;
    QStringList m_nameSpacesToUnregister;
    QHash<QString, QVariant> m_customValues;
};

static HelpManager *m_instance = 0;

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
    QObject(parent), d(new HelpManagerPrivate)
{
    Q_ASSERT(!m_instance);
    m_instance = this;
}

HelpManager::~HelpManager()
{
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
    return QDir::cleanPath(Core::ICore::userResourcePath()
        + QLatin1String("/helpcollection.qhc"));
}

void HelpManager::registerDocumentation(const QStringList &files)
{
    if (d->m_needsSetup) {
        d->m_filesToRegister.append(files);
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
        emit documentationChanged();
}

void HelpManager::unregisterDocumentation(const QStringList &nameSpaces)
{
    if (d->m_needsSetup) {
        d->m_nameSpacesToUnregister.append(nameSpaces);
        return;
    }

    bool docsChanged = false;
    foreach (const QString &nameSpace, nameSpaces) {
        if (d->m_helpEngine->unregisterDocumentation(nameSpace)) {
            docsChanged = true;
        } else {
            qWarning() << "Error unregistering namespace '" << nameSpace
                << "' from file '" << d->m_helpEngine->documentationFileName(nameSpace)
                << "': " << d->m_helpEngine->error();
        }
    }
    if (docsChanged)
        emit documentationChanged();
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
QMap<QString, QUrl> HelpManager::linksForKeyword(const QString &key) const
{
    QMap<QString, QUrl> links;
    if (d->m_needsSetup)
        return links;

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

QMap<QString, QUrl> HelpManager::linksForIdentifier(const QString &id) const
{
    if (d->m_needsSetup)
        return QMap<QString, QUrl>();
    return d->m_helpEngine->linksForIdentifier(id);
}

// This should go into Qt 4.8 once we start using it for Qt Creator
QStringList HelpManager::findKeywords(const QString &key, int maxHits) const
{
    if (d->m_needsSetup)
        return QStringList();

    const QLatin1String sqlite("QSQLITE");
    const QLatin1String name("HelpManager::findKeywords");

    QSet<QString> keywords;
    QSet<QString> keywordsToSort;

    DbCleaner cleaner(name);
    QSqlDatabase db = QSqlDatabase::addDatabase(sqlite, name);
    if (db.driver() && db.driver()->lastError().type() == QSqlError::NoError) {
        QHelpEngineCore core(collectionFilePath());
        core.setAutoSaveFilter(false);
        core.setCurrentFilter(tr("Unfiltered"));
        core.setupData();
        const QStringList &registeredDocs = core.registeredDocumentations();
        foreach (const QString &nameSpace, registeredDocs) {
            db.setDatabaseName(core.documentationFileName(nameSpace));
            if (db.open()) {
                QSqlQuery query = QSqlQuery(db);
                query.setForwardOnly(true);
                query.exec(QString::fromLatin1("SELECT DISTINCT Name FROM IndexTable WHERE Name LIKE "
                    "'%%1%' LIMIT %2").arg(key, QString::number(maxHits)));
                while (query.next()) {
                    const QString &keyValue = query.value(0).toString();
                    if (!keyValue.isEmpty()) {
                        if (keyValue.startsWith(key, Qt::CaseInsensitive))
                            keywordsToSort.insert(keyValue);
                        else
                            keywords.insert(keyValue);
                    }
                }
            }
        }
    }

    QStringList keywordsSorted = keywordsToSort.toList();
    qSort(keywordsSorted.begin(), keywordsSorted.end());
    return keywordsSorted + keywords.toList();
}

QUrl HelpManager::findFile(const QUrl &url) const
{
    if (d->m_needsSetup)
        return QUrl();
    return d->m_helpEngine->findFile(url);
}

QByteArray HelpManager::fileData(const QUrl &url) const
{
    if (d->m_needsSetup)
        return QByteArray();
    return d->m_helpEngine->fileData(url);
}

void HelpManager::handleHelpRequest(const QString &url)
{
    emit helpRequested(QUrl(url));
}

QStringList HelpManager::registeredNamespaces() const
{
    if (d->m_needsSetup)
        return QStringList();
    return d->m_helpEngine->registeredDocumentations();
}

QString HelpManager::namespaceFromFile(const QString &file) const
{
    if (d->m_needsSetup)
        return QString();
    return d->m_helpEngine->namespaceName(file);
}

QString HelpManager::fileFromNamespace(const QString &nameSpace) const
{
    if (d->m_needsSetup)
        return QString();
    return d->m_helpEngine->documentationFileName(nameSpace);
}

void HelpManager::setCustomValue(const QString &key, const QVariant &value)
{
    if (d->m_needsSetup) {
        d->m_customValues.insert(key, value);
        return;
    }
    if (d->m_helpEngine->setCustomValue(key, value))
        emit collectionFileChanged();
}

QVariant HelpManager::customValue(const QString &key, const QVariant &value) const
{
    if (d->m_needsSetup)
        return QVariant();
    return d->m_helpEngine->customValue(key, value);
}

HelpManager::Filters HelpManager::filters() const
{
    if (d->m_needsSetup)
        return Filters();

    Filters filters;
    const QStringList &customFilters = d->m_helpEngine->customFilters();
    foreach (const QString &filter, customFilters)
        filters.insert(filter, d->m_helpEngine->filterAttributes(filter));
    return filters;
}

HelpManager::Filters HelpManager::fixedFilters() const
{
    Filters fixedFilters;
    if (d->m_needsSetup)
        return fixedFilters;

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

HelpManager::Filters HelpManager::userDefinedFilters() const
{
    if (d->m_needsSetup)
        return Filters();

    Filters all = filters();
    const Filters &fixed = fixedFilters();
    for (Filters::const_iterator it = fixed.constBegin(); it != fixed.constEnd(); ++it)
        all.remove(it.key());
    return all;
}

void HelpManager::removeUserDefinedFilter(const QString &filter)
{
    if (d->m_needsSetup)
        return;

    if (d->m_helpEngine->removeCustomFilter(filter))
        emit collectionFileChanged();
}

void HelpManager::addUserDefinedFilter(const QString &filter, const QStringList &attr)
{
    if (d->m_needsSetup)
        return;

    if (d->m_helpEngine->addCustomFilter(filter, attr))
        emit collectionFileChanged();
}

// -- private slots

void HelpManager::setupHelpManager()
{
    if (!d->m_needsSetup)
        return;
    d->m_needsSetup = false;

    d->m_helpEngine = new QHelpEngineCore(collectionFilePath(), this);
    d->m_helpEngine->setAutoSaveFilter(false);
    d->m_helpEngine->setCurrentFilter(tr("Unfiltered"));
    d->m_helpEngine->setupData();
    verifyDocumenation();

    if (!d->m_nameSpacesToUnregister.isEmpty()) {
        unregisterDocumentation(d->m_nameSpacesToUnregister);
        d->m_nameSpacesToUnregister.clear();
    }

    d->m_filesToRegister << d->documentationFromInstaller();

    if (!d->m_filesToRegister.isEmpty()) {
        registerDocumentation(d->m_filesToRegister);
        d->m_filesToRegister.clear();
    }

    QHash<QString, QVariant>::const_iterator it;
    for (it = d->m_customValues.constBegin(); it != d->m_customValues.constEnd(); ++it)
        setCustomValue(it.key(), it.value());

    emit setupFinished();
}

// -- private

void HelpManager::verifyDocumenation()
{
    const QStringList &registeredDocs = d->m_helpEngine->registeredDocumentations();
    foreach (const QString &nameSpace, registeredDocs) {
        if (!QFileInfo(d->m_helpEngine->documentationFileName(nameSpace)).exists())
            d->m_nameSpacesToUnregister.append(nameSpace);
    }
}

QStringList HelpManagerPrivate::documentationFromInstaller()
{
    QSettings *installSettings = Core::ICore::settings();
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

}   // Core
