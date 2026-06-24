// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "helpmanager.h"
#include "localhelpmanager.h"

#include "helptr.h"

#include <coreplugin/helplink.h>
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

#include <QtTaskTree/QSequentialTaskTreeRunner>

#include <QHelpEngineCore>

#include <QtHelp/QHelpLink>

#include <algorithm>

static Q_LOGGING_CATEGORY(helpLog, "qtc.help.helpmanager", QtWarningMsg)

using namespace Core;
using namespace QtTaskTree;
using namespace Utils;

#if QT_VERSION >= QT_VERSION_CHECK(6, 12, 0)
static QList<HelpLink> qHelpLinks2Links(const QList<QHelpLink> &in)
{
    return Utils::transform(in, [](const QHelpLink &link) {
        return HelpLink{link.url, link.title};
    });
}
#else
// Pre 6.12, QHelpLib returned duplicate matches and unsorted results.
// Remove duplicate URLs and sort by title.
static bool titleLessThan(const HelpLink &lhs, const HelpLink &rhs)
{
    return lhs.title < rhs.title;
}

static QList<HelpLink> qHelpLinks2Links(const QList<QHelpLink> &in)
{
    QList<HelpLink> result;
    result.reserve(in.size());
    for (const auto &qLink : in) {
        const auto urlExists = [&qLink](const HelpLink &link){ return link.url == qLink.url; };
        if (std::none_of(result.cbegin(), result.cend(), urlExists))
            result.append({qLink.url, qLink.title});
    }
    std::stable_sort(result.begin(), result.end(), titleLessThan);
    return result;
}
#endif // < 6.12

namespace Help {
namespace Internal {

const char kUserDocumentationKey[] = "Help/UserDocumentation";
const char kUpdateDocumentationTask[] = "UpdateDocumentationTask";
const char kPurgeDocumentationTask[] = "PurgeDocumentationTask";

struct HelpManagerPrivate
{
    HelpManagerPrivate() = default;
    ~HelpManagerPrivate();

    const FilePaths documentationFromInstaller();
    void readSettings();
    void writeSettings();
    void cleanUpDocumentation();

    bool m_needsSetup = true;
    QHelpEngineCore *m_helpEngine = nullptr;
    Utils::FileSystemWatcher *m_collectionWatcher = nullptr;

    // data for delayed initialization
    QSet<FilePath> m_filesToRegister;
    QSet<FilePath> m_blockedDocumentation;
    QSet<FilePath> m_filesToUnregister;

    QSet<FilePath> m_userRegisteredFiles;

    QSequentialTaskTreeRunner m_taskTreeRunner;
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
    return ICore::userResourcePath("helpcollection.qhc").toUrlishString();
}

static void onDone(const Async<bool> &task)
{
    if (task.isResultAvailable() && task.result()) {
        d->m_helpEngine->setupData();
        emit Core::HelpManager::Signals::instance()->documentationChanged();
    }
}

static QHash<QString, QStringList> namespacesByFile(QHelpEngineCore &helpEngine)
{
    QHash<QString, QStringList> result;
    const QStringList registeredDocs = helpEngine.registeredDocumentations();
    for (const QString &nameSpace : registeredDocs) {
        const QString filePath
            = QFileInfo(helpEngine.documentationFileName(nameSpace)).canonicalFilePath();
        result[filePath].append(nameSpace);
    }
    return result;
}

static void registerDocumentationNow(
    QPromise<bool> &promise, const QString &collectionFilePath, const FilePaths &files)
{
    promise.setProgressRange(0, files.count());
    promise.setProgressValue(0);

    QHelpEngineCore helpEngine(collectionFilePath);
    helpEngine.setReadOnly(false);
    helpEngine.setupData();
    bool docsChanged = false;
    QSet<QString> registeredNamespaces = Utils::toSet(helpEngine.registeredDocumentations());
    QHash<QString, QStringList> nsByFile = namespacesByFile(helpEngine);
    for (const FilePath &file : files) {
        if (promise.isCanceled())
            break;
        promise.setProgressValue(promise.future().progressValue() + 1);
        const QString filePath = file.canonicalPath().path();
        const QString &nameSpace = QHelpEngineCore::namespaceName(filePath);
        if (nameSpace.isEmpty())
            continue;
        const QStringList existingNamespacesForFile = nsByFile.value(filePath);
        for (const QString &ns : existingNamespacesForFile) {
            if (ns != nameSpace) {
                // unregister out of date namespace
                if (helpEngine.unregisterDocumentation(ns)) {
                    docsChanged = true;
                } else {
                    qWarning() << "Error unregistering outdated namespace" << ns << "from file"
                               << file << ":" << helpEngine.error();
                }
            }
        }
        if (!registeredNamespaces.contains(nameSpace)) {
            if (helpEngine.registerDocumentation(file.path())) {
                docsChanged = true;
                registeredNamespaces.insert(nameSpace);
            } else {
                qWarning() << "Error registering namespace" << nameSpace << "from file" << file
                           << ":" << helpEngine.error();
            }
        }
        // set as handled if there are duplicates
        nsByFile.insert(filePath, {nameSpace});
    }
    promise.addResult(docsChanged);
}

void HelpManager::registerDocumentation(const FilePaths &files)
{
    if (d->m_needsSetup) {
        for (const FilePath &filePath : files)
            d->m_filesToRegister.insert(filePath);
        return;
    }

    if (files.isEmpty())
        return;

    const auto onSetup = [files](Async<bool> &task) {
        task.setConcurrentCallData(registerDocumentationNow, collectionFilePath(), files);
        QObject::connect(&task, &AsyncBase::started, &task, [taskPtr = &task] {
            ProgressManager::addTask(taskPtr->future(), Tr::tr("Update Documentation"),
                                     kUpdateDocumentationTask);
        });
    };
    d->m_taskTreeRunner.enqueue({AsyncTask<bool>(onSetup, onDone)});
}

void HelpManager::setBlockedDocumentation(const FilePaths &fileNames)
{
    for (const FilePath &filePath : fileNames)
        d->m_blockedDocumentation.insert(filePath);
}

static void unregisterDocumentationNow(
    QPromise<bool> &promise, const QString collectionFilePath, const FilePaths &files)
{
    promise.setProgressRange(0, files.count());
    promise.setProgressValue(0);

    bool docsChanged = false;

    QHelpEngineCore helpEngine(collectionFilePath);
    helpEngine.setReadOnly(false);
    helpEngine.setupData();
    QHash<QString, QStringList> nsByFile = namespacesByFile(helpEngine);
    for (const FilePath &file : files) {
        if (promise.isCanceled())
            break;
        promise.setProgressValue(promise.future().progressValue() + 1);
        const QString filePath = file.canonicalPath().path();
        // remove all namespaces that might refer to this file (e.g. if the file changed in between)
        const QStringList namespaces = nsByFile.value(filePath);
        nsByFile.remove(filePath); // a file may be listed more than once
        for (const QString &nameSpace : namespaces) {
            if (helpEngine.unregisterDocumentation(nameSpace)) {
                docsChanged = true;
            } else {
                qWarning() << "Error unregistering namespace" << nameSpace << "from file" << file
                           << ":" << helpEngine.error();
            }
        }
    }
    promise.addResult(docsChanged);
}

void HelpManager::unregisterDocumentation(const FilePaths &files)
{
    if (d->m_needsSetup) {
        for (const FilePath &file : files)
            d->m_filesToUnregister.insert(file);
        return;
    }

    if (files.isEmpty())
        return;

    d->m_userRegisteredFiles.subtract(Utils::toSet(files));

    const auto onSetup = [files](Async<bool> &task) {
        task.setConcurrentCallData(unregisterDocumentationNow, collectionFilePath(), files);
        QObject::connect(&task, &AsyncBase::started, &task, [taskPtr = &task] {
            ProgressManager::addTask(taskPtr->future(), Tr::tr("Purge Outdated Documentation"),
                                     kPurgeDocumentationTask);
        });
    };
    d->m_taskTreeRunner.enqueue({AsyncTask<bool>(onSetup, onDone)});
}

void HelpManager::registerUserDocumentation(const FilePaths &filePaths)
{
    for (const FilePath &filePath : filePaths)
        d->m_userRegisteredFiles.insert(filePath);
    m_instance->registerDocumentation(filePaths);
}

QSet<FilePath> HelpManager::userDocumentationPaths()
{
    return d->m_userRegisteredFiles;
}

QList<HelpLink> HelpManager::linksForKeyword(QHelpEngineCore *engine,
                                             const QString &key,
                                             std::optional<QString> filterName)
{
    const QList<QHelpLink> qLinks = filterName.has_value()
        ? engine->documentsForKeyword(key, filterName.value())
        : engine->documentsForKeyword(key);
    QList<Core::HelpLink> links = qHelpLinks2Links(qLinks);

    qCDebug(helpLog) << "Looking up help for keyword" << key
                     << (filterName.has_value() ? "with filter" : "without filter")
                     << " returned" << links.size() << "links";

    return links;
}

QList<HelpLink> HelpManager::linksForKeyword(const QString &key)
{
    QTC_ASSERT(!d->m_needsSetup, return {});
    if (key.isEmpty())
        return {};

    auto results = HelpManager::linksForKeyword(d->m_helpEngine, key, QString());

    qCDebug(helpLog) << "Looking up help for keyword" << key
                     << "returned" << results.size() << "links";

    return results;
}

QList<HelpLink> HelpManager::linksForIdentifier(const QString &id)
{
    QTC_ASSERT(!d->m_needsSetup, return {});
    if (id.isEmpty())
        return {};
    const auto qLinks = d->m_helpEngine->documentsForIdentifier(id, QString());
    QList<Core::HelpLink> links = qHelpLinks2Links(qLinks);

    qCDebug(helpLog) << "Looking up help for id" << id
                     << "returned" << links.size() << "links";

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

void HelpManager::addOnlineHelpHandler(const Core::HelpManager::OnlineHelpHandler &handler)
{
    LocalHelpManager::addOnlineHelpHandler(handler);
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

FilePath HelpManager::fileFromNamespace(const QString &nameSpace)
{
    QTC_ASSERT(!d->m_needsSetup, return {});
    return FilePath::fromString(d->m_helpEngine->documentationFileName(nameSpace));
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

    for (const FilePath &filePath : d->documentationFromInstaller())
        d->m_filesToRegister.insert(filePath);

    // The online installer registers documentation for Qt versions explicitly via an install
    // setting, which defeats that we only register the Qt versions matching the setting.
    // So the Qt support explicitly blocks the files that we do _not_ want to register, so the
    // Help plugin knows about this.
    d->m_filesToRegister -= d->m_blockedDocumentation;

    d->cleanUpDocumentation();

    if (!d->m_filesToUnregister.isEmpty()) {
        m_instance->unregisterDocumentation(Utils::toList(d->m_filesToUnregister));
        d->m_filesToUnregister.clear();
    }

    if (!d->m_filesToRegister.isEmpty()) {
        m_instance->registerDocumentation(Utils::toList(d->m_filesToRegister));
        d->m_filesToRegister.clear();
    }

    emit Core::HelpManager::Signals::instance()->setupFinished();
}

void HelpManagerPrivate::cleanUpDocumentation()
{
    // mark documentation for removal for which there is no documentation file anymore
    // mark documentation for removal that is neither user registered, nor marked for registration
    const QStringList &registeredDocs = m_helpEngine->registeredDocumentations();
    for (const QString &nameSpace : registeredDocs) {
        const FilePath filePath = FilePath::fromString(
            m_helpEngine->documentationFileName(nameSpace));
        if (!filePath.exists()
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

const FilePaths HelpManagerPrivate::documentationFromInstaller()
{
    QtcSettings *installSettings = ICore::settings();
    const auto documentationPaths = FilePaths::fromSettings(
        installSettings->value("Help/InstalledDocumentation"));
    FilePaths documentationFiles;
    for (const FilePath &path : documentationPaths) {
        if (path.isReadableFile()) {
            documentationFiles << path;
        } else if (path.isDir()) {
            documentationFiles += path.dirEntries(
                FileFilter({"*.qch"}, DirFilterFlag::Files | DirFilterFlag::Readable));
        }
    }
    return documentationFiles;
}

void HelpManagerPrivate::readSettings()
{
    m_userRegisteredFiles = Utils::toSet(
        FilePaths::fromSettings(ICore::settings()->value(kUserDocumentationKey)));
}

void HelpManagerPrivate::writeSettings()
{
    const QStringList list = Utils::transform(Utils::toList(m_userRegisteredFiles), &FilePath::path);
    ICore::settings()->setValueWithDefault(kUserDocumentationKey, list);
}

} // Internal
} // Core
