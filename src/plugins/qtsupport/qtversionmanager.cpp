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

#include "qtversionmanager.h"

#include "baseqtversion.h"
#include "exampleslistmodel.h"
#include "qtkitinformation.h"
#include "qtsupportconstants.h"
#include "qtversionfactory.h"

#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/toolchainmanager.h>

#include <utils/algorithm.h>
#include <utils/buildablehelperlibrary.h>
#include <utils/environment.h>
#include <utils/filesystemwatcher.h>
#include <utils/hostosinfo.h>
#include <utils/persistentsettings.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QFile>
#include <QLoggingCategory>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>
#include <QTextStream>
#include <QTimer>

using namespace Utils;

namespace QtSupport {

using namespace Internal;

const char QTVERSION_DATA_KEY[] = "QtVersion.";
const char QTVERSION_TYPE_KEY[] = "QtVersion.Type";
const char QTVERSION_FILE_VERSION_KEY[] = "Version";
const char QTVERSION_FILENAME[] = "/qtversion.xml";

using VersionMap = QMap<int, BaseQtVersion *>;
static VersionMap m_versions;

const char DOCUMENTATION_SETTING_KEY[] = "QtSupport/DocumentationSetting";

static int m_idcount = 0;
// managed by QtProjectManagerPlugin
static QtVersionManager *m_instance = nullptr;
static FileSystemWatcher *m_configFileWatcher = nullptr;
static QTimer *m_fileWatcherTimer = nullptr;
static PersistentSettingsWriter *m_writer = nullptr;
static QVector<ExampleSetModel::ExtraExampleSet> m_pluginRegisteredExampleSets;

static Q_LOGGING_CATEGORY(log, "qtc.qt.versions", QtWarningMsg);

static FilePath globalSettingsFileName()
{
    return FilePath::fromString(Core::ICore::installerResourcePath() + QTVERSION_FILENAME);
}

static FilePath settingsFileName(const QString &path)
{
    return FilePath::fromString(Core::ICore::userResourcePath() + path);
}


// prefer newer qts otherwise compare on id
bool qtVersionNumberCompare(BaseQtVersion *a, BaseQtVersion *b)
{
    return a->qtVersion() > b->qtVersion() || (a->qtVersion() == b->qtVersion() && a->uniqueId() < b->uniqueId());
}
static bool restoreQtVersions();
static void findSystemQt();
static void saveQtVersions();

QVector<ExampleSetModel::ExtraExampleSet> ExampleSetModel::pluginRegisteredExampleSets()
{
    return m_pluginRegisteredExampleSets;
}

// --------------------------------------------------------------------------
// QtVersionManager
// --------------------------------------------------------------------------

QtVersionManager::QtVersionManager()
{
    m_instance = this;
    m_configFileWatcher = nullptr;
    m_fileWatcherTimer = new QTimer(this);
    m_writer = nullptr;
    m_idcount = 1;

    qRegisterMetaType<FilePath>();

    // Give the file a bit of time to settle before reading it...
    m_fileWatcherTimer->setInterval(2000);
    connect(m_fileWatcherTimer, &QTimer::timeout, this, [this] { updateFromInstaller(); });
}

void QtVersionManager::triggerQtVersionRestore()
{
    disconnect(ProjectExplorer::ToolChainManager::instance(), &ProjectExplorer::ToolChainManager::toolChainsLoaded,
               this, &QtVersionManager::triggerQtVersionRestore);

    bool success = restoreQtVersions();
    m_instance->updateFromInstaller(false);
    if (!success) {
        // We did neither restore our settings or upgraded
        // in that case figure out if there's a qt in path
        // and add it to the Qt versions
        findSystemQt();
    }

    emit m_instance->qtVersionsLoaded();
    emit m_instance->qtVersionsChanged(m_versions.keys(), QList<int>(), QList<int>());
    saveQtVersions();

    const FilePath configFileName = globalSettingsFileName();
    if (configFileName.exists()) {
        m_configFileWatcher = new FileSystemWatcher(m_instance);
        connect(m_configFileWatcher, &FileSystemWatcher::fileChanged,
                m_fileWatcherTimer, QOverload<>::of(&QTimer::start));
        m_configFileWatcher->addFile(configFileName.toString(),
                                     FileSystemWatcher::WatchModifiedDate);
    } // exists

    const QList<BaseQtVersion *> vs = versions();
    updateDocumentation(vs, {}, vs);
}

bool QtVersionManager::isLoaded()
{
    return m_writer;
}

QtVersionManager::~QtVersionManager()
{
    delete m_writer;
    qDeleteAll(m_versions);
    m_versions.clear();
}

void QtVersionManager::initialized()
{
    connect(ProjectExplorer::ToolChainManager::instance(), &ProjectExplorer::ToolChainManager::toolChainsLoaded,
            QtVersionManager::instance(), &QtVersionManager::triggerQtVersionRestore);
}

QtVersionManager *QtVersionManager::instance()
{
    return m_instance;
}

static bool restoreQtVersions()
{
    QTC_ASSERT(!m_writer, return false);
    m_writer = new PersistentSettingsWriter(settingsFileName(QTVERSION_FILENAME),
                                            "QtCreatorQtVersions");

    const QList<QtVersionFactory *> factories = QtVersionFactory::allQtVersionFactories();

    PersistentSettingsReader reader;
    const FilePath filename = settingsFileName(QTVERSION_FILENAME);

    if (!reader.load(filename))
        return false;
    QVariantMap data = reader.restoreValues();

    // Check version:
    const int version = data.value(QTVERSION_FILE_VERSION_KEY, 0).toInt();
    if (version < 1)
        return false;

    const QString keyPrefix(QTVERSION_DATA_KEY);
    const QVariantMap::ConstIterator dcend = data.constEnd();
    for (QVariantMap::ConstIterator it = data.constBegin(); it != dcend; ++it) {
        const QString &key = it.key();
        if (!key.startsWith(keyPrefix))
            continue;
        bool ok;
        int count = key.midRef(keyPrefix.count()).toInt(&ok);
        if (!ok || count < 0)
            continue;

        const QVariantMap qtversionMap = it.value().toMap();
        const QString type = qtversionMap.value(QTVERSION_TYPE_KEY).toString();

        bool restored = false;
        for (QtVersionFactory *f : factories) {
            if (f->canRestore(type)) {
                if (BaseQtVersion *qtv = f->restore(type, qtversionMap)) {
                    if (m_versions.contains(qtv->uniqueId())) {
                        // This shouldn't happen, we are restoring the same id multiple times?
                        qWarning() << "A Qt version with id"<<qtv->uniqueId()<<"already exists";
                        delete qtv;
                    } else {
                        m_versions.insert(qtv->uniqueId(), qtv);
                        m_idcount = qtv->uniqueId() > m_idcount ? qtv->uniqueId() : m_idcount;
                        restored = true;
                        break;
                    }
                }
            }
        }
        if (!restored)
            qWarning("Warning: Unable to restore Qt version '%s' stored in %s.",
                     qPrintable(type),
                     qPrintable(filename.toUserOutput()));
    }
    ++m_idcount;

    return true;
}

void QtVersionManager::updateFromInstaller(bool emitSignal)
{
    m_fileWatcherTimer->stop();

    const FilePath path = globalSettingsFileName();
    // Handle overwritting of data:
    if (m_configFileWatcher) {
        m_configFileWatcher->removeFile(path.toString());
        m_configFileWatcher->addFile(path.toString(), FileSystemWatcher::WatchModifiedDate);
    }

    QList<int> added;
    QList<int> removed;
    QList<int> changed;

    const QList<QtVersionFactory *> factories = QtVersionFactory::allQtVersionFactories();
    PersistentSettingsReader reader;
    QVariantMap data;
    if (reader.load(path))
        data = reader.restoreValues();

    if (log().isDebugEnabled()) {
        qCDebug(log) << "======= Existing Qt versions =======";
        for (BaseQtVersion *version : qAsConst(m_versions)) {
            qCDebug(log) << version->qmakeCommand().toString() << "id:"<<version->uniqueId();
            qCDebug(log) << "  autodetection source:"<< version->autodetectionSource();
            qCDebug(log) << "";
        }
        qCDebug(log)<< "======= Adding sdk versions =======";
    }

    QStringList sdkVersions;

    const QString keyPrefix(QTVERSION_DATA_KEY);
    const QVariantMap::ConstIterator dcend = data.constEnd();
    for (QVariantMap::ConstIterator it = data.constBegin(); it != dcend; ++it) {
        const QString &key = it.key();
        if (!key.startsWith(keyPrefix))
            continue;
        bool ok;
        int count = key.midRef(keyPrefix.count()).toInt(&ok);
        if (!ok || count < 0)
            continue;

        QVariantMap qtversionMap = it.value().toMap();
        const QString type = qtversionMap.value(QTVERSION_TYPE_KEY).toString();
        const QString autoDetectionSource = qtversionMap.value("autodetectionSource").toString();
        sdkVersions << autoDetectionSource;
        int id = -1; // see BaseQtVersion::fromMap()
        QtVersionFactory *factory = nullptr;
        for (QtVersionFactory *f : factories) {
            if (f->canRestore(type))
                factory = f;
        }
        if (!factory) {
            qCDebug(log, "Warning: Unable to find factory for type '%s'", qPrintable(type));
            continue;
        }
        // First try to find a existing Qt version to update
        bool restored = false;
        const VersionMap versionsCopy = m_versions; // m_versions is modified in loop
        for (BaseQtVersion *v : versionsCopy) {
            if (v->autodetectionSource() == autoDetectionSource) {
                id = v->uniqueId();
                qCDebug(log) << " Qt version found with same autodetection source" << autoDetectionSource << " => Migrating id:" << id;
                m_versions.remove(id);
                qtversionMap[Constants::QTVERSIONID] = id;
                qtversionMap[Constants::QTVERSIONNAME] = v->unexpandedDisplayName();
                delete v;

                if (BaseQtVersion *qtv = factory->restore(type, qtversionMap)) {
                    Q_ASSERT(qtv->isAutodetected());
                    m_versions.insert(id, qtv);
                    restored = true;
                }
                if (restored)
                    changed << id;
                else
                    removed << id;
            }
        }
        // Create a new qtversion
        if (!restored) { // didn't replace any existing versions
            qCDebug(log) << " No Qt version found matching" << autoDetectionSource << " => Creating new version";
            if (BaseQtVersion *qtv = factory->restore(type, qtversionMap)) {
                Q_ASSERT(qtv->isAutodetected());
                m_versions.insert(qtv->uniqueId(), qtv);
                added << qtv->uniqueId();
                restored = true;
            }
        }
        if (!restored) {
            qCDebug(log, "Warning: Unable to update qtversion '%s' from sdk installer.",
                    qPrintable(autoDetectionSource));
        }
    }

    if (log().isDebugEnabled()) {
        qCDebug(log) << "======= Before removing outdated sdk versions =======";
        for (BaseQtVersion *version : qAsConst(m_versions)) {
            qCDebug(log) << version->qmakeCommand().toString() << "id:"<<version->uniqueId();
            qCDebug(log) << "  autodetection source:"<< version->autodetectionSource();
            qCDebug(log) << "";
        }
    }
    const VersionMap versionsCopy = m_versions; // m_versions is modified in loop
    for (BaseQtVersion *qtVersion : versionsCopy) {
        if (qtVersion->autodetectionSource().startsWith("SDK.")) {
            if (!sdkVersions.contains(qtVersion->autodetectionSource())) {
                qCDebug(log) << "  removing version"<<qtVersion->autodetectionSource();
                m_versions.remove(qtVersion->uniqueId());
                removed << qtVersion->uniqueId();
            }
        }
    }

    if (log().isDebugEnabled()) {
        qCDebug(log)<< "======= End result =======";
        for (BaseQtVersion *version : qAsConst(m_versions)) {
            qCDebug(log) << version->qmakeCommand().toString() << "id:" << version->uniqueId();
            qCDebug(log) << "  autodetection source:"<< version->autodetectionSource();
            qCDebug(log) << "";
        }
    }
    if (emitSignal)
        emit qtVersionsChanged(added, removed, changed);
}

static void saveQtVersions()
{
    if (!m_writer)
        return;

    QVariantMap data;
    data.insert(QTVERSION_FILE_VERSION_KEY, 1);

    int count = 0;
    for (BaseQtVersion *qtv : qAsConst(m_versions)) {
        QVariantMap tmp = qtv->toMap();
        if (tmp.isEmpty())
            continue;
        tmp.insert(QTVERSION_TYPE_KEY, qtv->type());
        data.insert(QString::fromLatin1(QTVERSION_DATA_KEY) + QString::number(count), tmp);
        ++count;
    }
    m_writer->save(data, Core::ICore::dialogParent());
}

// Executes qtchooser with arguments in a process and returns its output
static QList<QByteArray> runQtChooser(const QString &qtchooser, const QStringList &arguments)
{
    QProcess p;
    p.start(qtchooser, arguments);
    p.waitForFinished();
    const bool success = p.exitCode() == 0;
    return success ? p.readAllStandardOutput().split('\n') : QList<QByteArray>();
}

// Asks qtchooser for the qmake path of a given version
static QString qmakePath(const QString &qtchooser, const QString &version)
{
    const QList<QByteArray> outputs = runQtChooser(qtchooser,
                                                   {QStringLiteral("-qt=%1").arg(version),
                                                    QStringLiteral("-print-env")});
    for (const QByteArray &output : outputs) {
        if (output.startsWith("QTTOOLDIR=\"")) {
            QByteArray withoutVarName = output.mid(11); // remove QTTOOLDIR="
            withoutVarName.chop(1); // remove trailing quote
            return QStandardPaths::findExecutable(QStringLiteral("qmake"), QStringList()
                                                  << QString::fromLocal8Bit(withoutVarName));
        }
    }
    return QString();
}

static FilePaths gatherQmakePathsFromQtChooser()
{
    const QString qtchooser = QStandardPaths::findExecutable(QStringLiteral("qtchooser"));
    if (qtchooser.isEmpty())
        return FilePaths();

    const QList<QByteArray> versions = runQtChooser(qtchooser, QStringList("-l"));
    QSet<FilePath> foundQMakes;
    for (const QByteArray &version : versions) {
        FilePath possibleQMake = FilePath::fromString(
                    qmakePath(qtchooser, QString::fromLocal8Bit(version)));
        if (!possibleQMake.isEmpty())
            foundQMakes << possibleQMake;
    }
    return Utils::toList(foundQMakes);
}

static void findSystemQt()
{
    FilePaths systemQMakes
            = BuildableHelperLibrary::findQtsInEnvironment(Environment::systemEnvironment());
    systemQMakes.append(gatherQmakePathsFromQtChooser());
    for (const FilePath &qmakePath : qAsConst(systemQMakes)) {
        if (BuildableHelperLibrary::isQtChooser(qmakePath.toFileInfo()))
            continue;
        const auto isSameQmake = [qmakePath](const BaseQtVersion *version) {
            return Environment::systemEnvironment().
                    isSameExecutable(qmakePath.toString(), version->qmakeCommand().toString());
        };
        if (contains(m_versions, isSameQmake))
            continue;
        BaseQtVersion *version = QtVersionFactory::createQtVersionFromQMakePath(qmakePath,
                                                                                false,
                                                                                "PATH");
        if (version)
            m_versions.insert(version->uniqueId(), version);
    }
}

void QtVersionManager::addVersion(BaseQtVersion *version)
{
    QTC_ASSERT(m_writer, return);
    QTC_ASSERT(version, return);
    if (m_versions.contains(version->uniqueId()))
        return;

    int uniqueId = version->uniqueId();
    m_versions.insert(uniqueId, version);

    emit m_instance->qtVersionsChanged(QList<int>() << uniqueId, QList<int>(), QList<int>());
    saveQtVersions();
}

void QtVersionManager::removeVersion(BaseQtVersion *version)
{
    QTC_ASSERT(version, return);
    m_versions.remove(version->uniqueId());
    emit m_instance->qtVersionsChanged(QList<int>(), QList<int>() << version->uniqueId(), QList<int>());
    saveQtVersions();
    delete version;
}

void QtVersionManager::registerExampleSet(const QString &displayName,
                                          const QString &manifestPath,
                                          const QString &examplesPath)
{
    m_pluginRegisteredExampleSets.append({displayName, manifestPath, examplesPath});
}

using Path = QString;
using FileName = QString;
static QList<std::pair<Path, FileName>> documentationFiles(BaseQtVersion *v)
{
    QList<std::pair<Path, FileName>> files;
    const QStringList docPaths = QStringList(
        {v->docsPath().toString() + QChar('/'), v->docsPath().toString() + "/qch/"});
    for (const QString &docPath : docPaths) {
        const QDir versionHelpDir(docPath);
        for (const QString &helpFile : versionHelpDir.entryList(QStringList("*.qch"), QDir::Files))
            files.append({docPath, helpFile});
    }
    return files;
}

static QStringList documentationFiles(const QList<BaseQtVersion *> &vs, bool highestOnly = false)
{
    QSet<QString> includedFileNames;
    QSet<QString> filePaths;
    const QList<BaseQtVersion *> versions = highestOnly ? QtVersionManager::sortVersions(vs) : vs;
    for (BaseQtVersion *v : versions) {
        for (const std::pair<Path, FileName> &file : documentationFiles(v)) {
            if (!highestOnly || !includedFileNames.contains(file.second)) {
                filePaths.insert(file.first + file.second);
                includedFileNames.insert(file.second);
            }
        }
    }
    return filePaths.values();
}

void QtVersionManager::updateDocumentation(const QList<BaseQtVersion *> &added,
                                           const QList<BaseQtVersion *> &removed,
                                           const QList<BaseQtVersion *> &allNew)
{
    const DocumentationSetting setting = documentationSetting();
    const QStringList docsOfAll = setting == DocumentationSetting::None
                                      ? QStringList()
                                      : documentationFiles(allNew,
                                                           setting
                                                               == DocumentationSetting::HighestOnly);
    const QStringList docsToRemove = Utils::filtered(documentationFiles(removed),
                                                     [&docsOfAll](const QString &f) {
                                                         return !docsOfAll.contains(f);
                                                     });
    const QStringList docsToAdd = Utils::filtered(documentationFiles(added),
                                                  [&docsOfAll](const QString &f) {
                                                      return docsOfAll.contains(f);
                                                  });
    Core::HelpManager::unregisterDocumentation(docsToRemove);
    Core::HelpManager::registerDocumentation(docsToAdd);
}

int QtVersionManager::getUniqueId()
{
    return m_idcount++;
}

QList<BaseQtVersion *> QtVersionManager::versions(const BaseQtVersion::Predicate &predicate)
{
    QList<BaseQtVersion *> versions;
    QTC_ASSERT(isLoaded(), return versions);
    if (predicate)
        return Utils::filtered(m_versions.values(), predicate);
    return m_versions.values();
}

QList<BaseQtVersion *> QtVersionManager::sortVersions(const QList<BaseQtVersion *> &input)
{
    QList<BaseQtVersion *> result = input;
    Utils::sort(result, qtVersionNumberCompare);
    return result;
}

BaseQtVersion *QtVersionManager::version(int id)
{
    QTC_ASSERT(isLoaded(), return nullptr);
    VersionMap::const_iterator it = m_versions.constFind(id);
    if (it == m_versions.constEnd())
        return nullptr;
    return it.value();
}

BaseQtVersion *QtVersionManager::version(const BaseQtVersion::Predicate &predicate)
{
    return Utils::findOrDefault(m_versions.values(), predicate);
}

// This function is really simplistic...
static bool equals(BaseQtVersion *a, BaseQtVersion *b)
{
    return a->equals(b);
}

void QtVersionManager::setNewQtVersions(const QList<BaseQtVersion *> &newVersions)
{
    // We want to preserve the same order as in the settings dialog
    // so we sort a copy
    QList<BaseQtVersion *> sortedNewVersions = newVersions;
    Utils::sort(sortedNewVersions, &BaseQtVersion::uniqueId);

    QList<BaseQtVersion *> addedVersions;
    QList<BaseQtVersion *> removedVersions;
    QList<std::pair<BaseQtVersion *, BaseQtVersion *>> changedVersions;
    // So we trying to find the minimal set of changed versions,
    // iterate over both sorted list

    // newVersions and oldVersions iterator
    QList<BaseQtVersion *>::const_iterator nit, nend;
    VersionMap::const_iterator oit, oend;
    nit = sortedNewVersions.constBegin();
    nend = sortedNewVersions.constEnd();
    oit = m_versions.constBegin();
    oend = m_versions.constEnd();

    while (nit != nend && oit != oend) {
        int nid = (*nit)->uniqueId();
        int oid = (*oit)->uniqueId();
        if (nid < oid) {
            addedVersions.push_back(*nit);
            ++nit;
        } else if (oid < nid) {
            removedVersions.push_back(*oit);
            ++oit;
        } else {
            if (!equals(*oit, *nit))
                changedVersions.push_back({*oit, *nit});
            ++oit;
            ++nit;
        }
    }

    while (nit != nend) {
        addedVersions.push_back(*nit);
        ++nit;
    }

    while (oit != oend) {
        removedVersions.push_back(*oit);
        ++oit;
    }

    if (!changedVersions.isEmpty() || !addedVersions.isEmpty() || !removedVersions.isEmpty()) {
        const QList<BaseQtVersion *> changedOldVersions
            = Utils::transform(changedVersions, &std::pair<BaseQtVersion *, BaseQtVersion *>::first);
        const QList<BaseQtVersion *> changedNewVersions
            = Utils::transform(changedVersions,
                               &std::pair<BaseQtVersion *, BaseQtVersion *>::second);
        updateDocumentation(addedVersions + changedNewVersions,
                            removedVersions + changedOldVersions,
                            sortedNewVersions);
    }
    const QList<int> addedIds = Utils::transform(addedVersions, &BaseQtVersion::uniqueId);
    const QList<int> removedIds = Utils::transform(removedVersions, &BaseQtVersion::uniqueId);
    const QList<int> changedIds = Utils::transform(changedVersions,
                                                   [](std::pair<BaseQtVersion *, BaseQtVersion *> v) {
                                                       return v.first->uniqueId();
                                                   });

    qDeleteAll(m_versions);
    m_versions = Utils::transform<VersionMap>(sortedNewVersions, [](BaseQtVersion *v) {
        return std::make_pair(v->uniqueId(), v);
    });
    saveQtVersions();

    if (!changedVersions.isEmpty() || !addedVersions.isEmpty() || !removedVersions.isEmpty())
        emit m_instance->qtVersionsChanged(addedIds, removedIds, changedIds);
}

void QtVersionManager::setDocumentationSetting(const QtVersionManager::DocumentationSetting &setting)
{
    if (setting == documentationSetting())
        return;
    Core::ICore::settings()->setValue(DOCUMENTATION_SETTING_KEY, int(setting));
    // force re-evaluating which documentation should be registered
    // by claiming that all are removed and re-added
    const QList<BaseQtVersion *> vs = versions();
    updateDocumentation(vs, vs, vs);
}

QtVersionManager::DocumentationSetting QtVersionManager::documentationSetting()
{
    return DocumentationSetting(
        Core::ICore::settings()->value(DOCUMENTATION_SETTING_KEY, 0).toInt());
}

} // namespace QtVersion
