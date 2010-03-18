/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qtversionmanager.h"

#include "qt4projectmanagerconstants.h"
#include "qt4target.h"
#include "profilereader.h"

#include "qt-maemo/maemomanager.h"
#include "qt-s60/s60manager.h"

#include <projectexplorer/debugginghelper.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/cesdkhandler.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <extensionsystem/pluginmanager.h>
#include <help/helpmanager.h>
#include <utils/qtcassert.h>

#include <QtCore/QProcess>
#include <QtCore/QSettings>
#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtCore/QTextStream>
#include <QtCore/QDir>
#include <QtGui/QApplication>
#include <QtGui/QDesktopServices>

#ifdef Q_OS_WIN32
#include <windows.h>
#endif

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

using ProjectExplorer::DebuggingHelperLibrary;

static const char *QtVersionsSectionName = "QtVersions";
static const char *newQtVersionsKey = "NewQtVersions";
static const char *PATH_AUTODETECTION_SOURCE = "PATH";

enum { debug = 0 };

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
        version->setMingwDirectory(s->value("MingwDirectory").toString());
        version->setMsvcVersion(s->value("msvcVersion").toString());
        version->setMwcDirectory(s->value("MwcDirectory").toString());
        version->setS60SDKDirectory(s->value("S60SDKDirectory").toString());
        version->setGcceDirectory(s->value("GcceDirectory").toString());
        m_versions.append(version);
    }
    s->endArray();
    updateUniqueIdToIndexMap();

    ++m_idcount;
    addNewVersionsFromInstaller();
    updateSystemVersion();

    writeVersionsIntoSettings();

    updateDocumentation();

    // cannot call from ctor, needs to get connected extenernally first
    QTimer::singleShot(0, this, SLOT(updateExamples()));
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
    m_versions.append(version);
    int uniqueId = version->uniqueId();
    m_uniqueIdToIndex.insert(uniqueId, m_versions.count() - 1);
    emit qtVersionsChanged(QList<int>() << uniqueId);
    writeVersionsIntoSettings();
}

void QtVersionManager::removeVersion(QtVersion *version)
{
    QTC_ASSERT(version != 0, return);
    m_versions.removeAll(version);
    int uniqueId = version->uniqueId();
    m_uniqueIdToIndex.remove(uniqueId);
    emit qtVersionsChanged(QList<int>() << uniqueId);
    writeVersionsIntoSettings();
    delete version;
}

bool QtVersionManager::supportsTargetId(const QString &id) const
{
    foreach (QtVersion *version, m_versions) {
        if (version->supportsTargetId(id))
            return true;
    }
    return false;
}

QList<QtVersion *> QtVersionManager::versionsForTargetId(const QString &id) const
{
    QList<QtVersion *> targetVersions;
    foreach (QtVersion *version, m_versions) {
        if (version->supportsTargetId(id))
            targetVersions.append(version);
    }
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
    Help::HelpManager *helpManager
        = ExtensionSystem::PluginManager::instance()->getObject<Help::HelpManager>();
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

void QtVersionManager::updateExamples()
{
    QList<QtVersion *> versions;
    versions.append(m_versions);

    QString examplesPath;
    QString docPath;
    QString demosPath;
    QtVersion *version = 0;
    // try to find a version which has both, demos and examples
    foreach (version, versions) {
        if (version->hasExamples())
            examplesPath = version->examplesPath();
        if (version->hasDemos())
            demosPath = version->demosPath();
        if (!examplesPath.isEmpty() && !demosPath.isEmpty()) {
            emit updateExamples(examplesPath, demosPath, version->sourcePath());
            return;
        }
    }
}

int QtVersionManager::getUniqueId()
{
    return m_idcount++;
}

void QtVersionManager::updateUniqueIdToIndexMap()
{
    m_uniqueIdToIndex.clear();
    for (int i = 0; i < m_versions.size(); ++i)
        m_uniqueIdToIndex.insert(m_versions.at(i)->uniqueId(), i);
}

void QtVersionManager::writeVersionsIntoSettings()
{
    QSettings *s = Core::ICore::instance()->settings();
    s->beginWriteArray(QtVersionsSectionName);
    for (int i = 0; i < m_versions.size(); ++i) {
        const QtVersion *version = m_versions.at(i);
        s->setArrayIndex(i);
        s->setValue("Name", version->displayName());
        // for downwards compat
        s->setValue("Path", version->versionInfo().value("QT_INSTALL_DATA"));
        s->setValue("QMakePath", version->qmakeCommand());
        s->setValue("Id", version->uniqueId());
        s->setValue("MingwDirectory", version->mingwDirectory());
        s->setValue("msvcVersion", version->msvcVersion());
        s->setValue("isAutodetected", version->isAutodetected());
        if (version->isAutodetected())
            s->setValue("autodetectionSource", version->autodetectionSource());
        s->setValue("MwcDirectory", version->mwcDirectory());
        s->setValue("S60SDKDirectory", version->s60SDKDirectory());
        s->setValue("GcceDirectory", version->gcceDirectory());
    }
    s->endArray();
}

QList<QtVersion* > QtVersionManager::versions() const
{
    return m_versions;
}

bool QtVersionManager::isValidId(int id) const
{
    int pos = m_uniqueIdToIndex.value(id, -1);
    return (pos != -1);
}

QtVersion *QtVersionManager::version(int id) const
{
    int pos = m_uniqueIdToIndex.value(id, -1);
    if (pos != -1)
        return m_versions.at(pos);

    return m_emptyVersion;
}

void QtVersionManager::addNewVersionsFromInstaller()
{
    // Add new versions which may have been installed by the WB installer in the form:
    // NewQtVersions="qt 4.3.2=c:\\qt\\qt432\bin\qmake.exe;qt embedded=c:\\qtembedded;"
    // or NewQtVersions="qt 4.3.2=c:\\qt\\qt432bin\qmake.exe=c:\\qtcreator\\mingw\\;
    // i.e.
    // NewQtVersions="versionname=pathtoversion=mingw=s60sdk=gcce=carbide;"
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
                    version->setMingwDirectory(newVersionData[2]);
                if (newVersionData.count() >= 4)
                    version->setS60SDKDirectory(QDir::fromNativeSeparators(newVersionData[3]));
                if (newVersionData.count() >= 5)
                    version->setGcceDirectory(QDir::fromNativeSeparators(newVersionData[4]));
                if (newVersionData.count() >= 6)
                    version->setMwcDirectory(QDir::fromNativeSeparators(newVersionData[5]));

                bool versionWasAlreadyInList = false;
                foreach(const QtVersion * const it, m_versions) {
                    if (QDir(version->qmakeCommand()).canonicalPath() == QDir(it->qmakeCommand()).canonicalPath()) {
                        versionWasAlreadyInList = true;
                        break;
                    }
                }

                if (!versionWasAlreadyInList) {
                    m_versions.append(version);
                } else {
                    // clean up
                    delete version;
                }
            }
        }
    }
    updateUniqueIdToIndexMap();
    settings->setValue(QLatin1String("General/LastQtVersionUpdate"), QDateTime::currentDateTime());
}

void QtVersionManager::updateSystemVersion()
{
    bool haveSystemVersion = false;
    QString systemQMakePath = DebuggingHelperLibrary::findSystemQt(ProjectExplorer::Environment::systemEnvironment());
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
    m_versions.prepend(version);
    updateUniqueIdToIndexMap();
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
    if (a->m_mingwDirectory != b->m_mingwDirectory
        || a->m_msvcVersion != b->m_msvcVersion
        || a->m_mwcDirectory != b->m_mwcDirectory)
        return false;
    if (a->m_displayName != b->displayName())
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
    qSort(m_versions.begin(), m_versions.end(), sortByUniqueId);

    QList<int> changedVersions;
    // So we trying to find the minimal set of changed versions,
    // iterate over both sorted list

    // newVersions and oldVersions iterator
    QList<QtVersion *>::const_iterator nit, nend, oit, oend;
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
    m_versions = newVersions;

    if (!changedVersions.isEmpty())
        updateDocumentation();
    updateUniqueIdToIndexMap();

    updateExamples();
    writeVersionsIntoSettings();

    if (!changedVersions.isEmpty())
        emit qtVersionsChanged(changedVersions);
}

///
/// QtVersion
///

QtVersion::QtVersion(const QString &name, const QString &qmakeCommand, int id,
                     bool isAutodetected, const QString &autodetectionSource)
    : m_displayName(name),
    m_isAutodetected(isAutodetected),
    m_autodetectionSource(autodetectionSource),
    m_hasDebuggingHelper(false),
    m_toolChainUpToDate(false),
    m_versionInfoUpToDate(false),
    m_notInstalled(false),
    m_defaultConfigIsDebug(true),
    m_defaultConfigIsDebugAndRelease(true),
    m_hasExamples(false),
    m_hasDemos(false),
    m_hasDocumentation(false)
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
    m_toolChainUpToDate(false),
    m_versionInfoUpToDate(false),
    m_notInstalled(false),
    m_defaultConfigIsDebug(true),
    m_defaultConfigIsDebugAndRelease(true),
    m_hasExamples(false),
    m_hasDemos(false),
    m_hasDocumentation(false)
{
    m_id = getUniqueId();
    setQMakeCommand(qmakeCommand);
}


QtVersion::QtVersion(const QString &qmakeCommand, bool isAutodetected, const QString &autodetectionSource)
    : m_isAutodetected(isAutodetected),
    m_autodetectionSource(autodetectionSource),
    m_hasDebuggingHelper(false),
    m_toolChainUpToDate(false),
    m_versionInfoUpToDate(false),
    m_notInstalled(false),
    m_defaultConfigIsDebug(true),
    m_defaultConfigIsDebugAndRelease(true),
    m_hasExamples(false),
    m_hasDemos(false),
    m_hasDocumentation(false)
{
    m_id = getUniqueId();
    setQMakeCommand(qmakeCommand);
    m_displayName = qtVersionString();
}

QtVersion::QtVersion()
    :  m_id(-1),
    m_isAutodetected(false),
    m_hasDebuggingHelper(false),
    m_toolChainUpToDate(false),
    m_versionInfoUpToDate(false),
    m_notInstalled(false),
    m_defaultConfigIsDebug(true),
    m_defaultConfigIsDebugAndRelease(true),
    m_hasExamples(false),
    m_hasDemos(false),
    m_hasDocumentation(false)
{
    setQMakeCommand(QString());
}


QtVersion::~QtVersion()
{
}

QString QtVersion::toHtml() const
{
    QString rc;
    QTextStream str(&rc);
    str << "<html></head><body><table>";
    str << "<tr><td><b>" << QtVersionManager::tr("Name:")
        << "</b></td><td>" << displayName() << "</td></tr>";
    str << "<tr><td><b>" << QtVersionManager::tr("Source:")
        << "</b></td><td>" << sourcePath() << "</td></tr>";
    str << "<tr><td><b>" << QtVersionManager::tr("mkspec:")
        << "</b></td><td>" << mkspec() << "</td></tr>";
    str << "<tr><td><b>" << QtVersionManager::tr("qmake:")
        << "</b></td><td>" << m_qmakeCommand << "</td></tr>";
    updateToolChainAndMkspec();
    if (m_defaultConfigIsDebug || m_defaultConfigIsDebugAndRelease) {
        str << "<tr><td><b>" << QtVersionManager::tr("Default:") << "</b></td><td>"
            << (m_defaultConfigIsDebug ? "debug" : "release");
        if (m_defaultConfigIsDebugAndRelease)
            str << " debug_and_release";
        str << "</td></tr>";
    } // default config.
    str << "<tr><td><b>" << QtVersionManager::tr("Version:")
        << "</b></td><td>" << qtVersionString() << "</td></tr>";
    if (hasDebuggingHelper())
        str << "<tr><td><b>" << QtVersionManager::tr("Debugging helper:")
            << "</b></td><td>" << debuggingHelperLibrary() << "</td></tr>";
    const QHash<QString,QString> vInfo = versionInfo();
    if (!vInfo.isEmpty()) {
        const QHash<QString,QString>::const_iterator vcend = vInfo.constEnd();
        for (QHash<QString,QString>::const_iterator it = vInfo.constBegin(); it != vcend; ++it)
            str << "<tr><td><pre>" << it.key() <<  "</pre></td><td>" << it.value() << "</td></tr>";
    }
    str << "<table></body></html>";
    return rc;
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
    updateToolChainAndMkspec();
    return m_mkspec;
}

QString QtVersion::mkspecPath() const
{
    updateToolChainAndMkspec();
    return m_mkspecFullPath;
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
    m_uicCommand.clear();
    m_toolChainUpToDate = false;
    // TODO do i need to optimize this?
    m_versionInfoUpToDate = false;
    m_qtVersionString = QString();
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
QString QtVersionManager::findQMakeBinaryFromMakefile(const QString &directory)
{
    bool debugAdding = false;
    QFile makefile(directory + "/Makefile" );
    if (makefile.exists() && makefile.open(QFile::ReadOnly)) {
        QTextStream ts(&makefile);
        QRegExp r1("QMAKE\\s*=(.*)");
        while (!ts.atEnd()) {
            QString line = ts.readLine();
            if (r1.exactMatch(line)) {
                if (debugAdding)
                    qDebug()<<"#~~ QMAKE is:"<<r1.cap(1).trimmed();
                QFileInfo qmake(r1.cap(1).trimmed());
                QString qmakePath = qmake.filePath();
#ifdef Q_OS_WIN
                qmakePath = qmakePath.toLower();
#endif
                // Is qmake still installed?
                if (QFile::exists(qmakePath))
                    return qmakePath;
            }
        }
        makefile.close();
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

QPair<QtVersion::QmakeBuildConfigs, QStringList> QtVersionManager::scanMakeFile(const QString &directory, QtVersion::QmakeBuildConfigs defaultBuildConfig)
{
    if (debug)
        qDebug()<<"ScanMakeFile, the gory details:";
    QtVersion::QmakeBuildConfigs result = defaultBuildConfig;
    QStringList result2;

    QString line = findQMakeLine(directory);
    if (!line.isEmpty()) {
        if (debug)
            qDebug()<<"Found line"<<line;
        line = trimLine(line);
        QStringList parts = splitLine(line);
        if (debug)
            qDebug()<<"Split into"<<parts;
        QList<QMakeAssignment> assignments;
        QList<QMakeAssignment> afterAssignments;
        QStringList additionalArguments;
        parseParts(parts, &assignments, &afterAssignments, &additionalArguments);

        if (debug) {
            dumpQMakeAssignments(assignments);
            if (!afterAssignments.isEmpty())
                qDebug()<<"-after";
            dumpQMakeAssignments(afterAssignments);
        }

        // Search in assignments for CONFIG(+=,-=,=)(debug,release,debug_and_release)
        // Also remove them from the list
        result = qmakeBuildConfigFromCmdArgs(&assignments, defaultBuildConfig);

        dumpQMakeAssignments(assignments);

        result2.append(additionalArguments);
        foreach(const QMakeAssignment &qa, assignments)
            result2.append(qa.variable + qa.op + qa.value);
        if (!afterAssignments.isEmpty()) {
            result2.append("-after");
            foreach(const QMakeAssignment &qa, afterAssignments)
                result2.append(qa.variable + qa.op + qa.value);
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

QString QtVersionManager::findQMakeLine(const QString &directory)
{
    QFile makefile(directory + QLatin1String("/Makefile" ));
    if (makefile.exists() && makefile.open(QFile::ReadOnly)) {
        QTextStream ts(&makefile);
        while (!ts.atEnd()) {
            const QString line = ts.readLine();
            if (line.startsWith(QLatin1String("# Command:")))
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

QStringList QtVersionManager::splitLine(const QString &line)
{
    // Split on each " ", except on those which are escaped
    // On Unix also remove all escaping
    // On Windows also, but different escaping
    bool escape = false;
    QString currentWord;
    QStringList results;
    int length = line.length();
    for (int i=0; i<length; ++i) {
#ifdef Q_OS_WIN
        if (line.at(i) == '"') {
            escape = !escape;
        } else if (escape || line.at(i) != ' ') {
            currentWord += line.at(i);
        } else {
            results << currentWord;
            currentWord.clear();;
        }
#else
        if (escape) {
            currentWord += line.at(i);
            escape = false;
        } else if (line.at(i) == ' ') {
            results << currentWord;
            currentWord.clear();
        } else if (line.at(i) == '\\') {
            escape = true;
        } else {
            currentWord += line.at(i);
        }
#endif
    }
    return results;
}

void QtVersionManager::parseParts(const QStringList &parts, QList<QMakeAssignment> *assignments, QList<QMakeAssignment> *afterAssignments, QStringList *additionalArguments)
{
    QRegExp regExp("([^\\s\\+-]*)\\s*(\\+=|=|-=|~=)(.*)");
    bool after = false;
    bool ignoreNext = false;
    foreach (const QString &part, parts) {
        if (ignoreNext) {
            // Ignoring
            ignoreNext = false;
        } else if (part == "after") {
            after = true;
        } else if(part.contains('=')) {
            if (regExp.exactMatch(part)) {
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
        } else if (part == "-o") {
            ignoreNext = true;
        } else {
            additionalArguments->append(part);
        }
    }
#if defined(Q_OS_WIN32)
    additionalArguments->removeAll("-win32");
#elif defined(Q_OS_MAC)
    additionalArguments->removeAll("-macx");
#elif defined(Q_OS_QNX6)
    additionalArguments->removeAll("-qnx6");
#else
    additionalArguments->removeAll("-unix");
#endif
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

void QtVersion::updateVersionInfo() const
{
    if (m_versionInfoUpToDate)
        return;

    // extract data from qmake executable
    m_versionInfo.clear();
    m_notInstalled = false;
    m_hasExamples = false;
    m_hasDocumentation = false;

    QFileInfo qmake(qmakeCommand());
    if (qmake.exists() && qmake.isExecutable()) {
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
             "QMAKEFEATURES"
        };
        QStringList args;
        for (uint i = 0; i < sizeof variables / sizeof variables[0]; ++i)
            args << "-query" << variables[i];
        QProcess process;
        process.start(qmake.absoluteFilePath(), args, QIODevice::ReadOnly);
        if (process.waitForFinished(2000)) {
            QByteArray output = process.readAllStandardOutput();
            QTextStream stream(&output);
            while (!stream.atEnd()) {
                const QString line = stream.readLine();
                const int index = line.indexOf(QLatin1Char(':'));
                if (index != -1)
                    m_versionInfo.insert(line.left(index), QDir::fromNativeSeparators(line.mid(index+1)));
            }
        }

        if (m_versionInfo.contains("QT_INSTALL_DATA")) {
            QString qtInstallData = m_versionInfo.value("QT_INSTALL_DATA");
            m_versionInfo.insert("QMAKE_MKSPECS", QDir::cleanPath(qtInstallData+"/mkspecs"));

            if (qtInstallData.isEmpty())
                m_hasDebuggingHelper = false;
            else
                m_hasDebuggingHelper = DebuggingHelperLibrary::debuggingHelperLibraryByInstallData(qtInstallData).isEmpty();
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

bool QtVersion::supportsTargetId(const QString &id) const
{
    updateToolChainAndMkspec();
    return m_targetIds.contains(id);
}

QSet<QString> QtVersion::supportedTargetIds() const
{
    updateToolChainAndMkspec();
    return m_targetIds;
}

QList<QSharedPointer<ProjectExplorer::ToolChain> > QtVersion::toolChains() const
{
    updateToolChainAndMkspec();
    return m_toolChains;
}

ProjectExplorer::ToolChain *QtVersion::toolChain(ProjectExplorer::ToolChain::ToolChainType type) const
{
    foreach(const QSharedPointer<ProjectExplorer::ToolChain> &tcptr, toolChains())
        if (tcptr->type() == type)
            return tcptr.data();
    return 0;
}

QList<ProjectExplorer::ToolChain::ToolChainType> QtVersion::possibleToolChainTypes() const
{
    QList<ProjectExplorer::ToolChain::ToolChainType> types;
    foreach(const QSharedPointer<ProjectExplorer::ToolChain> &tc, toolChains())
        types << tc->type();
    return types;
}

// if none, then it's INVALID everywhere this function is called
void QtVersion::updateToolChainAndMkspec() const
{
    typedef QSharedPointer<ProjectExplorer::ToolChain> ToolChainPtr;
    if (m_toolChainUpToDate)
        return;

    m_toolChains.clear();
    m_targetIds.clear();

    if (!isValid())
        return;

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
                    QFileInfo f3(mkspecFullPath);
                    while (f3.isSymLink()) {
                        mkspecFullPath = f3.symLinkTarget();
                        f3.setFile(mkspecFullPath);
                    }
                }
                break;
            }
        }
        f2.close();
    }
#else
    QFileInfo f2(mkspecFullPath);
    while (f2.isSymLink()) {
        mkspecFullPath = f2.symLinkTarget();
        f2.setFile(mkspecFullPath);
    }
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

//    qDebug()<<"mkspec for "<<qmakeCommand()<<" is "<<m_mkspec<<m_mkspecFullPath;

    ProFileOption option;
    option.properties = versionInfo();
    option.cache = ProFileCacheManager::instance()->cache();
    ProFileCacheManager::instance()->incRefCount();
    ProFileReader *reader = new ProFileReader(&option);
    reader->setCumulative(false);
    reader->setParsePreAndPostFiles(false);
    reader->readProFile(m_mkspecFullPath + "/qmake.conf");
    QString qmakeCXX = reader->value("QMAKE_CXX");
    QString makefileGenerator = reader->value("MAKEFILE_GENERATOR");
    QString ce_sdk = reader->values("CE_SDK").join(QLatin1String(" "));
    QString ce_arch = reader->value("CE_ARCH");
    QString qt_arch = reader->value("QT_ARCH");
    if (!ce_sdk.isEmpty() && !ce_arch.isEmpty()) {
        QString wincePlatformName = ce_sdk + " (" + ce_arch + QLatin1Char(')');
        m_toolChains << ToolChainPtr(ProjectExplorer::ToolChain::createWinCEToolChain(msvcVersion(), wincePlatformName));
        m_targetIds.insert(Constants::DESKTOP_TARGET_ID);
    } else if (makefileGenerator == QLatin1String("SYMBIAN_ABLD") ||
               makefileGenerator == QLatin1String("SYMBIAN_SBSV2") ||
               makefileGenerator == QLatin1String("SYMBIAN_UNIX")) {
        if (S60Manager *s60mgr = S60Manager::instance()) {
#    ifdef Q_OS_WIN
            m_targetIds.insert(QLatin1String(Constants::S60_DEVICE_TARGET_ID));
            m_toolChains << ToolChainPtr(s60mgr->createGCCEToolChain(this));
            if (S60Manager::hasRvctCompiler())
                m_toolChains << ToolChainPtr(s60mgr->createRVCTToolChain(this, ProjectExplorer::ToolChain::RVCT_ARMV5))
                             << ToolChainPtr(s60mgr->createRVCTToolChain(this, ProjectExplorer::ToolChain::RVCT_ARMV6));
            if (!mwcDirectory().isEmpty()) {
                m_toolChains << ToolChainPtr(s60mgr->createWINSCWToolChain(this));
                m_targetIds.insert(QLatin1String(Constants::S60_EMULATOR_TARGET_ID));
            }
#    else
            if (S60Manager::hasRvctCompiler())
                m_toolChains << ToolChainPtr(s60mgr->createRVCTToolChain(this, ProjectExplorer::ToolChain::RVCT_ARMV5_GNUPOC));
            m_toolChains << ToolChainPtr(s60mgr->createGCCE_GnuPocToolChain(this));
            m_targetIds.insert(QLatin1String(Constants::S60_DEVICE_TARGET_ID));
#    endif
        }
    } else if (qt_arch.startsWith(QLatin1String("arm"))
               && MaemoManager::instance().isValidMaemoQtVersion(this)) {
        m_toolChains << ToolChainPtr(MaemoManager::instance().maemoToolChain(this));
        m_targetIds.insert(QLatin1String(Constants::MAEMO_DEVICE_TARGET_ID));
    } else if (qmakeCXX == "cl" || qmakeCXX == "icl") {
        // TODO proper support for intel cl
        m_toolChains << ToolChainPtr(
                ProjectExplorer::ToolChain::createMSVCToolChain(msvcVersion(), isQt64Bit()));
        m_targetIds.insert(QLatin1String(Constants::DESKTOP_TARGET_ID));
    } else if (qmakeCXX == "g++" && makefileGenerator == "MINGW") {
        ProjectExplorer::Environment env = ProjectExplorer::Environment::systemEnvironment();
        //addToEnvironment(env);
        env.prependOrSetPath(mingwDirectory() + "/bin");
        qmakeCXX = env.searchInPath(qmakeCXX);
        m_toolChains << ToolChainPtr(
                ProjectExplorer::ToolChain::createMinGWToolChain(qmakeCXX, mingwDirectory()));
        m_targetIds.insert(QLatin1String(Constants::DESKTOP_TARGET_ID));
    } else if (qmakeCXX == "g++" || qmakeCXX == "icc") {
        ProjectExplorer::Environment env = ProjectExplorer::Environment::systemEnvironment();
        //addToEnvironment(env);
        qmakeCXX = env.searchInPath(qmakeCXX);
        if (qmakeCXX.isEmpty()) {
            // macx-xcode mkspec resets the value of QMAKE_CXX.
            // Unfortunately, we need a valid QMAKE_CXX to configure the parser.
            qmakeCXX = QLatin1String("cc");
        }
        m_toolChains << ToolChainPtr(ProjectExplorer::ToolChain::createGccToolChain(qmakeCXX));
        m_targetIds.insert(QLatin1String(Constants::DESKTOP_TARGET_ID));
    }

    if (m_toolChains.isEmpty()) {
        qDebug()<<"Could not create ToolChain for"<<m_mkspecFullPath<<qmakeCXX;
        qDebug()<<"Qt Creator doesn't know about the system includes, nor the systems defines.";
    }

    QStringList configValues = reader->values("CONFIG");
    m_defaultConfigIsDebugAndRelease = false;
    foreach(const QString &value, configValues) {
        if (value == "debug")
            m_defaultConfigIsDebug = true;
        else if (value == "release")
            m_defaultConfigIsDebug = false;
        else if (value == "build_all")
            m_defaultConfigIsDebugAndRelease = true;
    }

    delete reader;
    ProFileCacheManager::instance()->decRefCount();
    m_toolChainUpToDate = true;
}

QString QtVersion::mwcDirectory() const
{
    return m_mwcDirectory;
}

void QtVersion::setMwcDirectory(const QString &directory)
{
    m_mwcDirectory = directory;
    m_toolChainUpToDate = false;
}
QString QtVersion::s60SDKDirectory() const
{
    return m_s60SDKDirectory;
}

void QtVersion::setS60SDKDirectory(const QString &directory)
{
    m_s60SDKDirectory = directory;
    m_toolChainUpToDate = false;
}

QString QtVersion::gcceDirectory() const
{
    return m_gcceDirectory;
}

void QtVersion::setGcceDirectory(const QString &directory)
{
    m_gcceDirectory = directory;
    m_toolChainUpToDate = false;
}

QString QtVersion::mingwDirectory() const
{
    return m_mingwDirectory;
}

void QtVersion::setMingwDirectory(const QString &directory)
{
    m_mingwDirectory = directory;
    m_toolChainUpToDate = false;
}

QString QtVersion::msvcVersion() const
{
    return m_msvcVersion;
}

void QtVersion::setMsvcVersion(const QString &version)
{
    m_msvcVersion = version;
    m_toolChainUpToDate = false;
}

void QtVersion::addToEnvironment(ProjectExplorer::Environment &env) const
{
    env.set("QTDIR", QDir::toNativeSeparators(versionInfo().value("QT_INSTALL_DATA")));
    env.prependOrSetPath(versionInfo().value("QT_INSTALL_BINS"));
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
    return m_id != -1
            && !qmakeCommand().isEmpty()
            && !displayName().isEmpty()
            && !m_notInstalled
            && m_versionInfo.contains("QT_INSTALL_BINS");
}

QString QtVersion::invalidReason() const
{
    if (isValid())
        return QString();
    if (qmakeCommand().isEmpty())
        return QCoreApplication::translate("QtVersion", "No qmake path set");
    if (displayName().isEmpty())
        return QCoreApplication::translate("QtVersion", "Qt version has no name");
    if (m_notInstalled)
        return QCoreApplication::translate("QtVersion", "Qt version is not properly installed, please run make install");
    if (!m_versionInfo.contains("QT_INSTALL_BINS"))
        return QCoreApplication::translate("QtVersion",
					   "Could not determine the path to the binaries of the Qt installation, maybe the qmake path is wrong?");
    return QString();
}

QtVersion::QmakeBuildConfigs QtVersion::defaultBuildConfig() const
{
    updateToolChainAndMkspec();
    QtVersion::QmakeBuildConfigs result = QtVersion::QmakeBuildConfig(0);

    if (m_defaultConfigIsDebugAndRelease)
        result = QtVersion::BuildAll;
    if (m_defaultConfigIsDebug)
        result = result | QtVersion::DebugBuild;
    return result;
}

bool QtVersion::hasDebuggingHelper() const
{
    updateVersionInfo();
    return m_hasDebuggingHelper;
}

QString QtVersion::debuggingHelperLibrary() const
{
    QString qtInstallData = versionInfo().value("QT_INSTALL_DATA");
    if (qtInstallData.isEmpty())
        return QString();
    return DebuggingHelperLibrary::debuggingHelperLibraryByInstallData(qtInstallData);
}

QStringList QtVersion::debuggingHelperLibraryLocations() const
{
    QString qtInstallData = versionInfo().value("QT_INSTALL_DATA");
    if (qtInstallData.isEmpty())
        return QStringList();
    return DebuggingHelperLibrary::debuggingHelperLibraryLocationsByInstallData(qtInstallData);
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

bool QtVersion::isQt64Bit() const
{
        const QString make = qmakeCommand();
//        qDebug() << make;
        bool isAmd64 = false;
#ifdef Q_OS_WIN32
#  ifdef __GNUC__   // MinGW lacking some definitions/winbase.h
#    define SCS_64BIT_BINARY 6
#  endif
        DWORD binaryType = 0;
        bool success = GetBinaryTypeW(reinterpret_cast<const TCHAR*>(make.utf16()), &binaryType) != 0;
        if (success && binaryType == SCS_64BIT_BINARY)
            isAmd64=true;
//        qDebug() << "isAmd64:" << isAmd64 << binaryType;
        return isAmd64;
#else
        Q_UNUSED(isAmd64)
        return false;
#endif
}

QString QtVersion::buildDebuggingHelperLibrary()
{
    QString qtInstallData = versionInfo().value("QT_INSTALL_DATA");
    if (qtInstallData.isEmpty())
        return QString();
    ProjectExplorer::Environment env = ProjectExplorer::Environment::systemEnvironment();
    addToEnvironment(env);

    // TODO: the debugging helper doesn't comply to actual tool chain yet
    QList<QSharedPointer<ProjectExplorer::ToolChain> > alltc = toolChains();
    ProjectExplorer::ToolChain *tc = alltc.isEmpty() ? 0 : alltc.first().data();
    if (!tc)
        return QCoreApplication::translate("QtVersion", "The Qt Version has no toolchain.");
    tc->addToEnvironment(env);
    QString output;
    QString directory = DebuggingHelperLibrary::copyDebuggingHelperLibrary(qtInstallData, &output);
    if (!directory.isEmpty())
        output += DebuggingHelperLibrary::buildDebuggingHelperLibrary(directory, tc->makeCommand(), qmakeCommand(), mkspec(), env);
    m_hasDebuggingHelper = !debuggingHelperLibrary().isEmpty();
    return output;
}

