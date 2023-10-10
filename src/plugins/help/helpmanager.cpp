// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "helpmanager.h"

#include "helptr.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/filesystemwatcher.h>
#include <utils/qtcassert.h>

#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QPromise>
#include <QStringList>
#include <QUrl>

#include <QHelpEngineCore>

#include <QMutexLocker>

#include <QtHelp/QHelpLink>

using namespace Core;
using namespace Utils;

namespace Help {
namespace Internal {

const char kUserDocumentationKey[] = "Help/UserDocumentation";
const char kUpdateDocumentationTask[] = "UpdateDocumentationTask";
const char kPurgeDocumentationTask[] = "PurgeDocumentationTask";

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
    QSet<QString> m_filesToUnregister;
    QHash<QString, QVariant> m_customValues;

    QSet<QString> m_userRegisteredFiles;

    QMutex m_helpengineMutex;
    QFuture<bool> m_registerFuture;
};

static HelpManager *m_instance = nullptr;
static HelpManagerPrivate *d = nullptr;

// -- HelpManager

HelpManager::HelpManager(QObject *parent) :
    QObject(parent)
{
    QTC_CHECK(!m_instance);
    m_instance = this;
    d = new HelpManagerPrivate;
    QDesktopServices::setUrlHandler("qthelp", this, "showHelpUrl");
}

HelpManager::~HelpManager()
{
    QDesktopServices::unsetUrlHandler("qthelp");
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
    return ICore::userResourcePath("helpcollection.qhc").toString();
}

static void registerDocumentationNow(QPromise<bool> &promise, const QString &collectionFilePath,
                                     const QStringList &files)
{
    QMutexLocker locker(&d->m_helpengineMutex);

    promise.setProgressRange(0, files.count());
    promise.setProgressValue(0);

    QHelpEngineCore helpEngine(collectionFilePath);
    helpEngine.setReadOnly(false);
    helpEngine.setupData();
    bool docsChanged = false;
    QStringList nameSpaces = helpEngine.registeredDocumentations();
    for (const QString &file : files) {
        if (promise.isCanceled())
            break;
        promise.setProgressValue(promise.future().progressValue() + 1);
        const QString &nameSpace = QHelpEngineCore::namespaceName(file);
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
        }
    }
    promise.addResult(docsChanged);
}

void HelpManager::registerDocumentation(const QStringList &files)
{
    if (d->m_needsSetup) {
        for (const QString &filePath : files)
            d->m_filesToRegister.insert(filePath);
        return;
    }

    QFuture<bool> future = Utils::asyncRun(&registerDocumentationNow, collectionFilePath(), files);
    Utils::onResultReady(future, this, [](bool docsChanged){
        if (docsChanged) {
            d->m_helpEngine->setupData();
            emit Core::HelpManager::Signals::instance()->documentationChanged();
        }
    });
    ProgressManager::addTask(future, Tr::tr("Update Documentation"), kUpdateDocumentationTask);
}

static void unregisterDocumentationNow(QPromise<bool> &promise,
                                       const QString collectionFilePath,
                                       const QStringList &files)
{
    QMutexLocker locker(&d->m_helpengineMutex);

    promise.setProgressRange(0, files.count());
    promise.setProgressValue(0);

    bool docsChanged = false;

    QHelpEngineCore helpEngine(collectionFilePath);
    helpEngine.setReadOnly(false);
    helpEngine.setupData();
    for (const QString &file : files) {
        if (promise.isCanceled())
            break;
        promise.setProgressValue(promise.future().progressValue() + 1);
        const QString nameSpace = QHelpEngineCore::namespaceName(file);
        const QString filePath = helpEngine.documentationFileName(nameSpace);
        if (filePath.isEmpty()) // wasn't registered anyhow, ignore
            continue;
        if (helpEngine.unregisterDocumentation(nameSpace)) {
            docsChanged = true;

        } else {
            qWarning() << "Error unregistering namespace '" << nameSpace
                       << "' from file '" << filePath
                       << "': " << helpEngine.error();
        }
    }
    promise.addResult(docsChanged);
}

void HelpManager::unregisterDocumentation(const QStringList &files)
{
    if (d->m_needsSetup) {
        for (const QString &file : files)
            d->m_filesToUnregister.insert(file);
        return;
    }

    if (files.isEmpty())
        return;

    d->m_userRegisteredFiles.subtract(Utils::toSet(files));
    QFuture<bool> future = Utils::asyncRun(&unregisterDocumentationNow, collectionFilePath(), files);
    Utils::onResultReady(future, this, [](bool docsChanged){
        if (docsChanged) {
            d->m_helpEngine->setupData();
            emit Core::HelpManager::Signals::instance()->documentationChanged();
        }
    });
    ProgressManager::addTask(future,
                             Tr::tr("Purge Outdated Documentation"),
                             kPurgeDocumentationTask);
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

QMultiMap<QString, QUrl> HelpManager::linksForKeyword(QHelpEngineCore *engine,
                                                      const QString &key,
                                                      std::optional<QString> filterName)
{
    QMultiMap<QString, QUrl> links;
    const QList<QHelpLink> docs = filterName.has_value()
                                      ? engine->documentsForKeyword(key, filterName.value())
                                      : engine->documentsForKeyword(key);

    for (const auto &doc : docs)
        links.insert(doc.title, doc.url);

    // Remove duplicates (workaround for QTBUG-108131)
    links.removeIf([&links](const QMultiMap<QString, QUrl>::iterator it) {
        return links.find(it.key(), it.value()) != it;
    });

    return links;
}

QMultiMap<QString, QUrl> HelpManager::linksForKeyword(const QString &key)
{
    QTC_ASSERT(!d->m_needsSetup, return {});
    if (key.isEmpty())
        return {};
    return HelpManager::linksForKeyword(d->m_helpEngine, key, QString());
}

QMultiMap<QString, QUrl> HelpManager::linksForIdentifier(const QString &id)
{
    QTC_ASSERT(!d->m_needsSetup, return {});
    if (id.isEmpty())
        return {};
    QMultiMap<QString, QUrl> links;
    const QList<QHelpLink> docs = d->m_helpEngine->documentsForIdentifier(id, QString());
    for (const auto &doc : docs)
        links.insert(doc.title, doc.url);
    return links;
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

void HelpManager::showHelpUrl(const QUrl &url, Core::HelpManager::HelpViewerLocation location)
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
    return QHelpEngineCore::namespaceName(file);
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
    d->m_helpEngine->setReadOnly(false);
    d->m_helpEngine->setUsesFilterEngine(true);
    d->m_helpEngine->setupData();

    for (const QString &filePath : d->documentationFromInstaller())
        d->m_filesToRegister.insert(filePath);

    d->cleanUpDocumentation();

    if (!d->m_filesToUnregister.isEmpty()) {
        m_instance->unregisterDocumentation(Utils::toList(d->m_filesToUnregister));
        d->m_filesToUnregister.clear();
    }

    if (!d->m_filesToRegister.isEmpty()) {
        m_instance->registerDocumentation(Utils::toList(d->m_filesToRegister));
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
            m_filesToUnregister.insert(filePath);
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
    QtcSettings *installSettings = ICore::settings();
    const QStringList documentationPaths = installSettings->value("Help/InstalledDocumentation")
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
    m_userRegisteredFiles = Utils::toSet(ICore::settings()->value(kUserDocumentationKey).toStringList());
}

void HelpManagerPrivate::writeSettings()
{
    const QStringList list = Utils::toList(m_userRegisteredFiles);
    ICore::settings()->setValueWithDefault(kUserDocumentationKey, list);
}

} // Internal
} // Core
