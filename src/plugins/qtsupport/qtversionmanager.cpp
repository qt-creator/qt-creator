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

#include "qtversionmanager.h"

#include "qtkitinformation.h"
#include "qtversionfactory.h"

#include "qtsupportconstants.h"

// only for legay restore
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/gcctoolchain.h>

#include <qtsupport/debugginghelper.h>

#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/filesystemwatcher.h>
#include <utils/persistentsettings.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#ifdef Q_OS_WIN
#    include <utils/winutils.h>
#endif

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QTextStream>
#include <QTimer>

#include <algorithm>

using namespace QtSupport;
using namespace QtSupport::Internal;

static const char QTVERSION_DATA_KEY[] = "QtVersion.";
static const char QTVERSION_TYPE_KEY[] = "QtVersion.Type";
static const char QTVERSION_FILE_VERSION_KEY[] = "Version";
static const char QTVERSION_FILENAME[] = "/qtcreator/qtversion.xml";
static const char QTVERSION_LEGACY_FILENAME[] = "/qtversion.xml"; // TODO: pre 2.6, remove later

// legacy settings
static const char QtVersionsSectionName[] = "QtVersions";

enum { debug = 0 };

template<class T>
static T *createToolChain(const QString &id)
{
    QList<ProjectExplorer::ToolChainFactory *> factories =
            ExtensionSystem::PluginManager::getObjects<ProjectExplorer::ToolChainFactory>();
    foreach (ProjectExplorer::ToolChainFactory *f, factories) {
       if (f->id() == id) {
           Q_ASSERT(f->canCreate());
           return static_cast<T *>(f->create());
       }
    }
    return 0;
}

static Utils::FileName globalSettingsFileName()
{
    QSettings *globalSettings = ExtensionSystem::PluginManager::globalSettings();
    return Utils::FileName::fromString(QFileInfo(globalSettings->fileName()).absolutePath()
                                       + QLatin1String(QTVERSION_FILENAME));
}

static Utils::FileName settingsFileName(const QString &path)
{
    QSettings *settings = ExtensionSystem::PluginManager::settings();
    QFileInfo settingsLocation(settings->fileName());
    return Utils::FileName::fromString(settingsLocation.absolutePath() + path);
}


// prefer newer qts otherwise compare on id
bool qtVersionNumberCompare(BaseQtVersion *a, BaseQtVersion *b)
{
    return a->qtVersion() > b->qtVersion() || (a->qtVersion() == b->qtVersion() && a->uniqueId() < b->uniqueId());
}

// --------------------------------------------------------------------------
// QtVersionManager
// --------------------------------------------------------------------------
QtVersionManager *QtVersionManager::m_self = 0;

QtVersionManager::QtVersionManager() :
    m_configFileWatcher(0),
    m_fileWatcherTimer(new QTimer(this)),
    m_writer(0)
{
    m_self = this;
    m_idcount = 1;

    qRegisterMetaType<Utils::FileName>();

    // Give the file a bit of time to settle before reading it...
    m_fileWatcherTimer->setInterval(2000);
    connect(m_fileWatcherTimer, SIGNAL(timeout()), SLOT(updateFromInstaller()));
}

void QtVersionManager::extensionsInitialized()
{
    bool success = restoreQtVersions();
    updateFromInstaller(false);
    if (!success) {
        // We did neither restore our settings or upgraded
        // in that case figure out if there's a qt in path
        // and add it to the Qt versions
        findSystemQt();
    }

    emit qtVersionsChanged(m_versions.keys(), QList<int>(), QList<int>());
    saveQtVersions();

    const Utils::FileName configFileName = globalSettingsFileName();
    if (configFileName.toFileInfo().exists()) {
        m_configFileWatcher = new Utils::FileSystemWatcher(this);
        connect(m_configFileWatcher, SIGNAL(fileChanged(QString)),
                m_fileWatcherTimer, SLOT(start()));
        m_configFileWatcher->addFile(configFileName.toString(),
                                     Utils::FileSystemWatcher::WatchModifiedDate);
    } // exists
}

bool QtVersionManager::delayedInitialize()
{
    updateDocumentation();
    return true;
}

QtVersionManager::~QtVersionManager()
{
    delete m_writer;
    qDeleteAll(m_versions);
    m_versions.clear();
}

QtVersionManager *QtVersionManager::instance()
{
    return m_self;
}

bool QtVersionManager::restoreQtVersions()
{
    QTC_ASSERT(!m_writer, return false);
    m_writer = new Utils::PersistentSettingsWriter(settingsFileName(QLatin1String(QTVERSION_FILENAME)),
                                                   QLatin1String("QtCreatorQtVersions"));

    QList<QtVersionFactory *> factories = ExtensionSystem::PluginManager::getObjects<QtVersionFactory>();

    Utils::PersistentSettingsReader reader;
    Utils::FileName filename = settingsFileName(QLatin1String(QTVERSION_FILENAME));

    // Read Qt Creator 2.5 qtversions.xml once:
    if (!filename.toFileInfo().exists())
        filename = settingsFileName(QLatin1String(QTVERSION_LEGACY_FILENAME));
    if (!reader.load(filename))
        return false;
    QVariantMap data = reader.restoreValues();

    // Check version:
    int version = data.value(QLatin1String(QTVERSION_FILE_VERSION_KEY), 0).toInt();
    if (version < 1)
        return false;

    const QString keyPrefix = QLatin1String(QTVERSION_DATA_KEY);
    foreach (const QString &key, data.keys()) {
        if (!key.startsWith(keyPrefix))
            continue;
        bool ok;
        int count = key.mid(keyPrefix.count()).toInt(&ok);
        if (!ok || count < 0)
            continue;

        const QVariantMap qtversionMap = data.value(key).toMap();
        const QString type = qtversionMap.value(QLatin1String(QTVERSION_TYPE_KEY)).toString();

        bool restored = false;
        foreach (QtVersionFactory *f, factories) {
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

    const Utils::FileName path = globalSettingsFileName();
    // Handle overwritting of data:
    if (m_configFileWatcher) {
        m_configFileWatcher->removeFile(path.toString());
        m_configFileWatcher->addFile(path.toString(), Utils::FileSystemWatcher::WatchModifiedDate);
    }

    QList<int> added;
    QList<int> removed;
    QList<int> changed;

    QList<QtVersionFactory *> factories = ExtensionSystem::PluginManager::getObjects<QtVersionFactory>();
    Utils::PersistentSettingsReader reader;
    QVariantMap data;
    if (reader.load(path))
        data = reader.restoreValues();

    if (debug) {
        qDebug()<< "======= Existing Qt versions =======";
        foreach (BaseQtVersion *version, m_versions) {
            qDebug() << version->qmakeCommand().toString() << "id:"<<version->uniqueId();
            qDebug() << "  autodetection source:"<< version->autodetectionSource();
            qDebug() << "";
        }
        qDebug()<< "======= Adding sdk versions =======";
    }

    QStringList sdkVersions;

    const QString keyPrefix = QLatin1String(QTVERSION_DATA_KEY);
    foreach (const QString &key, data.keys()) {
        if (!key.startsWith(keyPrefix))
            continue;
        bool ok;
        int count = key.mid(keyPrefix.count()).toInt(&ok);
        if (!ok || count < 0)
            continue;

        QVariantMap qtversionMap = data.value(key).toMap();
        const QString type = qtversionMap.value(QLatin1String(QTVERSION_TYPE_KEY)).toString();
        const QString autoDetectionSource = qtversionMap.value(QLatin1String("autodetectionSource")).toString();
        sdkVersions << autoDetectionSource;
        int id = -1; // see BaseQtVersion::fromMap()
        QtVersionFactory *factory = 0;
        foreach (QtVersionFactory *f, factories) {
            if (f->canRestore(type))
                factory = f;
        }
        if (!factory) {
            if (debug)
                qDebug("Warning: Unable to find factory for type '%s'", qPrintable(type));
            continue;
        }
        // First try to find a existing Qt version to update
        bool restored = false;
        foreach (BaseQtVersion *v, m_versions) {
            if (v->autodetectionSource() == autoDetectionSource) {
                id = v->uniqueId();
                if (debug)
                    qDebug() << " Qt version found with same autodetection source" << autoDetectionSource << " => Migrating id:" << id;
                m_versions.remove(id);
                qtversionMap[QLatin1String("Id")] = id;

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
            if (debug)
                qDebug() << " No Qt version found matching" << autoDetectionSource << " => Creating new version";
            if (BaseQtVersion *qtv = factory->restore(type, qtversionMap)) {
                Q_ASSERT(qtv->isAutodetected());
                m_versions.insert(qtv->uniqueId(), qtv);
                added << qtv->uniqueId();
                restored = true;
            }
        }
        if (!restored)
            if (debug)
                qDebug("Warning: Unable to update qtversion '%s' from sdk installer.",
                       qPrintable(autoDetectionSource));
    }

    if (debug) {
        qDebug() << "======= Before removing outdated sdk versions =======";
        foreach (BaseQtVersion *version, m_versions) {
            qDebug() << version->qmakeCommand().toString() << "id:"<<version->uniqueId();
            qDebug() << "  autodetection source:"<< version->autodetectionSource();
            qDebug() << "";
        }
    }
    foreach (BaseQtVersion *qtVersion, QtVersionManager::instance()->versions()) {
        if (qtVersion->autodetectionSource().startsWith(QLatin1String("SDK."))) {
            if (!sdkVersions.contains(qtVersion->autodetectionSource())) {
                if (debug)
                    qDebug() << "  removing version"<<qtVersion->autodetectionSource();
                m_versions.remove(qtVersion->uniqueId());
                removed << qtVersion->uniqueId();
            }
        }
    }

    if (debug) {
        qDebug()<< "======= End result =======";
        foreach (BaseQtVersion *version, m_versions) {
            qDebug() << version->qmakeCommand().toString() << "id:"<<version->uniqueId();
            qDebug() << "  autodetection source:"<< version->autodetectionSource();
            qDebug() << "";
        }
    }
    if (emitSignal)
        emit qtVersionsChanged(added, removed, changed);
    saveQtVersions();
}

void QtVersionManager::saveQtVersions()
{
    if (!m_writer)
        return;

    QVariantMap data;
    data.insert(QLatin1String(QTVERSION_FILE_VERSION_KEY), 1);

    int count = 0;
    foreach (BaseQtVersion *qtv, m_versions) {
        QVariantMap tmp = qtv->toMap();
        if (tmp.isEmpty())
            continue;
        tmp.insert(QLatin1String(QTVERSION_TYPE_KEY), qtv->type());
        data.insert(QString::fromLatin1(QTVERSION_DATA_KEY) + QString::number(count), tmp);
        ++count;

    }
    m_writer->save(data, Core::ICore::mainWindow());
}

void QtVersionManager::findSystemQt()
{
    Utils::FileName systemQMakePath = QtSupport::DebuggingHelperLibrary::findSystemQt(Utils::Environment::systemEnvironment());
    if (systemQMakePath.isNull())
        return;

    BaseQtVersion *version = QtVersionFactory::createQtVersionFromQMakePath(systemQMakePath);
    if (version) {
        version->setDisplayName(BaseQtVersion::defaultDisplayName(version->qtVersionString(), systemQMakePath, true));
        m_versions.insert(version->uniqueId(), version);
    }
}

void QtVersionManager::addVersion(BaseQtVersion *version)
{
    QTC_ASSERT(version != 0, return);
    if (m_versions.contains(version->uniqueId()))
        return;

    int uniqueId = version->uniqueId();
    m_versions.insert(uniqueId, version);

    emit qtVersionsChanged(QList<int>() << uniqueId, QList<int>(), QList<int>());
    saveQtVersions();
}

void QtVersionManager::removeVersion(BaseQtVersion *version)
{
    QTC_ASSERT(version != 0, return);
    m_versions.remove(version->uniqueId());
    emit qtVersionsChanged(QList<int>(), QList<int>() << version->uniqueId(), QList<int>());
    saveQtVersions();
    delete version;
}

void QtVersionManager::updateDocumentation()
{
    Core::HelpManager *helpManager = Core::HelpManager::instance();
    Q_ASSERT(helpManager);
    QStringList files;
    foreach (BaseQtVersion *v, m_versions) {
        const QStringList docPaths = QStringList() << v->documentationPath() + QLatin1Char('/')
                                                   << v->documentationPath() + QLatin1String("/qch/");
        foreach (const QString &docPath, docPaths) {
            const QDir versionHelpDir(docPath);
            foreach (const QString &helpFile,
                     versionHelpDir.entryList(QStringList() << QLatin1String("*.qch"), QDir::Files))
                files << docPath + helpFile;
        }
    }
    helpManager->registerDocumentation(files);
}

void QtVersionManager::updateDumpFor(const Utils::FileName &qmakeCommand)
{
    foreach (BaseQtVersion *v, versions()) {
        if (v->qmakeCommand() == qmakeCommand)
            v->recheckDumper();
    }
    emit dumpUpdatedFor(qmakeCommand);
}

int QtVersionManager::getUniqueId()
{
    return m_idcount++;
}

QList<BaseQtVersion *> QtVersionManager::versions() const
{
    QList<BaseQtVersion *> versions;
    foreach (BaseQtVersion *version, m_versions)
        versions << version;
    qSort(versions.begin(), versions.end(), &qtVersionNumberCompare);
    return versions;
}

QList<BaseQtVersion *> QtVersionManager::validVersions() const
{
    QList<BaseQtVersion *> results;
    foreach (BaseQtVersion *v, m_versions) {
        if (v->isValid())
            results.append(v);
    }
    qSort(results.begin(), results.end(), &qtVersionNumberCompare);
    return results;
}

bool QtVersionManager::isValidId(int id) const
{
    return m_versions.contains(id);
}

Core::FeatureSet QtVersionManager::availableFeatures(const QString &platformName) const
{
    Core::FeatureSet features;
    foreach (BaseQtVersion *const qtVersion, validVersions()) {
        if (qtVersion->isValid() && ((qtVersion->platformName() == platformName) || platformName.isEmpty()))
            features |= qtVersion->availableFeatures();
    }

    return features;
}

QStringList QtVersionManager::availablePlatforms() const
{
    QStringList platforms;
    foreach (BaseQtVersion *const qtVersion, validVersions()) {
        if (qtVersion->isValid() && !qtVersion->platformName().isEmpty())
            platforms.append(qtVersion->platformName());
    }
    platforms.removeDuplicates();
    return platforms;
}

QString QtVersionManager::displayNameForPlatform(const QString &string) const
{
    foreach (BaseQtVersion *const qtVersion, validVersions()) {
        if (qtVersion->platformName() == string)
            return qtVersion->platformDisplayName();
    }
    return QString();
}

BaseQtVersion *QtVersionManager::version(int id) const
{
    QMap<int, BaseQtVersion *>::const_iterator it = m_versions.find(id);
    if (it == m_versions.constEnd())
        return 0;
    return it.value();
}

class SortByUniqueId
{
public:
    bool operator()(BaseQtVersion *a, BaseQtVersion *b)
    {
        return a->uniqueId() < b->uniqueId();
    }
};

bool QtVersionManager::equals(BaseQtVersion *a, BaseQtVersion *b)
{
    return a->equals(b);
}

void QtVersionManager::setNewQtVersions(QList<BaseQtVersion *> newVersions)
{
    // We want to preserve the same order as in the settings dialog
    // so we sort a copy
    QList<BaseQtVersion *> sortedNewVersions = newVersions;
    SortByUniqueId sortByUniqueId;
    qSort(sortedNewVersions.begin(), sortedNewVersions.end(), sortByUniqueId);

    QList<int> addedVersions;
    QList<int> removedVersions;
    QList<int> changedVersions;
    // So we trying to find the minimal set of changed versions,
    // iterate over both sorted list

    // newVersions and oldVersions iterator
    QList<BaseQtVersion *>::const_iterator nit, nend;
    QMap<int, BaseQtVersion *>::const_iterator oit, oend;
    nit = sortedNewVersions.constBegin();
    nend = sortedNewVersions.constEnd();
    oit = m_versions.constBegin();
    oend = m_versions.constEnd();

    while (nit != nend && oit != oend) {
        int nid = (*nit)->uniqueId();
        int oid = (*oit)->uniqueId();
        if (nid < oid) {
            addedVersions.push_back(nid);
            ++nit;
        } else if (oid < nid) {
            removedVersions.push_back(oid);
            ++oit;
        } else {
            if (!equals(*oit, *nit))
                changedVersions.push_back(oid);
            ++oit;
            ++nit;
        }
    }

    while (nit != nend) {
        addedVersions.push_back((*nit)->uniqueId());
        ++nit;
    }

    while (oit != oend) {
        removedVersions.push_back((*oit)->uniqueId());
        ++oit;
    }

    qDeleteAll(m_versions);
    m_versions.clear();
    foreach (BaseQtVersion *v, sortedNewVersions)
        m_versions.insert(v->uniqueId(), v);

    if (!changedVersions.isEmpty() || !addedVersions.isEmpty() || !removedVersions.isEmpty())
        updateDocumentation();

    saveQtVersions();

    if (!changedVersions.isEmpty() || !addedVersions.isEmpty() || !removedVersions.isEmpty())
        emit qtVersionsChanged(addedVersions, removedVersions, changedVersions);
}

// Returns the version that was used to build the project in that directory
// That is returns the directory
// To find out whether we already have a qtversion for that directory call
// QtVersion *QtVersionManager::qtVersionForDirectory(const QString directory);
Utils::FileName QtVersionManager::findQMakeBinaryFromMakefile(const QString &makefile)
{
    bool debugAdding = false;
    QFile fi(makefile);
    if (fi.exists() && fi.open(QFile::ReadOnly)) {
        QTextStream ts(&fi);
        QRegExp r1(QLatin1String("QMAKE\\s*=(.*)"));
        while (!ts.atEnd()) {
            QString line = ts.readLine();
            if (r1.exactMatch(line)) {
                if (debugAdding)
                    qDebug()<<"#~~ QMAKE is:"<<r1.cap(1).trimmed();
                QFileInfo qmake(r1.cap(1).trimmed());
                QString qmakePath = qmake.filePath();
#ifdef Q_OS_WIN
                if (!qmakePath.endsWith(QLatin1String(".exe")))
                    qmakePath.append(QLatin1String(".exe"));
#endif
                // Is qmake still installed?
                QFileInfo fi(qmakePath);
                if (fi.exists())
                    return Utils::FileName(fi);
            }
        }
    }
    return Utils::FileName();
}

BaseQtVersion *QtVersionManager::qtVersionForQMakeBinary(const Utils::FileName &qmakePath)
{
    foreach (BaseQtVersion *version, versions()) {
        if (version->qmakeCommand() == qmakePath) {
            return version;
            break;
        }
    }
    return 0;
}

void dumpQMakeAssignments(const QList<QMakeAssignment> &list)
{
    foreach (const QMakeAssignment &qa, list) {
        qDebug()<<qa.variable<<qa.op<<qa.value;
    }
}

QtVersionManager::MakefileCompatible QtVersionManager::makefileIsFor(const QString &makefile, const QString &proFile)
{
    if (proFile.isEmpty())
        return CouldNotParse;

    // The Makefile.Debug / Makefile.Release lack a # Command: line
    if (findQMakeLine(makefile, QLatin1String("# Command:")).trimmed().isEmpty())
        return CouldNotParse;

    QString line = findQMakeLine(makefile, QLatin1String("# Project:")).trimmed();
    if (line.isEmpty())
        return CouldNotParse;

    line.remove(0, line.indexOf(QLatin1Char(':')) + 1);
    line = line.trimmed();

    QFileInfo srcFileInfo(QFileInfo(makefile).absoluteDir(), line);
    QFileInfo proFileInfo(proFile);
    return (srcFileInfo == proFileInfo) ? SameProject : DifferentProject;
}

QPair<BaseQtVersion::QmakeBuildConfigs, QString> QtVersionManager::scanMakeFile(const QString &makefile, BaseQtVersion::QmakeBuildConfigs defaultBuildConfig)
{
    if (debug)
        qDebug()<<"ScanMakeFile, the gory details:";
    BaseQtVersion::QmakeBuildConfigs result = defaultBuildConfig;
    QString result2;

    QString line = findQMakeLine(makefile, QLatin1String("# Command:"));
    if (!line.isEmpty()) {
        if (debug)
            qDebug()<<"Found line"<<line;
        line = trimLine(line);
        QList<QMakeAssignment> assignments;
        QList<QMakeAssignment> afterAssignments;
        parseArgs(line, &assignments, &afterAssignments, &result2);

        if (debug) {
            dumpQMakeAssignments(assignments);
            if (!afterAssignments.isEmpty())
                qDebug()<<"-after";
            dumpQMakeAssignments(afterAssignments);
        }

        // Search in assignments for CONFIG(+=,-=,=)(debug,release,debug_and_release)
        // Also remove them from the list
        result = qmakeBuildConfigFromCmdArgs(&assignments, defaultBuildConfig);

        if (debug)
            dumpQMakeAssignments(assignments);

        foreach (const QMakeAssignment &qa, assignments)
            Utils::QtcProcess::addArg(&result2, qa.variable + qa.op + qa.value);
        if (!afterAssignments.isEmpty()) {
            Utils::QtcProcess::addArg(&result2, QLatin1String("-after"));
            foreach (const QMakeAssignment &qa, afterAssignments)
                Utils::QtcProcess::addArg(&result2, qa.variable + qa.op + qa.value);
        }
    }

    // Dump the gathered information:
    if (debug) {
        qDebug()<<"\n\nDumping information from scanMakeFile";
        qDebug()<<"QMake CONFIG variable parsing";
        qDebug()<<"  "<< (result & BaseQtVersion::NoBuild ? QByteArray("No Build") : QByteArray::number(int(result)));
        qDebug()<<"  "<< (result & BaseQtVersion::DebugBuild ? "debug" : "release");
        qDebug()<<"  "<< (result & BaseQtVersion::BuildAll ? "debug_and_release" : "no debug_and_release");
        qDebug()<<"\nAddtional Arguments";
        qDebug()<<result2;
        qDebug()<<"\n\n";
    }
    return qMakePair(result, result2);
}

QString QtVersionManager::findQMakeLine(const QString &makefile, const QString &key)
{
    QFile fi(makefile);
    if (fi.exists() && fi.open(QFile::ReadOnly)) {
        QTextStream ts(&fi);
        while (!ts.atEnd()) {
            const QString line = ts.readLine();
            if (line.startsWith(key))
                return line;
        }
    }
    return QString();
}

/// This function trims the "#Command /path/to/qmake" from the the line
QString QtVersionManager::trimLine(const QString line)
{

    // Actually the first space after #Command: /path/to/qmake
    const int firstSpace = line.indexOf(QLatin1Char(' '), 11);
    return line.mid(firstSpace).trimmed();
}

void QtVersionManager::parseArgs(const QString &args, QList<QMakeAssignment> *assignments, QList<QMakeAssignment> *afterAssignments, QString *additionalArguments)
{
    QRegExp regExp(QLatin1String("([^\\s\\+-]*)\\s*(\\+=|=|-=|~=)(.*)"));
    bool after = false;
    bool ignoreNext = false;
    *additionalArguments = args;
    Utils::QtcProcess::ArgIterator ait(additionalArguments);
    while (ait.next()) {
        if (ignoreNext) {
            // Ignoring
            ignoreNext = false;
            ait.deleteArg();
        } else if (ait.value() == QLatin1String("-after")) {
            after = true;
            ait.deleteArg();
        } else if (ait.value().contains(QLatin1Char('='))) {
            if (regExp.exactMatch(ait.value())) {
                QMakeAssignment qa;
                qa.variable = regExp.cap(1);
                qa.op = regExp.cap(2);
                qa.value = regExp.cap(3).trimmed();
                if (after)
                    afterAssignments->append(qa);
                else
                    assignments->append(qa);
            } else {
                qDebug()<<"regexp did not match";
            }
            ait.deleteArg();
        } else if (ait.value() == QLatin1String("-o")) {
            ignoreNext = true;
            ait.deleteArg();
#if defined(Q_OS_WIN32)
        } else if (ait.value() == QLatin1String("-win32")) {
#elif defined(Q_OS_MAC)
        } else if (ait.value() == QLatin1String("-macx")) {
#elif defined(Q_OS_QNX6)
        } else if (ait.value() == QLatin1String("-qnx6")) {
#else
        } else if (ait.value() == QLatin1String("-unix")) {
#endif
            ait.deleteArg();
        }
    }
    ait.deleteArg();  // The .pro file is always the last arg
}

/// This function extracts all the CONFIG+=debug, CONFIG+=release
BaseQtVersion::QmakeBuildConfigs QtVersionManager::qmakeBuildConfigFromCmdArgs(QList<QMakeAssignment> *assignments, BaseQtVersion::QmakeBuildConfigs defaultBuildConfig)
{
    BaseQtVersion::QmakeBuildConfigs result = defaultBuildConfig;
    QList<QMakeAssignment> oldAssignments = *assignments;
    assignments->clear();
    foreach (const QMakeAssignment &qa, oldAssignments) {
        if (qa.variable == QLatin1String("CONFIG")) {
            QStringList values = qa.value.split(QLatin1Char(' '));
            QStringList newValues;
            foreach (const QString &value, values) {
                if (value == QLatin1String("debug")) {
                    if (qa.op == QLatin1String("+="))
                        result = result  | BaseQtVersion::DebugBuild;
                    else
                        result = result  & ~BaseQtVersion::DebugBuild;
                } else if (value == QLatin1String("release")) {
                    if (qa.op == QLatin1String("+="))
                        result = result & ~BaseQtVersion::DebugBuild;
                    else
                        result = result | BaseQtVersion::DebugBuild;
                } else if (value == QLatin1String("debug_and_release")) {
                    if (qa.op == QLatin1String("+="))
                        result = result | BaseQtVersion::BuildAll;
                    else
                        result = result & ~BaseQtVersion::BuildAll;
                } else {
                    newValues.append(value);
                }
                QMakeAssignment newQA = qa;
                newQA.value = newValues.join(QLatin1String(" "));
                if (!newValues.isEmpty())
                    assignments->append(newQA);
            }
        } else {
            assignments->append(qa);
        }
    }
    return result;
}

Core::FeatureSet QtFeatureProvider::availableFeatures(const QString &platformName) const
{
     return QtVersionManager::instance()->availableFeatures(platformName);
}

QStringList QtFeatureProvider::availablePlatforms() const
{
    return QtVersionManager::instance()->availablePlatforms();
}

QString QtFeatureProvider::displayNameForPlatform(const QString &string) const
{
    return QtVersionManager::instance()->displayNameForPlatform(string);
}
