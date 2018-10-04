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
#include <coreplugin/progressmanager/progressmanager.h>
#include <utils/algorithm.h>
#include <utils/filesystemwatcher.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QStringList>
#include <QUrl>

#include <QHelpEngineCore>

#include <QMutexLocker>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>

using namespace Core;

static const char kUserDocumentationKey[] = "Help/UserDocumentation";
static const char kUpdateDocumentationTask[] = "UpdateDocumentationTask";

namespace Help {
namespace Internal {

struct HelpManagerPrivate
{
    HelpManagerPrivate() = default;
    ~HelpManagerPrivate();

    const QStringList documentationFromInstaller();
    void readSettings();
    void writeSettings();
    void cleanUpDocumentation();

    bool m_needsSetup = true;
    QHelpEngineCore *m_helpEngine = nullptr;
    Utils::FileSystemWatcher *m_collectionWatcher = nullptr;

    // data for delayed initialization
    QSet<QString> m_filesToRegister;
    QSet<QString> m_nameSpacesToUnregister;
    QHash<QString, QVariant> m_customValues;

    QSet<QString> m_userRegisteredFiles;

    QMutex m_helpengineMutex;
    QFuture<bool> m_registerFuture;
};

static HelpManager *m_instance = nullptr;
static HelpManagerPrivate *d = nullptr;

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
    delete d;
    m_instance = nullptr;
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
        for (const QString &filePath : files)
            d->m_filesToRegister.insert(filePath);
        return;
    }

    QFuture<bool> future = Utils::runAsync(&HelpManager::registerDocumentationNow, files);
    Utils::onResultReady(future, this, [](bool docsChanged){
        if (docsChanged) {
            d->m_helpEngine->setupData();
            emit Core::HelpManager::Signals::instance()->documentationChanged();
        }
    });
    ProgressManager::addTask(future, tr("Update Documentation"),
                             kUpdateDocumentationTask);
}

void HelpManager::registerDocumentationNow(QFutureInterface<bool> &futureInterface,
                                           const QStringList &files)
{
    QMutexLocker locker(&d->m_helpengineMutex);

    futureInterface.setProgressRange(0, files.count());
    futureInterface.setProgressValue(0);

    QHelpEngineCore helpEngine(collectionFilePath());
    helpEngine.setupData();
    bool docsChanged = false;
    QStringList nameSpaces = helpEngine.registeredDocumentations();
    for (const QString &file : files) {
        if (futureInterface.isCanceled())
            break;
        futureInterface.setProgressValue(futureInterface.progressValue() + 1);
        const QString &nameSpace = helpEngine.namespaceName(file);
        if (nameSpace.isEmpty())
            continue;
        if (!nameSpaces.contains(nameSpace)) {
            if (helpEngine.registerDocumentation(file)) {
                nameSpaces.append(nameSpace);
                docsChanged = true;
            } else {
                qWarning() << "Error registering namespace '" << nameSpace
                    << "' from file '" << file << "':" << helpEngine.error();
            }
        } else {
            const QLatin1String key("CreationDate");
            const QString &newDate = helpEngine.metaData(file, key).toString();
            const QString &oldDate = helpEngine.metaData(
                helpEngine.documentationFileName(nameSpace), key).toString();
            if (QDateTime::fromString(newDate, Qt::ISODate)
                > QDateTime::fromString(oldDate, Qt::ISODate)) {
                if (helpEngine.unregisterDocumentation(nameSpace)) {
                    docsChanged = true;
                    helpEngine.registerDocumentation(file);
                }
            }
        }
    }
    futureInterface.reportResult(docsChanged);
}

void HelpManager::unregisterDocumentation(const QStringList &nameSpaces)
{
    if (d->m_needsSetup) {
        for (const QString &name : nameSpaces)
            d->m_nameSpacesToUnregister.insert(name);
        return;
    }

    QMutexLocker locker(&d->m_helpengineMutex);
    bool docsChanged = false;
    for (const QString &nameSpace : nameSpaces) {
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
    locker.unlock();
    if (docsChanged)
        emit Core::HelpManager::Signals::instance()->documentationChanged();
}

void HelpManager::registerUserDocumentation(const QStringList &filePaths)
{
    for (const QString &filePath : filePaths)
        d->m_userRegisteredFiles.insert(filePath);
    m_instance->registerDocumentation(filePaths);
}

QSet<QString> HelpManager::userDocumentationPaths()
{
    return d->m_userRegisteredFiles;
}

// This should go into Qt 4.8 once we start using it for Qt Creator
QMap<QString, QUrl> HelpManager::linksForKeyword(const QString &key)
{
    QTC_ASSERT(!d->m_needsSetup, return {});
    return d->m_helpEngine->linksForKeyword(key);
}

QMap<QString, QUrl> HelpManager::linksForIdentifier(const QString &id)
{
    QMap<QString, QUrl> empty;
    QTC_ASSERT(!d->m_needsSetup, return {});
    return d->m_helpEngine->linksForIdentifier(id);
}

QUrl HelpManager::findFile(const QUrl &url)
{
    QTC_ASSERT(!d->m_needsSetup, return {});
    return d->m_helpEngine->findFile(url);
}

QByteArray HelpManager::fileData(const QUrl &url)
{
    QTC_ASSERT(!d->m_needsSetup, return {});
    return d->m_helpEngine->fileData(url);
}

void HelpManager::handleHelpRequest(const QUrl &url, Core::HelpManager::HelpViewerLocation location)
{
    emit m_instance->helpRequested(url, location);
}

QStringList HelpManager::registeredNamespaces()
{
    QTC_ASSERT(!d->m_needsSetup, return {});
    return d->m_helpEngine->registeredDocumentations();
}

QString HelpManager::namespaceFromFile(const QString &file)
{
    QTC_ASSERT(!d->m_needsSetup, return {});
    return d->m_helpEngine->namespaceName(file);
}

QString HelpManager::fileFromNamespace(const QString &nameSpace)
{
    QTC_ASSERT(!d->m_needsSetup, return {});
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
    QTC_ASSERT(!d->m_needsSetup, return {});
    return d->m_helpEngine->customValue(key, value);
}

HelpManager::Filters HelpManager::filters()
{
    QTC_ASSERT(!d->m_needsSetup, return {});

    Filters filters;
    const QStringList &customFilters = d->m_helpEngine->customFilters();
    for (const QString &filter : customFilters)
        filters.insert(filter, d->m_helpEngine->filterAttributes(filter));
    return filters;
}

HelpManager::Filters HelpManager::fixedFilters()
{
    QTC_ASSERT(!d->m_needsSetup, return {});

    const QLatin1String sqlite("QSQLITE");
    const QLatin1String name("HelpManager::fixedCustomFilters");

    Filters fixedFilters;
    DbCleaner cleaner(name);
    QSqlDatabase db = QSqlDatabase::addDatabase(sqlite, name);
    if (db.driver() && db.driver()->lastError().type() == QSqlError::NoError) {
        const QStringList &registeredDocs = d->m_helpEngine->registeredDocumentations();
        for (const QString &nameSpace : registeredDocs) {
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
    QTC_ASSERT(!d->m_needsSetup, return {});

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

void HelpManager::aboutToShutdown()
{
    if (d && d->m_registerFuture.isRunning()) {
        d->m_registerFuture.cancel();
        d->m_registerFuture.waitForFinished();
    }
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

    for (const QString &filePath : d->documentationFromInstaller())
        d->m_filesToRegister.insert(filePath);

    d->cleanUpDocumentation();

    if (!d->m_nameSpacesToUnregister.isEmpty()) {
        m_instance->unregisterDocumentation(d->m_nameSpacesToUnregister.toList());
        d->m_nameSpacesToUnregister.clear();
    }

    if (!d->m_filesToRegister.isEmpty()) {
        m_instance->registerDocumentation(d->m_filesToRegister.toList());
        d->m_filesToRegister.clear();
    }

    QHash<QString, QVariant>::const_iterator it;
    for (it = d->m_customValues.constBegin(); it != d->m_customValues.constEnd(); ++it)
        setCustomValue(it.key(), it.value());

    emit Core::HelpManager::Signals::instance()->setupFinished();
}

void HelpManagerPrivate::cleanUpDocumentation()
{
    // mark documentation for removal for which there is no documentation file anymore
    // mark documentation for removal that is neither user registered, nor marked for registration
    const QStringList &registeredDocs = m_helpEngine->registeredDocumentations();
    for (const QString &nameSpace : registeredDocs) {
        const QString filePath = m_helpEngine->documentationFileName(nameSpace);
        if (!QFileInfo::exists(filePath)
                || (!m_filesToRegister.contains(filePath)
                    && !m_userRegisteredFiles.contains(filePath))) {
            m_nameSpacesToUnregister.insert(nameSpace);
        }
    }
}

HelpManagerPrivate::~HelpManagerPrivate()
{
    writeSettings();
    delete m_helpEngine;
    m_helpEngine = nullptr;
}

const QStringList HelpManagerPrivate::documentationFromInstaller()
{
    QSettings *installSettings = ICore::settings();
    const QStringList documentationPaths = installSettings->value(QLatin1String("Help/InstalledDocumentation"))
            .toStringList();
    QStringList documentationFiles;
    for (const QString &path : documentationPaths) {
        QFileInfo pathInfo(path);
        if (pathInfo.isFile() && pathInfo.isReadable()) {
            documentationFiles << pathInfo.absoluteFilePath();
        } else if (pathInfo.isDir()) {
            const QFileInfoList files(QDir(path).entryInfoList(QStringList(QLatin1String("*.qch")),
                                                               QDir::Files | QDir::Readable));
            for (const QFileInfo &fileInfo : files)
                documentationFiles << fileInfo.absoluteFilePath();
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

} // Internal
} // Core
