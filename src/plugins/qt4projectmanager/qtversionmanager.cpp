/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qtversionmanager.h"

#include "qt4projectmanagerconstants.h"
#include "qt4target.h"
#include "profilereader.h"

#include "qt-maemo/maemoglobal.h"
#include "qt-maemo/maemomanager.h"
#include "qt-s60/s60manager.h"
#include "qt-s60/abldparser.h"
#include "qt-s60/sbsv2parser.h"
#include "qt-s60/gccetoolchain.h"
#include "qt-s60/winscwtoolchain.h"
#include "qt4basetargetfactory.h"

#include "qmlobservertool.h"
#include "qmldumptool.h"
#include "qmldebugginglibrary.h"

#include <projectexplorer/debugginghelper.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/cesdkhandler.h>
#include <projectexplorer/gcctoolchain.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/ioutputparser.h>
#include <projectexplorer/task.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/synchronousprocess.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#ifdef Q_OS_WIN
#    include <utils/winutils.h>
#endif

#include <QtCore/QFile>
#include <QtCore/QProcess>
#include <QtCore/QSettings>
#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtCore/QTextStream>
#include <QtCore/QDir>
#include <QtGui/QApplication>
#include <QtGui/QDesktopServices>

#include <algorithm>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

using ProjectExplorer::DebuggingHelperLibrary;

static const char *QtVersionsSectionName = "QtVersions";
static const char *newQtVersionsKey = "NewQtVersions";
static const char *PATH_AUTODETECTION_SOURCE = "PATH";

enum { debug = 0 };

template<class T>
static T *createToolChain(const QString &id)
{
    QList<ProjectExplorer::ToolChainFactory *> factories =
            ExtensionSystem::PluginManager::instance()->getObjects<ProjectExplorer::ToolChainFactory>();
    foreach (ProjectExplorer::ToolChainFactory *f, factories) {
       if (f->id() == id) {
           Q_ASSERT(f->canCreate());
           return static_cast<T *>(f->create());
       }
    }
    return 0;
}


// prefer newer qts otherwise compare on id
bool qtVersionNumberCompare(QtVersion *a, QtVersion *b)
{
    return a->qtVersion() > b->qtVersion() || (a->qtVersion() == b->qtVersion() && a->uniqueId() < b->uniqueId());
}

// --------------------------------------------------------------------------
// QtVersionManager
// --------------------------------------------------------------------------
QtVersionManager *QtVersionManager::m_self = 0;

QtVersionManager::QtVersionManager()
    : m_emptyVersion(new QtVersion)
{
    m_self = this;
    QSettings *s = Core::ICore::instance()->settings();

    m_idcount = 1;
    int size = s->beginReadArray(QtVersionsSectionName);
    for (int i = 0; i < size; ++i) {
        s->setArrayIndex(i);
        // Find the right id
        // Either something saved or something generated
        // Note: This code assumes that either all ids are read from the settings
        // or generated on the fly.
        int id = s->value("Id", -1).toInt();
        if (id == -1)
            id = getUniqueId();
        else if (m_idcount < id)
            m_idcount = id + 1;
        bool isAutodetected;
        QString autodetectionSource;
        if (s->contains("isAutodetected")) {
            isAutodetected = s->value("isAutodetected", false).toBool();
            autodetectionSource = s->value("autodetectionSource", QString()).toString();
        } else {// compatibility
            isAutodetected = s->value("IsSystemVersion", false).toBool();
            if (isAutodetected)
                autodetectionSource = QLatin1String(PATH_AUTODETECTION_SOURCE);
        }
        QString qmakePath = s->value("QMakePath").toString();
        if (qmakePath.isEmpty()) {
            QString path = s->value("Path").toString();
            if (!path.isEmpty()) {
                foreach(const QString& command, ProjectExplorer::DebuggingHelperLibrary::possibleQMakeCommands())
                {
                    QFileInfo fi(path + "/bin/" + command);
                    if (fi.exists())
                    {
                        qmakePath = fi.filePath();
                        break;
                    }
                }
            }
        }
        QtVersion *version = new QtVersion(s->value("Name").toString(),
                                           qmakePath,
                                           id,
                                           isAutodetected,
                                           autodetectionSource);
        // Make sure we do not import non-native separators from old Qt Creator versions:
        version->setSystemRoot(QDir::fromNativeSeparators(s->value("S60SDKDirectory").toString()));
        version->setSbsV2Directory(QDir::fromNativeSeparators(s->value(QLatin1String("SBSv2Directory")).toString()));

        // Update from pre-2.2:
        const QString mingwDir = s->value(QLatin1String("MingwDirectory")).toString();
        if (!mingwDir.isEmpty()) {
            QFileInfo fi(mingwDir + QLatin1String("/bin/g++.exe"));
            if (fi.exists() && fi.isExecutable()) {
                ProjectExplorer::MingwToolChain *tc = createToolChain<ProjectExplorer::MingwToolChain>(ProjectExplorer::Constants::MINGW_TOOLCHAIN_ID);
                if (tc) {
                    tc->setCompilerPath(fi.absoluteFilePath());
                    tc->setDisplayName(tr("MinGW from %1").arg(version->displayName()));
                    // The debugger is set later in the autoDetect method of the MinGw tool chain factory
                    // as the default debuggers are not yet registered.
                    ProjectExplorer::ToolChainManager::instance()->registerToolChain(tc);
                }
            }
        }
        const QString mwcDir = s->value(QLatin1String("MwcDirectory")).toString();
        if (!mwcDir.isEmpty())
            m_pendingMwcUpdates.append(mwcDir);
        const QString gcceDir = s->value(QLatin1String("GcceDirectory")).toString();
        if (!gcceDir.isEmpty())
            m_pendingGcceUpdates.append(gcceDir);

        m_versions.insert(version->uniqueId(), version);
    }
    s->endArray();

    ++m_idcount;
    addNewVersionsFromInstaller();
    updateSystemVersion();

    // cannot call from ctor, needs to get connected extenernally first
    QTimer::singleShot(0, this, SLOT(updateSettings()));
}

QtVersionManager::~QtVersionManager()
{
    qDeleteAll(m_versions);
    m_versions.clear();
    delete m_emptyVersion;
    m_emptyVersion = 0;
}

QtVersionManager *QtVersionManager::instance()
{
    return m_self;
}

void QtVersionManager::addVersion(QtVersion *version)
{
    QTC_ASSERT(version != 0, return);
    if (m_versions.contains(version->uniqueId()))
        return;

    int uniqueId = version->uniqueId();
    m_versions.insert(uniqueId, version);

    emit qtVersionsChanged(QList<int>() << uniqueId);
    writeVersionsIntoSettings();
}

void QtVersionManager::removeVersion(QtVersion *version)
{
    QTC_ASSERT(version != 0, return);
    m_versions.remove(version->uniqueId());
    emit qtVersionsChanged(QList<int>() << version->uniqueId());
    writeVersionsIntoSettings();
    delete version;
}

bool QtVersionManager::supportsTargetId(const QString &id) const
{
    QList<QtVersion *> versions = QtVersionManager::instance()->versionsForTargetId(id);
    foreach (QtVersion *v, versions)
        if (v->isValid() && v->toolChainAvailable(id))
            return true;
    return false;
}

QList<QtVersion *> QtVersionManager::versionsForTargetId(const QString &id, const QtVersionNumber &minimumQtVersion) const
{
    QList<QtVersion *> targetVersions;
    foreach (QtVersion *version, m_versions) {
        if (version->supportsTargetId(id) && version->qtVersion() >= minimumQtVersion)
            targetVersions.append(version);
    }
    qSort(targetVersions.begin(), targetVersions.end(), &qtVersionNumberCompare);
    return targetVersions;
}

QSet<QString> QtVersionManager::supportedTargetIds() const
{
    QSet<QString> results;
    foreach (QtVersion *version, m_versions)
        results.unite(version->supportedTargetIds());
    return results;
}

void QtVersionManager::updateDocumentation()
{
    Core::HelpManager *helpManager = Core::HelpManager::instance();
    Q_ASSERT(helpManager);
    QStringList files;
    foreach (QtVersion *version, m_versions) {
        const QString docPath = version->documentationPath() + QLatin1String("/qch/");
        const QDir versionHelpDir(docPath);
        foreach (const QString &helpFile,
                versionHelpDir.entryList(QStringList() << QLatin1String("*.qch"), QDir::Files))
            files << docPath + helpFile;

    }
    helpManager->registerDocumentation(files);
}

void QtVersionManager::updateSettings()
{
    writeVersionsIntoSettings();

    updateDocumentation();

    QtVersion *version = 0;
    QList<QtVersion*> candidates;

    // try to find a version which has both, demos and examples
    foreach (version, m_versions) {
        if (version->hasExamples() && version->hasDemos())
        candidates.append(version);
    }

    // in SDKs, we want to prefer the Qt version shipping with the SDK
    QSettings *settings = Core::ICore::instance()->settings();
    QString preferred = settings->value(QLatin1String("PreferredQMakePath")).toString();
    preferred = QDir::fromNativeSeparators(preferred);
    if (!preferred.isEmpty()) {
#ifdef Q_OS_WIN
        preferred = preferred.toLower();
        if (!preferred.endsWith(QLatin1String(".exe")))
            preferred.append(QLatin1String(".exe"));
#endif
        foreach (version, candidates) {
            if (version->qmakeCommand() == preferred) {
                emit updateExamples(version->examplesPath(), version->demosPath(), version->sourcePath());
                return;
            }
        }
    }

    // prefer versions with declarative examples
    foreach (version, candidates) {
        if (QDir(version->examplesPath()+"/declarative").exists()) {
            emit updateExamples(version->examplesPath(), version->demosPath(), version->sourcePath());
            return;
        }
    }

    if (!candidates.isEmpty()) {
        version = candidates.first();
        emit updateExamples(version->examplesPath(), version->demosPath(), version->sourcePath());
        return;
    }
    return;

}

int QtVersionManager::getUniqueId()
{
    return m_idcount++;
}

void QtVersionManager::writeVersionsIntoSettings()
{
    QSettings *s = Core::ICore::instance()->settings();
    s->beginWriteArray(QtVersionsSectionName);
    QMap<int, QtVersion *>::const_iterator it = m_versions.constBegin();
    for (int i = 0; i < m_versions.size(); ++i) {
        const QtVersion *version = it.value();
        s->setArrayIndex(i);
        s->setValue("Name", version->displayName());
        // for downwards compat
        s->setValue("Path", version->versionInfo().value("QT_INSTALL_DATA"));
        s->setValue("QMakePath", version->qmakeCommand());
        s->setValue("Id", version->uniqueId());
        s->setValue("isAutodetected", version->isAutodetected());
        if (version->isAutodetected())
            s->setValue("autodetectionSource", version->autodetectionSource());
        s->setValue("S60SDKDirectory", version->systemRoot());
        s->setValue(QLatin1String("SBSv2Directory"), version->sbsV2Directory());
        // Remove obsolete settings: New tool chains would be created at each startup
        // otherwise, overriding manually set ones.
        s->remove(QLatin1String("MingwDirectory"));
        s->remove(QLatin1String("MwcDirectory"));
        s->remove(QLatin1String("GcceDirectory"));
        ++it;
    }
    s->endArray();
}

QList<QtVersion *> QtVersionManager::versions() const
{
    QList<QtVersion *> versions;
    foreach (QtVersion *version, m_versions)
        versions << version;
    qSort(versions.begin(), versions.end(), &qtVersionNumberCompare);
    return versions;
}

QList<QtVersion *> QtVersionManager::validVersions() const
{
    QList<QtVersion *> results;
    foreach(QtVersion *v, m_versions) {
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

QString QtVersionManager::popPendingMwcUpdate()
{
    if (m_pendingMwcUpdates.isEmpty())
        return QString();
    return m_pendingMwcUpdates.takeFirst();
}

QString QtVersionManager::popPendingGcceUpdate()
{
    if (m_pendingGcceUpdates.isEmpty())
        return QString();
    return m_pendingGcceUpdates.takeFirst();
}

QtVersion *QtVersionManager::version(int id) const
{
    QMap<int, QtVersion *>::const_iterator it = m_versions.find(id);
    if (it == m_versions.constEnd())
        return m_emptyVersion;
    return it.value();
}

// FIXME: Rework this!
void QtVersionManager::addNewVersionsFromInstaller()
{
    // Add new versions which may have been installed by the WB installer in the form:
    // NewQtVersions="qt 4.3.2=c:\\qt\\qt432\bin\qmake.exe;qt embedded=c:\\qtembedded;"
    // or NewQtVersions="qt 4.3.2=c:\\qt\\qt432bin\qmake.exe;
    // i.e.
    // NewQtVersions="versionname=pathtoversion=s60sdk;"
    // Duplicate entries are not added, the first new version is set as default.
    QSettings *settings = Core::ICore::instance()->settings();
    QSettings *globalSettings = Core::ICore::instance()->settings(QSettings::SystemScope);

    QDateTime lastUpdateFromGlobalSettings = globalSettings->value(
            QLatin1String("General/LastQtVersionUpdate")).toDateTime();

    const QFileInfo gsFi(globalSettings->fileName());
    if ( !lastUpdateFromGlobalSettings.isNull() &&
         (!gsFi.exists() || (gsFi.lastModified() > lastUpdateFromGlobalSettings)) )
        return;

    if (!globalSettings->contains(newQtVersionsKey) &&
        !globalSettings->contains(QLatin1String("Installer/")+newQtVersionsKey))
    {
        return;
    }

    QString newVersionsValue = settings->value(newQtVersionsKey).toString();
    if (newVersionsValue.isEmpty())
        newVersionsValue = settings->value(QLatin1String("Installer/")+newQtVersionsKey).toString();

    QStringList newVersionsList = newVersionsValue.split(';', QString::SkipEmptyParts);
    foreach (const QString &newVersion, newVersionsList) {
        QStringList newVersionData = newVersion.split('=');
        if (newVersionData.count() >= 2) {
            if (QFile::exists(newVersionData[1])) {
                QtVersion *version = new QtVersion(newVersionData[0], newVersionData[1], m_idcount++ );
                if (newVersionData.count() >= 3)
                    version->setSystemRoot(QDir::fromNativeSeparators(newVersionData[2]));
                if (newVersionData.count() >= 4)
                    version->setSbsV2Directory(QDir::fromNativeSeparators(newVersionData[3]));

                bool versionWasAlreadyInList = false;
                foreach(const QtVersion * const it, m_versions) {
                    if (QDir(version->qmakeCommand()).canonicalPath() == QDir(it->qmakeCommand()).canonicalPath()) {
                        versionWasAlreadyInList = true;
                        break;
                    }
                }

                if (!versionWasAlreadyInList) {
                    m_versions.insert(version->uniqueId(), version);
                } else {
                    // clean up
                    delete version;
                }
            }
        }
    }
    settings->setValue(QLatin1String("General/LastQtVersionUpdate"), QDateTime::currentDateTime());
}

void QtVersionManager::updateSystemVersion()
{
    bool haveSystemVersion = false;
    QString systemQMakePath = DebuggingHelperLibrary::findSystemQt(Utils::Environment::systemEnvironment());
    if (systemQMakePath.isNull())
        systemQMakePath = tr("<not found>");

    foreach (QtVersion *version, m_versions) {
        if (version->isAutodetected()
            && version->autodetectionSource() == PATH_AUTODETECTION_SOURCE) {
            version->setQMakeCommand(systemQMakePath);
            version->setDisplayName(tr("Qt in PATH"));
            haveSystemVersion = true;
        }
    }
    if (haveSystemVersion)
        return;
    QtVersion *version = new QtVersion(tr("Qt in PATH"),
                                       systemQMakePath,
                                       getUniqueId(),
                                       true,
                                       PATH_AUTODETECTION_SOURCE);
    m_versions.insert(version->uniqueId(), version);
}

QtVersion *QtVersionManager::emptyVersion() const
{
    return m_emptyVersion;
}

class SortByUniqueId
{
public:
    bool operator()(QtVersion *a, QtVersion *b)
    {
        return a->uniqueId() < b->uniqueId();
    }
};

bool QtVersionManager::equals(QtVersion *a, QtVersion *b)
{
    if (a->m_qmakeCommand != b->m_qmakeCommand)
        return false;
    if (a->m_id != b->m_id)
        return false;
    if (a->m_displayName != b->displayName())
        return false;
    if (a->m_sbsV2Directory != b->m_sbsV2Directory)
        return false;
    if (a->m_systemRoot != b->m_systemRoot)
        return false;
    return true;
}

void QtVersionManager::setNewQtVersions(QList<QtVersion *> newVersions)
{
    // We want to preserve the same order as in the settings dialog
    // so we sort a copy
    QList<QtVersion *> sortedNewVersions = newVersions;
    SortByUniqueId sortByUniqueId;
    qSort(sortedNewVersions.begin(), sortedNewVersions.end(), sortByUniqueId);

    QList<int> changedVersions;
    // So we trying to find the minimal set of changed versions,
    // iterate over both sorted list

    // newVersions and oldVersions iterator
    QList<QtVersion *>::const_iterator nit, nend;
    QMap<int, QtVersion *>::const_iterator oit, oend;
    nit = sortedNewVersions.constBegin();
    nend = sortedNewVersions.constEnd();
    oit = m_versions.constBegin();
    oend = m_versions.constEnd();

    while (nit != nend && oit != oend) {
        int nid = (*nit)->uniqueId();
        int oid = (*oit)->uniqueId();
        if (nid < oid) {
            changedVersions.push_back(nid);
            ++nit;
        } else if (oid < nid) {
            changedVersions.push_back(oid);
            ++oit;
        } else {
            if (!equals(*oit, *nit))
                changedVersions.push_back(oid);
            ++oit;
            ++nit;
        }
    }

    while (nit != nend) {
        changedVersions.push_back((*nit)->uniqueId());
        ++nit;
    }

    while (oit != oend) {
        changedVersions.push_back((*oit)->uniqueId());
        ++oit;
    }

    qDeleteAll(m_versions);
    m_versions.clear();
    foreach (QtVersion *v, sortedNewVersions)
        m_versions.insert(v->uniqueId(), v);

    if (!changedVersions.isEmpty())
        updateDocumentation();

    updateSettings();
    writeVersionsIntoSettings();

    if (!changedVersions.isEmpty())
        emit qtVersionsChanged(changedVersions);
}

// --------------------------------------------------------------------------
// QtVersion
// --------------------------------------------------------------------------

QtVersion::QtVersion(const QString &name, const QString &qmakeCommand, int id,
                     bool isAutodetected, const QString &autodetectionSource)
    : m_displayName(name),
    m_isAutodetected(isAutodetected),
    m_autodetectionSource(autodetectionSource),
    m_hasDebuggingHelper(false),
    m_hasQmlDump(false),
    m_hasQmlDebuggingLibrary(false),
    m_hasQmlObserver(false),
    m_abiUpToDate(false),
    m_versionInfoUpToDate(false),
    m_notInstalled(false),
    m_defaultConfigIsDebug(true),
    m_defaultConfigIsDebugAndRelease(true),
    m_hasExamples(false),
    m_hasDemos(false),
    m_hasDocumentation(false),
    m_qmakeIsExecutable(false),
    m_validSystemRoot(true)
{
    if (id == -1)
        m_id = getUniqueId();
    else
        m_id = id;
    setQMakeCommand(qmakeCommand);
}

QtVersion::QtVersion(const QString &name, const QString &qmakeCommand,
                     bool isAutodetected, const QString &autodetectionSource)
    : m_displayName(name),
    m_isAutodetected(isAutodetected),
    m_autodetectionSource(autodetectionSource),
    m_hasDebuggingHelper(false),
    m_hasQmlDump(false),
    m_hasQmlDebuggingLibrary(false),
    m_hasQmlObserver(false),
    m_abiUpToDate(false),
    m_versionInfoUpToDate(false),
    m_notInstalled(false),
    m_defaultConfigIsDebug(true),
    m_defaultConfigIsDebugAndRelease(true),
    m_hasExamples(false),
    m_hasDemos(false),
    m_hasDocumentation(false),
    m_qmakeIsExecutable(false),
    m_validSystemRoot(true)
{
    m_id = getUniqueId();
    setQMakeCommand(qmakeCommand);
}


QtVersion::QtVersion(const QString &qmakeCommand, bool isAutodetected, const QString &autodetectionSource)
    : m_isAutodetected(isAutodetected),
    m_autodetectionSource(autodetectionSource),
    m_hasDebuggingHelper(false),
    m_hasQmlDump(false),
    m_hasQmlDebuggingLibrary(false),
    m_hasQmlObserver(false),
    m_abiUpToDate(false),
    m_versionInfoUpToDate(false),
    m_notInstalled(false),
    m_defaultConfigIsDebug(true),
    m_defaultConfigIsDebugAndRelease(true),
    m_hasExamples(false),
    m_hasDemos(false),
    m_hasDocumentation(false),
    m_qmakeIsExecutable(false),
    m_validSystemRoot(true)
{
    m_id = getUniqueId();
    setQMakeCommand(qmakeCommand);
    m_displayName = qtVersionString();
}

QtVersion::QtVersion()
    :  m_id(-1),
    m_isAutodetected(false),
    m_hasDebuggingHelper(false),
    m_hasQmlDump(false),
    m_hasQmlDebuggingLibrary(false),
    m_hasQmlObserver(false),
    m_abiUpToDate(false),
    m_versionInfoUpToDate(false),
    m_notInstalled(false),
    m_defaultConfigIsDebug(true),
    m_defaultConfigIsDebugAndRelease(true),
    m_hasExamples(false),
    m_hasDemos(false),
    m_hasDocumentation(false) ,
    m_qmakeIsExecutable(false),
    m_validSystemRoot(true)
{
    setQMakeCommand(QString());
}

QtVersion::~QtVersion()
{
}

QString QtVersion::toHtml(bool verbose) const
{
    QString rc;
    QTextStream str(&rc);
    str << "<html><body><table>";
    str << "<tr><td><b>" << QtVersionManager::tr("Name:")
        << "</b></td><td>" << displayName() << "</td></tr>";
    if (!isValid()) {
        str << "<tr><td colspan=2><b>" + QtVersionManager::tr("Invalid Qt version") +"</b></td></tr>";
    } else {
        QString prefix = QLatin1String("<tr><td><b>") + QtVersionManager::tr("ABI:") + QLatin1String("</b></td>");
        foreach (const ProjectExplorer::Abi &abi, qtAbis()) {
            str << prefix << "<td>" << abi.toString() << "</td></tr>";
            prefix = QLatin1String("<tr><td></td>");
        }
        str << "<tr><td><b>" << QtVersionManager::tr("Source:")
            << "</b></td><td>" << sourcePath() << "</td></tr>";
        str << "<tr><td><b>" << QtVersionManager::tr("mkspec:")
            << "</b></td><td>" << mkspec() << "</td></tr>";
        str << "<tr><td><b>" << QtVersionManager::tr("qmake:")
            << "</b></td><td>" << m_qmakeCommand << "</td></tr>";
        updateAbiAndMkspec();
        if (m_defaultConfigIsDebug || m_defaultConfigIsDebugAndRelease) {
            str << "<tr><td><b>" << QtVersionManager::tr("Default:") << "</b></td><td>"
                << (m_defaultConfigIsDebug ? "debug" : "release");
            if (m_defaultConfigIsDebugAndRelease)
                str << " debug_and_release";
            str << "</td></tr>";
        } // default config.
        str << "<tr><td><b>" << QtVersionManager::tr("Version:")
            << "</b></td><td>" << qtVersionString() << "</td></tr>";
        if (verbose) {
            const QHash<QString,QString> vInfo = versionInfo();
            if (!vInfo.isEmpty()) {
                const QHash<QString,QString>::const_iterator vcend = vInfo.constEnd();
                for (QHash<QString,QString>::const_iterator it = vInfo.constBegin(); it != vcend; ++it)
                    str << "<tr><td><pre>" << it.key() <<  "</pre></td><td>" << it.value() << "</td></tr>";
            }
        }
    }
    str << "</table></body></html>";
    return rc;
}

bool QtVersion::supportsShadowBuilds() const
{
    QSet<QString> targets = supportedTargetIds();
    // Symbian does not support shadow building
    if (targets.contains(Constants::S60_DEVICE_TARGET_ID) ||
        targets.contains(Constants::S60_EMULATOR_TARGET_ID)) {
        // We can not support shadow building with the ABLD system
        return false;
    }
#ifdef Q_OS_WIN
    if (targets.contains(Constants::MEEGO_DEVICE_TARGET_ID)
        || targets.contains(Constants::MAEMO5_DEVICE_TARGET_ID)
        || targets.contains(Constants::HARMATTAN_DEVICE_TARGET_ID))
        return false;
#endif
    return true;
}

ProjectExplorer::IOutputParser *QtVersion::createOutputParser() const
{
    if (supportsTargetId(Qt4ProjectManager::Constants::S60_DEVICE_TARGET_ID) ||
        supportsTargetId(Qt4ProjectManager::Constants::S60_EMULATOR_TARGET_ID)) {
        if (isBuildWithSymbianSbsV2()) {
            return new SbsV2Parser;
        } else {
            ProjectExplorer::IOutputParser *parser = new AbldParser;
            parser->appendOutputParser(new ProjectExplorer::GnuMakeParser);
            return parser;
        }
    }
    return new ProjectExplorer::GnuMakeParser;
}

QList<ProjectExplorer::Task>
QtVersion::reportIssues(const QString &proFile, const QString &buildDir, bool includeTargetSpecificErrors)
{
    QList<ProjectExplorer::Task> results;

    QString tmpBuildDir = QDir(buildDir).absolutePath();
    if (!tmpBuildDir.endsWith(QLatin1Char('/')))
        tmpBuildDir.append(QLatin1Char('/'));

    if (!isValid()) {
        //: %1: Reason for being invalid
        const QString msg = QCoreApplication::translate("Qt4ProjectManager::QtVersion", "The Qt version is invalid: %1").arg(invalidReason());
        results.append(ProjectExplorer::Task(ProjectExplorer::Task::Error, msg, QString(), -1,
                                             QLatin1String(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
    }

    QFileInfo qmakeInfo(qmakeCommand());
    if (!qmakeInfo.exists() ||
        !qmakeInfo.isExecutable()) {
        //: %1: Path to qmake executable
        const QString msg = QCoreApplication::translate("Qt4ProjectManager::QtVersion",
                                                        "The qmake command \"%1\" was not found or is not executable.").arg(qmakeCommand());
        results.append(ProjectExplorer::Task(ProjectExplorer::Task::Error, msg, QString(), -1,
                                             QLatin1String(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
    }

    QString sourcePath = QFileInfo(proFile).absolutePath();
    if (!sourcePath.endsWith(QLatin1Char('/')))
        sourcePath.append(QLatin1Char('/'));
    if ((tmpBuildDir.startsWith(sourcePath)) && (tmpBuildDir != sourcePath)) {
        const QString msg = QCoreApplication::translate("Qt4ProjectManager::QtVersion",
                                                        "Qmake does not support build directories below the source directory.");
        results.append(ProjectExplorer::Task(ProjectExplorer::Task::Warning, msg, QString(), -1,
                                             QLatin1String(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
    } else if (tmpBuildDir.count(QChar('/')) != sourcePath.count(QChar('/'))) {
        const QString msg = QCoreApplication::translate("Qt4ProjectManager::QtVersion",
                                                        "The build directory needs to be at the same level as the source directory.");

        results.append(ProjectExplorer::Task(ProjectExplorer::Task::Warning, msg, QString(), -1,
                                             QLatin1String(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
    }

#if defined (Q_OS_WIN)
    QSet<QString> targets = supportedTargetIds();
    if (targets.contains(Constants::S60_DEVICE_TARGET_ID) ||
            targets.contains(Constants::S60_EMULATOR_TARGET_ID)) {
        const QString epocRootDir = systemRoot();
        // Report an error if project- and epoc directory are on different drives:
        if (!epocRootDir.startsWith(proFile.left(3), Qt::CaseInsensitive) && !isBuildWithSymbianSbsV2()) {
            results.append(ProjectExplorer::Task(ProjectExplorer::Task::Error,
                                                 QCoreApplication::translate("ProjectExplorer::Internal::S60ProjectChecker",
                                                                             "The Symbian SDK and the project sources must reside on the same drive."),
                                                 QString(), -1, ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        }
    }
#endif

    if (includeTargetSpecificErrors) {
        QList<Qt4BaseTargetFactory *> factories;
        foreach (const QString &id, supportedTargetIds())
            if (Qt4BaseTargetFactory *factory = Qt4BaseTargetFactory::qt4BaseTargetFactoryForId(id))
                factories << factory;

        qSort(factories);
        QList<Qt4BaseTargetFactory *>::iterator newend = std::unique(factories.begin(), factories.end());
        QList<Qt4BaseTargetFactory *>::iterator it = factories.begin();
        for ( ; it != newend; ++it)
            results.append((*it)->reportIssues(proFile));
    }
    return results;
}

QString QtVersion::displayName() const
{
    return m_displayName;
}

QString QtVersion::qmakeCommand() const
{
    return m_qmakeCommand;
}

QString QtVersion::sourcePath() const
{
    return m_sourcePath;
}

QString QtVersion::mkspec() const
{
    updateAbiAndMkspec();
    return m_mkspec;
}

QString QtVersion::mkspecPath() const
{
    updateAbiAndMkspec();
    return m_mkspecFullPath;
}

bool QtVersion::isBuildWithSymbianSbsV2() const
{
    updateAbiAndMkspec();
    return m_isBuildUsingSbsV2;
}

QString QtVersion::qtVersionString() const
{
    if (m_qtVersionString.isNull()) {
        QFileInfo qmake(m_qmakeCommand);
        if (qmake.exists() && qmake.isExecutable()) {
            m_qtVersionString = DebuggingHelperLibrary::qtVersionForQMake(qmake.absoluteFilePath());
        } else {
            m_qtVersionString = QLatin1String("");
        }
    }
    return m_qtVersionString;
}

QtVersionNumber QtVersion::qtVersion() const
{
    //todo cache this;
    return QtVersionNumber(qtVersionString());
}

QHash<QString,QString> QtVersion::versionInfo() const
{
    updateVersionInfo();
    return m_versionInfo;
}

void QtVersion::setDisplayName(const QString &name)
{
    m_displayName = name;
}

void QtVersion::setQMakeCommand(const QString& qmakeCommand)
{
    m_qmakeCommand = QDir::fromNativeSeparators(qmakeCommand);
#ifdef Q_OS_WIN
    m_qmakeCommand = m_qmakeCommand.toLower();
#endif
    m_designerCommand.clear();
    m_linguistCommand.clear();
    m_qmlviewerCommand.clear();
    m_uicCommand.clear();
    m_abiUpToDate = false;
    // TODO do i need to optimize this?
    m_versionInfoUpToDate = false;
    m_qtVersionString.clear();
    updateSourcePath();
}

void QtVersion::updateSourcePath()
{
    updateVersionInfo();
    const QString installData = m_versionInfo["QT_INSTALL_DATA"];
    m_sourcePath = installData;
    QFile qmakeCache(installData + QLatin1String("/.qmake.cache"));
    if (qmakeCache.exists()) {
        qmakeCache.open(QIODevice::ReadOnly | QIODevice::Text);
        QTextStream stream(&qmakeCache);
        while (!stream.atEnd()) {
            QString line = stream.readLine().trimmed();
            if (line.startsWith(QLatin1String("QT_SOURCE_TREE"))) {
                m_sourcePath = line.split(QLatin1Char('=')).at(1).trimmed();
                if (m_sourcePath.startsWith(QLatin1String("$$quote("))) {
                    m_sourcePath.remove(0, 8);
                    m_sourcePath.chop(1);
                }
                break;
            }
        }
    }
    m_sourcePath = QDir::cleanPath(m_sourcePath);
#ifdef Q_OS_WIN
    m_sourcePath = m_sourcePath.toLower();
#endif
}

// Returns the version that was used to build the project in that directory
// That is returns the directory
// To find out whether we already have a qtversion for that directory call
// QtVersion *QtVersionManager::qtVersionForDirectory(const QString directory);
QString QtVersionManager::findQMakeBinaryFromMakefile(const QString &makefile)
{
    bool debugAdding = false;
    QFile fi(makefile);
    if (fi.exists() && fi.open(QFile::ReadOnly)) {
        QTextStream ts(&fi);
        QRegExp r1("QMAKE\\s*=(.*)");
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
                if (fi.exists()) {
                    qmakePath = fi.absoluteFilePath();
#ifdef Q_OS_WIN
                    qmakePath = qmakePath.toLower();
#endif
                    return qmakePath;
                }
            }
        }
    }
    return QString();
}

QtVersion *QtVersionManager::qtVersionForQMakeBinary(const QString &qmakePath)
{
   foreach(QtVersion *v, versions()) {
       if (v->qmakeCommand() == qmakePath) {
           return v;
           break;
       }
   }
   return 0;
}

void dumpQMakeAssignments(const QList<QMakeAssignment> &list)
{
    foreach(const QMakeAssignment &qa, list) {
        qDebug()<<qa.variable<<qa.op<<qa.value;
    }
}

bool QtVersionManager::makefileIsFor(const QString &makefile, const QString &proFile)
{
    if (proFile.isEmpty())
        return true;

    QString line = findQMakeLine(makefile, QLatin1String("# Project:")).trimmed();
    if (line.isEmpty())
        return false;

    line = line.mid(line.indexOf(QChar(':')) + 1);
    line = line.trimmed();

    QFileInfo srcFileInfo(QFileInfo(makefile).absoluteDir(), line);
    QFileInfo proFileInfo(proFile);
    return srcFileInfo == proFileInfo;
}

QPair<QtVersion::QmakeBuildConfigs, QString> QtVersionManager::scanMakeFile(const QString &makefile, QtVersion::QmakeBuildConfigs defaultBuildConfig)
{
    if (debug)
        qDebug()<<"ScanMakeFile, the gory details:";
    QtVersion::QmakeBuildConfigs result = defaultBuildConfig;
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

        foreach(const QMakeAssignment &qa, assignments)
            Utils::QtcProcess::addArg(&result2, qa.variable + qa.op + qa.value);
        if (!afterAssignments.isEmpty()) {
            Utils::QtcProcess::addArg(&result2, QLatin1String("-after"));
            foreach(const QMakeAssignment &qa, afterAssignments)
                Utils::QtcProcess::addArg(&result2, qa.variable + qa.op + qa.value);
        }
    }

    // Dump the gathered information:
    if (debug) {
        qDebug()<<"\n\nDumping information from scanMakeFile";
        qDebug()<<"QMake CONFIG variable parsing";
        qDebug()<<"  "<< (result & QtVersion::NoBuild ? "No Build" : QString::number(int(result)));
        qDebug()<<"  "<< (result & QtVersion::DebugBuild ? "debug" : "release");
        qDebug()<<"  "<< (result & QtVersion::BuildAll ? "debug_and_release" : "no debug_and_release");
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
    QRegExp regExp("([^\\s\\+-]*)\\s*(\\+=|=|-=|~=)(.*)");
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
QtVersion::QmakeBuildConfigs QtVersionManager::qmakeBuildConfigFromCmdArgs(QList<QMakeAssignment> *assignments, QtVersion::QmakeBuildConfigs defaultBuildConfig)
{
    QtVersion::QmakeBuildConfigs result = defaultBuildConfig;
    QList<QMakeAssignment> oldAssignments = *assignments;
    assignments->clear();
    foreach(const QMakeAssignment &qa, oldAssignments) {
        if (qa.variable == "CONFIG") {
            QStringList values = qa.value.split(' ');
            QStringList newValues;
            foreach(const QString &value, values) {
                if (value == "debug") {
                    if (qa.op == "+=")
                        result = result  | QtVersion::DebugBuild;
                    else
                        result = result  & ~QtVersion::DebugBuild;
                } else if (value == "release") {
                    if (qa.op == "+=")
                        result = result & ~QtVersion::DebugBuild;
                    else
                        result = result | QtVersion::DebugBuild;
                } else if (value == "debug_and_release") {
                    if (qa.op == "+=")
                        result = result | QtVersion::BuildAll;
                    else
                        result = result & ~QtVersion::BuildAll;
                } else {
                    newValues.append(value);
                }
                QMakeAssignment newQA = qa;
                newQA.value = newValues.join(" ");
                if (!newValues.isEmpty())
                    assignments->append(newQA);
            }
        } else {
            assignments->append(qa);
        }
    }
    return result;
}

static bool queryQMakeVariables(const QString &binary, QHash<QString, QString> *versionInfo)
{
    const int timeOutMS = 30000; // Might be slow on some machines.
    QFileInfo qmake(binary);
    static const char * const variables[] = {
             "QT_VERSION",
             "QT_INSTALL_DATA",
             "QT_INSTALL_LIBS",
             "QT_INSTALL_HEADERS",
             "QT_INSTALL_DEMOS",
             "QT_INSTALL_EXAMPLES",
             "QT_INSTALL_CONFIGURATION",
             "QT_INSTALL_TRANSLATIONS",
             "QT_INSTALL_PLUGINS",
             "QT_INSTALL_BINS",
             "QT_INSTALL_DOCS",
             "QT_INSTALL_PREFIX",
             "QT_INSTALL_IMPORTS",
             "QMAKEFEATURES"
        };
    QStringList args;
    for (uint i = 0; i < sizeof variables / sizeof variables[0]; ++i)
        args << "-query" << variables[i];
    QProcess process;
    process.start(qmake.absoluteFilePath(), args, QIODevice::ReadOnly);
    if (!process.waitForStarted()) {
        qWarning("Cannot start '%s': %s", qPrintable(binary), qPrintable(process.errorString()));
        return false;
    }
    if (!process.waitForFinished(timeOutMS)) {
        Utils::SynchronousProcess::stopProcess(process);
        qWarning("Timeout running '%s' (%dms).", qPrintable(binary), timeOutMS);
        return false;
    }
    if (process.exitStatus() != QProcess::NormalExit) {
        qWarning("'%s' crashed.", qPrintable(binary));
        return false;
    }
    QByteArray output = process.readAllStandardOutput();
    QTextStream stream(&output);
    while (!stream.atEnd()) {
        const QString line = stream.readLine();
        const int index = line.indexOf(QLatin1Char(':'));
        if (index != -1) {
            const QString value = QDir::fromNativeSeparators(line.mid(index+1));
            if (value != "**Unknown**")
                versionInfo->insert(line.left(index), value);
        }
    }
    return true;
}

void QtVersion::updateVersionInfo() const
{
    if (m_versionInfoUpToDate)
        return;

    // extract data from qmake executable
    m_versionInfo.clear();
    m_notInstalled = false;
    m_hasExamples = false;
    m_hasDocumentation = false;
    m_hasDebuggingHelper = false;
    m_hasQmlDump = false;
    m_hasQmlDebuggingLibrary = false;
    m_hasQmlObserver = false;
    m_qmakeIsExecutable = true;

    QFileInfo fi(qmakeCommand());
    if (!fi.exists() || !fi.isExecutable()) {
        m_qmakeIsExecutable = false;
        return;
    }

    if (!queryQMakeVariables(qmakeCommand(), &m_versionInfo))
        return;

    if (m_versionInfo.contains("QT_INSTALL_DATA")) {
        QString qtInstallData = m_versionInfo.value("QT_INSTALL_DATA");
        m_versionInfo.insert("QMAKE_MKSPECS", QDir::cleanPath(qtInstallData+"/mkspecs"));

        if (!qtInstallData.isEmpty()) {
            m_hasDebuggingHelper = !DebuggingHelperLibrary::debuggingHelperLibraryByInstallData(qtInstallData).isEmpty();
            m_hasQmlDump = !QmlDumpTool::toolByInstallData(qtInstallData, false).isEmpty() || !QmlDumpTool::toolByInstallData(qtInstallData, true).isEmpty();
            m_hasQmlDebuggingLibrary
                    = !QmlDebuggingLibrary::libraryByInstallData(qtInstallData, false).isEmpty()
                || !QmlDebuggingLibrary::libraryByInstallData(qtInstallData, true).isEmpty();
            m_hasQmlObserver = !QmlObserverTool::toolByInstallData(qtInstallData).isEmpty();
        }
    }

    // Now check for a qt that is configured with a prefix but not installed
    if (m_versionInfo.contains("QT_INSTALL_BINS")) {
        QFileInfo fi(m_versionInfo.value("QT_INSTALL_BINS"));
        if (!fi.exists())
            m_notInstalled = true;
    }
    if (m_versionInfo.contains("QT_INSTALL_HEADERS")){
        QFileInfo fi(m_versionInfo.value("QT_INSTALL_HEADERS"));
        if (!fi.exists())
            m_notInstalled = true;
    }
    if (m_versionInfo.contains("QT_INSTALL_DOCS")){
        QFileInfo fi(m_versionInfo.value("QT_INSTALL_DOCS"));
        if (fi.exists())
            m_hasDocumentation = true;
    }
    if (m_versionInfo.contains("QT_INSTALL_EXAMPLES")){
        QFileInfo fi(m_versionInfo.value("QT_INSTALL_EXAMPLES"));
        if (fi.exists())
            m_hasExamples = true;
    }
    if (m_versionInfo.contains("QT_INSTALL_DEMOS")){
        QFileInfo fi(m_versionInfo.value("QT_INSTALL_DEMOS"));
        if (fi.exists())
            m_hasDemos = true;
    }

    m_versionInfoUpToDate = true;
}

QString QtVersion::findQtBinary(const QStringList &possibleCommands) const
{
    const QString qtdirbin = versionInfo().value(QLatin1String("QT_INSTALL_BINS")) + QLatin1Char('/');
    foreach (const QString &possibleCommand, possibleCommands) {
        const QString fullPath = qtdirbin + possibleCommand;
        if (QFileInfo(fullPath).isFile())
            return QDir::cleanPath(fullPath);
    }
    return QString();
}

QString QtVersion::uicCommand() const
{
    if (!isValid())
        return QString();
    if (!m_uicCommand.isNull())
        return m_uicCommand;
#ifdef Q_OS_WIN
    const QStringList possibleCommands(QLatin1String("uic.exe"));
#else
    QStringList possibleCommands;
    possibleCommands << QLatin1String("uic-qt4") << QLatin1String("uic4") << QLatin1String("uic");
#endif
    m_uicCommand = findQtBinary(possibleCommands);
    return m_uicCommand;
}

// Return a list of GUI binary names
// 'foo', 'foo.exe', 'Foo.app/Contents/MacOS/Foo'
static inline QStringList possibleGuiBinaries(const QString &name)
{
#ifdef Q_OS_WIN
    return QStringList(name + QLatin1String(".exe"));
#elif defined(Q_OS_MAC) // 'Foo.app/Contents/MacOS/Foo'
    QString upCaseName = name;
    upCaseName[0] = upCaseName.at(0).toUpper();
    QString macBinary = upCaseName;
    macBinary += QLatin1String(".app/Contents/MacOS/");
    macBinary += upCaseName;
    return QStringList(macBinary);
#else
    return QStringList(name);
#endif
}

QString QtVersion::designerCommand() const
{
    if (!isValid())
        return QString();
    if (m_designerCommand.isNull())
        m_designerCommand = findQtBinary(possibleGuiBinaries(QLatin1String("designer")));
    return m_designerCommand;
}

QString QtVersion::linguistCommand() const
{
    if (!isValid())
        return QString();
    if (m_linguistCommand.isNull())
        m_linguistCommand = findQtBinary(possibleGuiBinaries(QLatin1String("linguist")));
    return m_linguistCommand;
}

QString QtVersion::qmlviewerCommand() const
{
    if (!isValid())
        return QString();

    if (m_qmlviewerCommand.isNull()) {
#ifdef Q_OS_MAC
        const QString qmlViewerName = QLatin1String("QMLViewer");
#else
        const QString qmlViewerName = QLatin1String("qmlviewer");
#endif

        m_qmlviewerCommand = findQtBinary(possibleGuiBinaries(qmlViewerName));
    }
    return m_qmlviewerCommand;
}

void QtVersion::setSystemRoot(const QString &root)
{
    if (root == m_systemRoot)
        return;
    m_systemRoot = root;
    m_abiUpToDate = false;
}

QString QtVersion::systemRoot() const
{
    updateAbiAndMkspec();
    return m_systemRoot;
}

bool QtVersion::supportsTargetId(const QString &id) const
{
    updateAbiAndMkspec();
    return m_targetIds.contains(id);
}

QSet<QString> QtVersion::supportedTargetIds() const
{
    updateAbiAndMkspec();
    return m_targetIds;
}

QList<ProjectExplorer::Abi> QtVersion::qtAbis() const
{
    updateAbiAndMkspec();
    return m_abis;
}

// if none, then it's INVALID everywhere this function is called
void QtVersion::updateAbiAndMkspec() const
{
    if (m_id == -1 || m_abiUpToDate)
        return;

    m_abiUpToDate = true;

    m_targetIds.clear();
    m_abis.clear();
    m_validSystemRoot = true;

//    qDebug()<<"Finding mkspec for"<<qmakeCommand();

    // no .qmake.cache so look at the default mkspec

    QString baseMkspecDir = versionInfo().value("QMAKE_MKSPECS");
    if (baseMkspecDir.isEmpty())
        baseMkspecDir = versionInfo().value("QT_INSTALL_DATA") + "/mkspecs";

#ifdef Q_OS_WIN
    baseMkspecDir = baseMkspecDir.toLower();
#endif

    QString mkspecFullPath = baseMkspecDir + "/default";

    // qDebug() << "default mkspec is located at" << mkspecFullPath;

#ifdef Q_OS_WIN
    QFile f2(mkspecFullPath + "/qmake.conf");
    if (f2.exists() && f2.open(QIODevice::ReadOnly)) {
        while (!f2.atEnd()) {
            QByteArray line = f2.readLine();
            if (line.startsWith("QMAKESPEC_ORIGINAL")) {
                const QList<QByteArray> &temp = line.split('=');
                if (temp.size() == 2) {
                    QString possibleFullPath = temp.at(1).trimmed();
                    // We sometimes get a mix of different slash styles here...
                    possibleFullPath = possibleFullPath.replace('\\', '/');
                    if (QFileInfo(possibleFullPath).exists()) // Only if the path exists
                        mkspecFullPath = possibleFullPath;
                }
                break;
            }
        }
        f2.close();
    }
#elif defined(Q_OS_MAC)
    QFile f2(mkspecFullPath + "/qmake.conf");
    if (f2.exists() && f2.open(QIODevice::ReadOnly)) {
        while (!f2.atEnd()) {
            QByteArray line = f2.readLine();
            if (line.startsWith("MAKEFILE_GENERATOR")) {
                const QList<QByteArray> &temp = line.split('=');
                if (temp.size() == 2) {
                    const QByteArray &value = temp.at(1);
                    if (value.contains("XCODE")) {
                        // we don't want to generate xcode projects...
//                      qDebug() << "default mkspec is xcode, falling back to g++";
                        mkspecFullPath = baseMkspecDir + "/macx-g++";
                    }
                    //resolve mkspec link
                    mkspecFullPath = resolveLink(mkspecFullPath);
                }
                break;
            }
        }
        f2.close();
    }
#else
    mkspecFullPath =resolveLink(mkspecFullPath);
#endif

#ifdef Q_OS_WIN
    mkspecFullPath = mkspecFullPath.toLower();
#endif

    m_mkspecFullPath = mkspecFullPath;
    QString mkspec = m_mkspecFullPath;

    if (mkspec.startsWith(baseMkspecDir)) {
        mkspec = mkspec.mid(baseMkspecDir.length() + 1);
//        qDebug() << "Setting mkspec to"<<mkspec;
    } else {
        QString sourceMkSpecPath = sourcePath() + "/mkspecs";
        if (mkspec.startsWith(sourceMkSpecPath)) {
            mkspec = mkspec.mid(sourceMkSpecPath.length() + 1);
        } else {
            // Do nothing
        }
    }

    m_mkspec = mkspec;
    m_isBuildUsingSbsV2 = false;

    ProFileOption option;
    option.properties = versionInfo();
    ProMessageHandler msgHandler(true);
    ProFileCacheManager::instance()->incRefCount();
    ProFileParser parser(ProFileCacheManager::instance()->cache(), &msgHandler);
    ProFileEvaluator evaluator(&option, &parser, &msgHandler);
    if (ProFile *pro = parser.parsedProFile(m_mkspecFullPath + "/qmake.conf")) {
        evaluator.setCumulative(false);
        evaluator.accept(pro, ProFileEvaluator::LoadProOnly);
        pro->deref();
    }

    QString qmakeCXX = evaluator.values("QMAKE_CXX").join(" ");
    QString makefileGenerator = evaluator.value("MAKEFILE_GENERATOR");
    QString ce_sdk = evaluator.values("CE_SDK").join(QLatin1String(" "));
    QString ce_arch = evaluator.value("CE_ARCH");

    const QString coreLibrary = qtCorePath();

    // Evaluate all the information we have:
    if (!ce_sdk.isEmpty() && !ce_arch.isEmpty()) {
        // Treat windows CE as desktop.
        m_abis.append(ProjectExplorer::Abi(ProjectExplorer::Abi::ArmArchitecture, ProjectExplorer::Abi::WindowsOS,
                                           ProjectExplorer::Abi::WindowsCEFlavor, ProjectExplorer::Abi::PEFormat, false));
        m_targetIds.insert(Constants::DESKTOP_TARGET_ID);
    } else if (makefileGenerator == QLatin1String("SYMBIAN_ABLD") ||
               makefileGenerator == QLatin1String("SYMBIAN_SBSV2") ||
               makefileGenerator == QLatin1String("SYMBIAN_UNIX")) {
        m_isBuildUsingSbsV2 = (makefileGenerator == QLatin1String("SYMBIAN_SBSV2"));
        if (S60Manager::instance()) {
            m_abis.append(ProjectExplorer::Abi(ProjectExplorer::Abi::ArmArchitecture, ProjectExplorer::Abi::SymbianOS,
                                               ProjectExplorer::Abi::UnknownFlavor,
                                               ProjectExplorer::Abi::ElfFormat,
                                               32));
            m_targetIds.insert(QLatin1String(Constants::S60_DEVICE_TARGET_ID));
            m_targetIds.insert(QLatin1String(Constants::S60_EMULATOR_TARGET_ID));
        }
    } else if (MaemoGlobal::isValidMaemo5QtVersion(this)) {
        m_abis.append(ProjectExplorer::Abi(ProjectExplorer::Abi::ArmArchitecture, ProjectExplorer::Abi::LinuxOS,
                                           ProjectExplorer::Abi::MaemoLinuxFlavor, ProjectExplorer::Abi::ElfFormat,
                                           32));
        m_targetIds.insert(QLatin1String(Constants::MAEMO5_DEVICE_TARGET_ID));
    } else if (MaemoGlobal::isValidHarmattanQtVersion(this)) {
        m_abis.append(ProjectExplorer::Abi(ProjectExplorer::Abi::ArmArchitecture, ProjectExplorer::Abi::LinuxOS,
                                           ProjectExplorer::Abi::HarmattanLinuxFlavor,
                                           ProjectExplorer::Abi::ElfFormat,
                                           32));
        m_targetIds.insert(QLatin1String(Constants::HARMATTAN_DEVICE_TARGET_ID));
    } else if (MaemoGlobal::isValidMeegoQtVersion(this)) {
        m_abis.append(ProjectExplorer::Abi(ProjectExplorer::Abi::ArmArchitecture, ProjectExplorer::Abi::LinuxOS,
                                           ProjectExplorer::Abi::MeegoLinuxFlavor,
                                           ProjectExplorer::Abi::ElfFormat, 32));
        m_targetIds.insert(QLatin1String(Constants::MEEGO_DEVICE_TARGET_ID));
    } else if (qmakeCXX.contains("g++")
               || qmakeCXX == "cl" || qmakeCXX == "icl" // intel cl
               || qmakeCXX == QLatin1String("icpc")) {
        m_abis = ProjectExplorer::Abi::abisOfBinary(coreLibrary);
#if defined (Q_OS_WIN)
        if (makefileGenerator == "MINGW") {
            QList<ProjectExplorer::Abi> tmp = m_abis;
            m_abis.clear();
            foreach (const ProjectExplorer::Abi &abi, tmp)
                m_abis.append(ProjectExplorer::Abi(abi.architecture(), abi.os(), ProjectExplorer::Abi::WindowsMSysFlavor,
                                                   abi.binaryFormat(), abi.wordWidth()));
        }
#endif
        m_targetIds.insert(QLatin1String(Constants::DESKTOP_TARGET_ID));
    }

    if (m_abis.isEmpty() && !coreLibrary.isEmpty()) {
        qWarning("Warning: Could not find ABI for '%s' ('%s', %s)/%s by looking at %s:"
                 "Qt Creator does not know about the system includes, "
                 "nor the system defines.",
                 qPrintable(m_mkspecFullPath), qPrintable(displayName()),
                 qPrintable(qmakeCommand()), qPrintable(qmakeCXX), qPrintable(coreLibrary));
    }

    QStringList configValues = evaluator.values("CONFIG");
    m_defaultConfigIsDebugAndRelease = false;
    foreach(const QString &value, configValues) {
        if (value == "debug")
            m_defaultConfigIsDebug = true;
        else if (value == "release")
            m_defaultConfigIsDebug = false;
        else if (value == "build_all")
            m_defaultConfigIsDebugAndRelease = true;
    }
    // Is this actually a simulator Qt?
    if (configValues.contains(QLatin1String("simulator"))) {
        m_targetIds.clear();
        m_targetIds.insert(QLatin1String(Constants::QT_SIMULATOR_TARGET_ID));
    }
    ProFileCacheManager::instance()->decRefCount();

    // Set up systemroot
    if (supportsTargetId(Constants::MAEMO5_DEVICE_TARGET_ID)
            || supportsTargetId(Constants::HARMATTAN_DEVICE_TARGET_ID)) {
        if (m_systemRoot.isNull()) {
            QFile file(QDir::cleanPath(MaemoGlobal::targetRoot(this))
                       + QLatin1String("/information"));
            if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream stream(&file);
                while (!stream.atEnd()) {
                    const QString &line = stream.readLine().trimmed();
                    const QStringList &list = line.split(QLatin1Char(' '));
                    if (list.count() <= 1)
                        continue;
                    if (list.at(0) == QLatin1String("sysroot")) {
                        m_systemRoot = MaemoGlobal::maddeRoot(this)
                                + QLatin1String("/sysroots/") + list.at(1);
                    }
                }
            }
        }
    } else if (supportsTargetId(Constants::S60_DEVICE_TARGET_ID)
           || supportsTargetId(Constants::S60_EMULATOR_TARGET_ID)) {
        if (m_systemRoot.isEmpty())
            m_validSystemRoot = false;

        if (!m_systemRoot.endsWith(QLatin1Char('/')))
            m_systemRoot.append(QLatin1Char('/'));

        QFileInfo cppheader(m_systemRoot + QLatin1String("epoc32/include/stdapis/string.h"));
        if (!cppheader.exists())
            m_validSystemRoot = false;
    } else {
        m_systemRoot = QLatin1String("");
    }
}

QString QtVersion::resolveLink(const QString &path) const
{
    QFileInfo f(path);
    int links = 16;
    while (links-- && f.isSymLink())
        f.setFile(f.symLinkTarget());
    if (links <= 0)
        return QString();
    return f.filePath();
}

QString QtVersion::qtCorePath() const
{
    QList<QDir> dirs;
    dirs << QDir(libraryInstallPath()) << QDir(versionInfo().value(QLatin1String("QT_INSTALL_BINS")));

    QFileInfoList staticLibs;
    foreach (const QDir &d, dirs) {
        QFileInfoList infoList = d.entryInfoList();
        foreach (const QFileInfo &info, infoList) {
            const QString file = info.fileName();
            if (info.isDir()
                    && file.startsWith(QLatin1String("QtCore"))
                    && file.endsWith(QLatin1String(".framework"))) {
                // handle Framework
                const QString libName = file.left(file.lastIndexOf('.'));
                return info.absoluteFilePath() + '/' + libName;
            }
            if (info.isReadable()) {
                if (file.startsWith(QLatin1String("libQtCore"))
                        || file.startsWith(QLatin1String("QtCore"))) {
                    // Only handle static libs if we can not find dynamic ones:
                    if (file.endsWith(".a") || file.endsWith(".lib"))
                        staticLibs.append(info);
                    else if (file.endsWith(QLatin1String(".dll"))
                                || file.endsWith(QString::fromLatin1(".so.") + qtVersionString())
                                || file.endsWith(QLatin1Char('.') + qtVersionString() + QLatin1String(".dylib")))
                        return info.absoluteFilePath();
                }
            }
        }
    }
    // Return path to first static library found:
    if (!staticLibs.isEmpty())
        return staticLibs.at(0).absoluteFilePath();
    return QString();
}

QString QtVersion::sbsV2Directory() const
{
    return m_sbsV2Directory;
}

void QtVersion::setSbsV2Directory(const QString &directory)
{
    QDir dir(directory);
    if (dir.exists(QLatin1String("sbs"))) {
        m_sbsV2Directory = dir.absolutePath();
        return;
    }
    dir.cd("bin");
    if (dir.exists(QLatin1String("sbs"))) {
        m_sbsV2Directory = dir.absolutePath();
        return;
    }
    m_sbsV2Directory = directory;
}

void QtVersion::addToEnvironment(Utils::Environment &env) const
{
    // Generic:
    env.set("QTDIR", QDir::toNativeSeparators(versionInfo().value("QT_INSTALL_DATA")));
    env.prependOrSetPath(versionInfo().value("QT_INSTALL_BINS"));

    // Symbian specific:
    if (supportsTargetId(Constants::S60_DEVICE_TARGET_ID)
            || supportsTargetId(Constants::S60_EMULATOR_TARGET_ID)) {
        // Generic Symbian environment:
        QString epocRootPath(systemRoot());
        QDir epocDir(epocRootPath);

        // Clean up epoc root path for the environment:
        if (!epocRootPath.endsWith(QLatin1Char('/')))
            epocRootPath.append(QLatin1Char('/'));
        if (!isBuildWithSymbianSbsV2()) {
#ifdef Q_OS_WIN
            if (epocRootPath.count() > 2
                    && epocRootPath.at(0).toLower() >= QLatin1Char('a')
                    && epocRootPath.at(0).toLower() <= QLatin1Char('z')
                    && epocRootPath.at(1) == QLatin1Char(':')) {
                epocRootPath = epocRootPath.mid(2);
            }
#endif
        }
        env.set(QLatin1String("EPOCROOT"), QDir::toNativeSeparators(epocRootPath));

        env.prependOrSetPath(epocDir.filePath(QLatin1String("epoc32/tools"))); // e.g. make.exe
        // Windows only:
        if (ProjectExplorer::Abi::hostAbi().os() == ProjectExplorer::Abi::WindowsOS) {
            QString winDir = QLatin1String(qgetenv("WINDIR"));
            if (!winDir.isEmpty())
                env.prependOrSetPath(QDir(winDir).filePath(QLatin1String("system32")));

            if (epocDir.exists(QLatin1String("epoc32/gcc/bin")))
                env.prependOrSetPath(epocDir.filePath(QLatin1String("epoc32/gcc/bin"))); // e.g. cpp.exe, *NOT* gcc.exe
            // Find perl in the special Symbian flavour:
            if (epocDir.exists(QLatin1String("../../tools/perl/bin"))) {
                epocDir.cd(QLatin1String("../../tools/perl/bin"));
                env.prependOrSetPath(epocDir.absolutePath());
            } else {
                env.prependOrSetPath(epocDir.filePath(QLatin1String("perl/bin")));
            }
        }

        // SBSv2:
        if (isBuildWithSymbianSbsV2()) {
            QString sbsHome(env.value(QLatin1String("SBS_HOME")));
            QString sbsConfig = sbsV2Directory();
            if (!sbsConfig.isEmpty()) {
                env.prependOrSetPath(sbsConfig);
                // SBS_HOME is the path minus the trailing /bin:
                env.set(QLatin1String("SBS_HOME"),
                        QDir::toNativeSeparators(sbsConfig.left(sbsConfig.count() - 4))); // We need this for Qt 4.6.3 compatibility
            } else if (!sbsHome.isEmpty()) {
                env.prependOrSetPath(sbsHome + QLatin1String("/bin"));
            }
        }
    }
}

static const char *S60_EPOC_HEADERS[] = {
    "include", "mkspecs/common/symbian", "epoc32/include",
    "epoc32/include/osextensions/stdapis", "epoc32/include/osextensions/stdapis/sys",
    "epoc32/include/stdapis", "epoc32/include/stdapis/sys",
    "epoc32/include/osextensions/stdapis/stlport", "epoc32/include/stdapis/stlport",
    "epoc32/include/oem", "epoc32/include/middleware", "epoc32/include/domain/middleware",
    "epoc32/include/osextensions", "epoc32/include/domain/osextensions",
    "epoc32/include/domain/osextensions/loc", "epoc32/include/domain/middleware/loc",
    "epoc32/include/domain/osextensions/loc/sc", "epoc32/include/domain/middleware/loc/sc"
};

QList<ProjectExplorer::HeaderPath> QtVersion::systemHeaderPathes() const
{
    QList<ProjectExplorer::HeaderPath> result;
    if (supportsTargetId(Constants::S60_DEVICE_TARGET_ID)
            || supportsTargetId(Constants::S60_EMULATOR_TARGET_ID)) {
        QString root = systemRoot() + QLatin1Char('/');
        const int count = sizeof(S60_EPOC_HEADERS) / sizeof(const char *);
        for (int i = 0; i < count; ++i) {
            const QDir dir(root + QLatin1String(S60_EPOC_HEADERS[i]));
            if (dir.exists())
            result.append(ProjectExplorer::HeaderPath(dir.absolutePath(),
                                                      ProjectExplorer::HeaderPath::GlobalHeaderPath));
        }
    }
    return result;
}

int QtVersion::uniqueId() const
{
    return m_id;
}

int QtVersion::getUniqueId()
{
    return QtVersionManager::instance()->getUniqueId();
}

bool QtVersion::isValid() const
{
    updateVersionInfo();
    updateAbiAndMkspec();

    return m_id != -1
            && !qmakeCommand().isEmpty()
            && !displayName().isEmpty()
            && !m_notInstalled
            && m_versionInfo.contains("QT_INSTALL_BINS")
            && (!m_mkspecFullPath.isEmpty() || !m_abiUpToDate)
            && !m_abis.isEmpty()
            && m_qmakeIsExecutable
            && m_validSystemRoot;
}

bool QtVersion::toolChainAvailable(const QString &id) const
{
    if (!isValid())
        return false;

    if (id == QLatin1String(Constants::S60_EMULATOR_TARGET_ID)) {
        QList<ProjectExplorer::ToolChain *> tcList =
                ProjectExplorer::ToolChainManager::instance()->toolChains();
        foreach (ProjectExplorer::ToolChain *tc, tcList) {
            if (tc->id().startsWith(QLatin1String(Constants::WINSCW_TOOLCHAIN_ID)))
                return true;
        }
        return false;
    } else if (id == QLatin1String(Constants::S60_DEVICE_TARGET_ID)) {
        QList<ProjectExplorer::ToolChain *> tcList =
                ProjectExplorer::ToolChainManager::instance()->toolChains();
        foreach (ProjectExplorer::ToolChain *tc, tcList) {
            if (!tc->id().startsWith(Qt4ProjectManager::Constants::WINSCW_TOOLCHAIN_ID))
                return true;
        }
        return false;
    }

    foreach (const ProjectExplorer::Abi &abi, qtAbis())
        if (!ProjectExplorer::ToolChainManager::instance()->findToolChains(abi).isEmpty())
            return true;
    return false;
}

QString QtVersion::invalidReason() const
{
    if (isValid())
        return QString();
    if (qmakeCommand().isEmpty())
        return QCoreApplication::translate("QtVersion", "No qmake path set");
    if (!m_qmakeIsExecutable)
        return QCoreApplication::translate("QtVersion", "qmake does not exist or is not executable");
    if (displayName().isEmpty())
        return QCoreApplication::translate("QtVersion", "Qt version has no name");
    if (m_notInstalled)
        return QCoreApplication::translate("QtVersion", "Qt version is not properly installed, please run make install");
    if (!m_versionInfo.contains("QT_INSTALL_BINS"))
        return QCoreApplication::translate("QtVersion",
                                           "Could not determine the path to the binaries of the Qt installation, maybe the qmake path is wrong?");
    if (m_abiUpToDate && m_mkspecFullPath.isEmpty())
        return QCoreApplication::translate("QtVersion", "The default mkspec symlink is broken.");
    if (m_abiUpToDate && m_abis.isEmpty())
        return QCoreApplication::translate("QtVersion", "Failed to detect the ABI(s) used by the Qt version.");
    if (!m_validSystemRoot)
        return QCoreApplication::translate("QtVersion", "The \"Open C/C++ plugin\" is not installed in the Symbian SDK or the Symbian SDK path is misconfigured");
    return QString();
}

QString QtVersion::description() const
{
    if (!isValid())
        return invalidReason();
    QSet<QString> targets = supportedTargetIds();
    QString envs;
    if (targets.contains(Constants::DESKTOP_TARGET_ID))
        envs = QCoreApplication::translate("QtVersion", "Desktop", "Qt Version is meant for the desktop");
    else if (targets.contains(Constants::S60_DEVICE_TARGET_ID) ||
             targets.contains(Constants::S60_EMULATOR_TARGET_ID))
        envs = QCoreApplication::translate("QtVersion", "Symbian", "Qt Version is meant for Symbian");
    else if (targets.contains(Constants::MAEMO5_DEVICE_TARGET_ID))
        envs = QCoreApplication::translate("QtVersion", "Maemo", "Qt Version is meant for Maemo5");
    else if (targets.contains(Constants::HARMATTAN_DEVICE_TARGET_ID))
        envs = QCoreApplication::translate("QtVersion", "Harmattan ", "Qt Version is meant for Harmattan");
    else if (targets.contains(Constants::MEEGO_DEVICE_TARGET_ID))
        envs = QCoreApplication::translate("QtVersion", "Meego", "Qt Version is meant for Meego");
    else if (targets.contains(Constants::QT_SIMULATOR_TARGET_ID))
        envs = QCoreApplication::translate("QtVersion", "Qt Simulator", "Qt Version is meant for Qt Simulator");
    else
        envs = QCoreApplication::translate("QtVersion", "unkown", "No idea what this Qt Version is meant for!");
    return QCoreApplication::translate("QtVersion", "Qt version %1, using mkspec %2 (%3)")
           .arg(qtVersionString(), mkspec(), envs);
}

QtVersion::QmakeBuildConfigs QtVersion::defaultBuildConfig() const
{
    updateAbiAndMkspec();
    QtVersion::QmakeBuildConfigs result = QtVersion::QmakeBuildConfig(0);

    if (m_defaultConfigIsDebugAndRelease)
        result = QtVersion::BuildAll;
    if (m_defaultConfigIsDebug)
        result = result | QtVersion::DebugBuild;
    return result;
}

bool QtVersion::hasGdbDebuggingHelper() const
{
    updateVersionInfo();
    return m_hasDebuggingHelper;
}


bool QtVersion::hasQmlDump() const
{
    updateVersionInfo();
    return m_hasQmlDump;
}

bool QtVersion::hasQmlDebuggingLibrary() const
{
    updateVersionInfo();
    return m_hasQmlDebuggingLibrary;
}

bool QtVersion::hasQmlObserver() const
{
    updateVersionInfo();
    return m_hasQmlObserver;
}

Utils::Environment QtVersion::qmlToolsEnvironment() const
{
    // FIXME: This seems broken!
    Utils::Environment environment = Utils::Environment::systemEnvironment();
    addToEnvironment(environment);

    // add preferred tool chain, as that is how the tools are built, compare QtVersion::buildDebuggingHelperLibrary
    QList<ProjectExplorer::ToolChain *> alltc =
            ProjectExplorer::ToolChainManager::instance()->findToolChains(qtAbis().at(0));
    if (!alltc.isEmpty())
        alltc.first()->addToEnvironment(environment);

    return environment;
}

QString QtVersion::gdbDebuggingHelperLibrary() const
{
    QString qtInstallData = versionInfo().value("QT_INSTALL_DATA");
    if (qtInstallData.isEmpty())
        return QString();
    return DebuggingHelperLibrary::debuggingHelperLibraryByInstallData(qtInstallData);
}

QString QtVersion::qmlDumpTool(bool debugVersion) const
{
    QString qtInstallData = versionInfo().value("QT_INSTALL_DATA");
    if (qtInstallData.isEmpty())
        return QString();
    return QmlDumpTool::toolByInstallData(qtInstallData, debugVersion);
}

QString QtVersion::qmlDebuggingHelperLibrary(bool debugVersion) const
{
    QString qtInstallData = versionInfo().value("QT_INSTALL_DATA");
    if (qtInstallData.isEmpty())
        return QString();
    return QmlDebuggingLibrary::libraryByInstallData(qtInstallData, debugVersion);
}

QString QtVersion::qmlObserverTool() const
{
    QString qtInstallData = versionInfo().value("QT_INSTALL_DATA");
    if (qtInstallData.isEmpty())
        return QString();
    return QmlObserverTool::toolByInstallData(qtInstallData);
}

QStringList QtVersion::debuggingHelperLibraryLocations() const
{
    QString qtInstallData = versionInfo().value("QT_INSTALL_DATA");
    if (qtInstallData.isEmpty())
        return QStringList();
    return DebuggingHelperLibrary::locationsByInstallData(qtInstallData);
}

bool QtVersion::supportsBinaryDebuggingHelper() const
{
    if (!isValid())
        return false;
    return qtAbis().at(0).os() != ProjectExplorer::Abi::SymbianOS;
}

bool QtVersion::hasDocumentation() const
{
    updateVersionInfo();
    return m_hasDocumentation;
}

QString QtVersion::documentationPath() const
{
    updateVersionInfo();
    return m_versionInfo["QT_INSTALL_DOCS"];
}

bool QtVersion::hasDemos() const
{
    updateVersionInfo();
    return m_hasDemos;
}

QString QtVersion::demosPath() const
{
    updateVersionInfo();
    return m_versionInfo["QT_INSTALL_DEMOS"];
}

QString QtVersion::headerInstallPath() const
{
    updateVersionInfo();
    return m_versionInfo["QT_INSTALL_HEADERS"];
}

QString QtVersion::frameworkInstallPath() const
{
#ifdef Q_OS_MAC
    updateVersionInfo();
    return m_versionInfo["QT_INSTALL_LIBS"];
#else
    return QString();
#endif
}

QString QtVersion::libraryInstallPath() const
{
    updateVersionInfo();
    return m_versionInfo["QT_INSTALL_LIBS"];
}

bool QtVersion::hasExamples() const
{
    updateVersionInfo();
    return m_hasExamples;
}

QString QtVersion::examplesPath() const
{
    updateVersionInfo();
    return m_versionInfo["QT_INSTALL_EXAMPLES"];
}

void QtVersion::invalidateCache()
{
    m_versionInfoUpToDate = false;
}

///////////////
// QtVersionNumber
///////////////


QtVersionNumber::QtVersionNumber(int ma, int mi, int p)
    : majorVersion(ma), minorVersion(mi), patchVersion(p)
{
}

QtVersionNumber::QtVersionNumber(const QString &versionString)
{
    if (!checkVersionString(versionString)) {
        majorVersion = minorVersion = patchVersion = -1;
        return;
    }

    QStringList parts = versionString.split(QLatin1Char('.'));
    majorVersion = parts.at(0).toInt();
    minorVersion = parts.at(1).toInt();
    patchVersion = parts.at(2).toInt();
}

QtVersionNumber::QtVersionNumber()
{
    majorVersion = minorVersion = patchVersion = -1;
}


bool QtVersionNumber::checkVersionString(const QString &version) const
{
    int dots = 0;
    QString validChars = "0123456789.";
    foreach (const QChar &c, version) {
        if (!validChars.contains(c))
            return false;
        if (c == '.')
            ++dots;
    }
    if (dots != 2)
        return false;
    return true;
}

bool QtVersionNumber::operator <(const QtVersionNumber &b) const
{
    if (majorVersion < b.majorVersion)
        return true;
    if (majorVersion > b.majorVersion)
        return false;
    if (minorVersion < b.minorVersion)
        return true;
    if (minorVersion > b.minorVersion)
        return false;
    if (patchVersion < b.patchVersion)
        return true;
    return false;
}

bool QtVersionNumber::operator >(const QtVersionNumber &b) const
{
    return b < *this;
}

bool QtVersionNumber::operator ==(const QtVersionNumber &b) const
{
    return majorVersion == b.majorVersion
            && minorVersion == b.minorVersion
            && patchVersion == b.patchVersion;
}

bool QtVersionNumber::operator !=(const QtVersionNumber &b) const
{
    return !(*this == b);
}

bool QtVersionNumber::operator <=(const QtVersionNumber &b) const
{
    return !(*this > b);
}

bool QtVersionNumber::operator >=(const QtVersionNumber &b) const
{
    return b <= *this;
}
