/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
#include "profilereader.h"

#ifdef QTCREATOR_WITH_S60
#include "qt-s60/s60manager.h"
#endif

#include <projectexplorer/debugginghelper.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/cesdkhandler.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <extensionsystem/pluginmanager.h>
#include <help/helpplugin.h>
#include <utils/qtcassert.h>


#include <QtCore/QProcess>
#include <QtCore/QSettings>
#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtGui/QDesktopServices>

#ifdef Q_OS_WIN32
#include <windows.h>
#endif

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

using ProjectExplorer::DebuggingHelperLibrary;

static const char *QtVersionsSectionName = "QtVersions";
static const char *defaultQtVersionKey = "DefaultQtVersion";
static const char *newQtVersionsKey = "NewQtVersions";
static const char *PATH_AUTODETECTION_SOURCE = "PATH";

enum { debug = 0 };

QtVersionManager *QtVersionManager::m_self = 0;

QtVersionManager::QtVersionManager()
    : m_emptyVersion(new QtVersion)
{
    m_self = this;
    QSettings *s = Core::ICore::instance()->settings();
    m_defaultVersion = s->value(defaultQtVersionKey, 0).toInt();

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
#ifdef QTCREATOR_WITH_S60
        version->setMwcDirectory(s->value("MwcDirectory").toString());
        version->setS60SDKDirectory(s->value("S60SDKDirectory").toString());
#endif
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
    m_uniqueIdToIndex.insert(version->uniqueId(), m_versions.count() - 1);
    emit qtVersionsChanged();
    writeVersionsIntoSettings();
}

void QtVersionManager::removeVersion(QtVersion *version)
{
    QTC_ASSERT(version != 0, return);
    m_versions.removeAll(version);
    m_uniqueIdToIndex.remove(version->uniqueId());
    emit qtVersionsChanged();
    writeVersionsIntoSettings();
    delete version;
}

void QtVersionManager::updateDocumentation()
{
    Help::HelpManager *helpManager
        = ExtensionSystem::PluginManager::instance()->getObject<Help::HelpManager>();
    Q_ASSERT(helpManager);
    QStringList fileEndings = QStringList() << "/qch/qt.qch" << "/qch/qmake.qch" << "/qch/designer.qch";
    QStringList files;
    foreach (QtVersion *version, m_versions) {
        QString docPath = version->documentationPath();
        foreach (const QString &fileEnding, fileEndings)
            files << docPath+fileEnding;
    }
    helpManager->registerDocumentation(files);
}

void QtVersionManager::updateExamples()
{
    QList<QtVersion *> versions;
    versions.append(defaultVersion());
    versions.append(m_versions);

    QString examplesPath;
    QString docPath;
    QString demosPath;
    QtVersion *version = 0;
    // try to find a version which has both, demos and examples, starting with default Qt
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
    s->setValue(defaultQtVersionKey, m_defaultVersion);
    s->beginWriteArray(QtVersionsSectionName);
    for (int i = 0; i < m_versions.size(); ++i) {
        const QtVersion *version = m_versions.at(i);
        s->setArrayIndex(i);
        s->setValue("Name", version->name());
        // for downwards compat
        s->setValue("Path", version->versionInfo().value("QT_INSTALL_DATA"));
        s->setValue("QMakePath", version->qmakeCommand());
        s->setValue("Id", version->uniqueId());
        s->setValue("MingwDirectory", version->mingwDirectory());
        s->setValue("msvcVersion", version->msvcVersion());
        s->setValue("isAutodetected", version->isAutodetected());
        if (version->isAutodetected())
            s->setValue("autodetectionSource", version->autodetectionSource());
#ifdef QTCREATOR_WITH_S60
        s->setValue("MwcDirectory", version->mwcDirectory());
        s->setValue("S60SDKDirectory", version->s60SDKDirectory());
#endif
    }
    s->endArray();
}

QList<QtVersion* > QtVersionManager::versions() const
{
    return m_versions;
}

QtVersion *QtVersionManager::version(int id) const
{
    int pos = m_uniqueIdToIndex.value(id, -1);
    if (pos != -1)
        return m_versions.at(pos);

    if (m_defaultVersion < m_versions.count())
        return m_versions.at(m_defaultVersion);
    else
        return m_emptyVersion;
}

void QtVersionManager::addNewVersionsFromInstaller()
{
    // Add new versions which may have been installed by the WB installer in the form:
    // NewQtVersions="qt 4.3.2=c:\\qt\\qt432\bin\qmake.exe;qt embedded=c:\\qtembedded;"
    // or NewQtVersions="qt 4.3.2=c:\\qt\\qt432bin\qmake.exe=c:\\qtcreator\\mingw\\=prependToPath;
    // Duplicate entries are not added, the first new version is set as default.
    QSettings *settings = Core::ICore::instance()->settings();

    if (!settings->contains(newQtVersionsKey) &&
        !settings->contains(QLatin1String("Installer/")+newQtVersionsKey))
        return;

//    qDebug()<<"QtVersionManager::addNewVersionsFromInstaller()";

    QString newVersionsValue = settings->value(newQtVersionsKey).toString();
    if (newVersionsValue.isEmpty())
        newVersionsValue = settings->value(QLatin1String("Installer/")+newQtVersionsKey).toString();

    QStringList newVersionsList = newVersionsValue.split(';', QString::SkipEmptyParts);
    bool defaultVersionWasReset = false;
    foreach (QString newVersion, newVersionsList) {
        QStringList newVersionData = newVersion.split('=');
        if (newVersionData.count()>=2) {
            if (QFile::exists(newVersionData[1])) {
                QtVersion *version = new QtVersion(newVersionData[0], newVersionData[1], m_idcount++ );
                if (newVersionData.count() >= 3)
                    version->setMingwDirectory(newVersionData[2]);

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
                if (!defaultVersionWasReset) {
                    m_defaultVersion = versionWasAlreadyInList? m_defaultVersion : m_versions.count() - 1;
                    defaultVersionWasReset = true;
                }
            }
        }
    }
    settings->remove(newQtVersionsKey);
    settings->remove(QLatin1String("Installer/")+newQtVersionsKey);
    updateUniqueIdToIndexMap();
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
            version->setName(tr("Qt in PATH"));
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
    if (m_versions.size() > 1) // we had other versions before adding system version
        ++m_defaultVersion;
}

QtVersion *QtVersionManager::defaultVersion() const
{
    if (m_defaultVersion < m_versions.count())
        return m_versions.at(m_defaultVersion);
    else
        return m_emptyVersion;
}

void QtVersionManager::setNewQtVersions(QList<QtVersion *> newVersions, int newDefaultVersion)
{
    bool versionPathsChanged = m_versions.size() != newVersions.size();
    if (!versionPathsChanged) {
        for (int i = 0; i < m_versions.size(); ++i) {
            if (m_versions.at(i)->qmakeCommand() != newVersions.at(i)->qmakeCommand()) {
                versionPathsChanged = true;
                break;
            }
        }
    }
    qDeleteAll(m_versions);
    m_versions.clear();
    foreach(QtVersion *version, newVersions)
        m_versions.append(new QtVersion(*version));
    if (versionPathsChanged)
        updateDocumentation();
    updateUniqueIdToIndexMap();

    bool emitDefaultChanged = false;
    if (m_defaultVersion != newDefaultVersion) {
        m_defaultVersion = newDefaultVersion;
        emitDefaultChanged = true;
    }

    emit qtVersionsChanged();
    if (emitDefaultChanged) {
        emit defaultQtVersionChanged();
    }

    updateExamples();
    writeVersionsIntoSettings();
}



///
/// QtVersion
///

QtVersion::QtVersion(const QString &name, const QString &qmakeCommand, int id,
                     bool isAutodetected, const QString &autodetectionSource)
    : m_name(name),
    m_isAutodetected(isAutodetected),
    m_autodetectionSource(autodetectionSource),
    m_hasDebuggingHelper(false),
    m_mkspecUpToDate(false),
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
    : m_name(name),
    m_isAutodetected(isAutodetected),
    m_autodetectionSource(autodetectionSource),
    m_hasDebuggingHelper(false),
    m_mkspecUpToDate(false),
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
    m_mkspecUpToDate(false),
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
    m_name = qtVersionString();
}

QtVersion::QtVersion()
    : m_name(QString::null),
    m_id(-1),
    m_isAutodetected(false),
    m_hasDebuggingHelper(false),
    m_mkspecUpToDate(false),
    m_versionInfoUpToDate(false),
    m_notInstalled(false),
    m_defaultConfigIsDebug(true),
    m_defaultConfigIsDebugAndRelease(true),
    m_hasExamples(false),
    m_hasDemos(false),
    m_hasDocumentation(false)
{
    setQMakeCommand(QString::null);
}


QtVersion::~QtVersion()
{

}

QString QtVersion::name() const
{
    return m_name;
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
    updateMkSpec();
    return m_mkspec;
}

QString QtVersion::mkspecPath() const
{
    updateMkSpec();
    return m_mkspecFullPath;
}

QString QtVersion::qtVersionString() const
{
    return m_qtVersionString;
}

QHash<QString,QString> QtVersion::versionInfo() const
{
    updateVersionInfo();
    return m_versionInfo;
}

QString QtVersion::qmakeCXX() const
{
    updateQMakeCXX();
    return m_qmakeCXX;
}


void QtVersion::setName(const QString &name)
{
    m_name = name;
}

void QtVersion::setQMakeCommand(const QString& qmakeCommand)
{
    m_qmakeCommand = QDir::fromNativeSeparators(qmakeCommand);
#ifdef Q_OS_WIN
    m_qmakeCommand = m_qmakeCommand.toLower();
#endif
    m_designerCommand = m_linguistCommand = m_uicCommand = QString::null;
    m_mkspecUpToDate = false;
    m_qmakeCXX = QString::null;
    m_qmakeCXXUpToDate = false;
    // TODO do i need to optimize this?
    m_versionInfoUpToDate = false;
    m_hasDebuggingHelper = !debuggingHelperLibrary().isEmpty();

    QFileInfo qmake(qmakeCommand);
    if (qmake.exists() && qmake.isExecutable()) {
        m_qtVersionString = DebuggingHelperLibrary::qtVersionForQMake(qmake.absoluteFilePath());
    } else {
        m_qtVersionString = QString::null;
    }
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
}

// Returns the version that was used to build the project in that directory
// That is returns the directory
// To find out wheter we already have a qtversion for that directory call
// QtVersion *QtVersionManager::qtVersionForDirectory(const QString directory);
QString QtVersionManager::findQMakeBinaryFromMakefile(const QString &directory)
{
    bool debugAdding = false;
    QFile makefile(directory + "/Makefile" );
    if (makefile.exists() && makefile.open(QFile::ReadOnly)) {
        QTextStream ts(&makefile);
        while (!ts.atEnd()) {
            QString line = ts.readLine();
            QRegExp r1("QMAKE\\s*=(.*)");
            if (r1.exactMatch(line)) {
                if (debugAdding)
                    qDebug()<<"#~~ QMAKE is:"<<r1.cap(1).trimmed();
                QFileInfo qmake(r1.cap(1).trimmed());
                QString qmakePath = qmake.filePath();
#ifdef Q_OS_WIN
                qmakePath = qmakePath.toLower();
#endif
                return qmakePath;
            }
        }
        makefile.close();
    }
    return QString::null;
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
    foreach(QMakeAssignment qa, list) {
        qDebug()<<qa.variable<<qa.op<<qa.value;
    }
}

QPair<QtVersion::QmakeBuildConfig, QStringList> QtVersionManager::scanMakeFile(const QString &directory, QtVersion::QmakeBuildConfig defaultBuildConfig)
{
    if (debug)
        qDebug()<<"ScanMakeFile, the gory details:";
    QtVersion::QmakeBuildConfig result = QtVersion::NoBuild;
    QStringList result2;

    QString line = findQMakeLine(directory);
    if (!line.isEmpty()) {
        if (debug)
            qDebug()<<"Found line"<<line;
        line = trimLine(line);
        QStringList parts = splitLine(line);
        if (debug)
            qDebug()<<"Splitted into"<<parts;
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
        foreach(QMakeAssignment qa, assignments)
            result2.append(qa.variable + qa.op + qa.value);
        if (!afterAssignments.isEmpty()) {
            result2.append("-after");
            foreach(QMakeAssignment qa, afterAssignments)
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
    QFile makefile(directory + "/Makefile" );
    if (makefile.exists() && makefile.open(QFile::ReadOnly)) {
        QTextStream ts(&makefile);
        while (!ts.atEnd()) {
            QString line = ts.readLine();
            if (line.startsWith("# Command:"))
                return line;
        }
    }
    return QString();
}

/// This function trims the "#Command /path/to/qmake" from the the line
QString QtVersionManager::trimLine(const QString line)
{

    // Actually the first space after #Command: /path/to/qmake
    int firstSpace = line.indexOf(" ", 11);
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
QtVersion::QmakeBuildConfig QtVersionManager::qmakeBuildConfigFromCmdArgs(QList<QMakeAssignment> *assignments, QtVersion::QmakeBuildConfig defaultBuildConfig)
{
    QtVersion::QmakeBuildConfig result = defaultBuildConfig;
    QList<QMakeAssignment> oldAssignments = *assignments;
    assignments->clear();
    foreach(QMakeAssignment qa, oldAssignments) {
        if (qa.variable == "CONFIG") {
            QStringList values = qa.value.split(' ');
            QStringList newValues;
            foreach(const QString &value, values) {
                if (value == "debug") {
                    if (qa.op == "+=")
                        result = QtVersion::QmakeBuildConfig(result  | QtVersion::DebugBuild);
                    else
                        result = QtVersion::QmakeBuildConfig(result  & ~QtVersion::DebugBuild);
                } else if (value == "release") {
                    if (qa.op == "+=")
                        result = QtVersion::QmakeBuildConfig(result & ~QtVersion::DebugBuild);
                    else
                        result = QtVersion::QmakeBuildConfig(result | QtVersion::DebugBuild);
                } else if (value == "debug_and_release") {
                    if (qa.op == "+=")
                        result = QtVersion::QmakeBuildConfig(result | QtVersion::BuildAll);
                    else
                        result = QtVersion::QmakeBuildConfig(result & ~QtVersion::BuildAll);
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
    if (ProjectExplorer::DebuggingHelperLibrary::possibleQMakeCommands().contains(qmake.fileName())
        && qmake.exists()) {
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
             "QT_INSTALL_PREFIX"
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
                QString line = stream.readLine();
                int index = line.indexOf(":");
                if (index != -1)
                    m_versionInfo.insert(line.left(index), QDir::fromNativeSeparators(line.mid(index+1)));
            }
        }

        if (m_versionInfo.contains("QT_INSTALL_DATA"))
            m_versionInfo.insert("QMAKE_MKSPECS", QDir::cleanPath(m_versionInfo.value("QT_INSTALL_DATA")+"/mkspecs"));

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

        // Parse qconfigpri
        QString baseDir = m_versionInfo.value("QT_INSTALL_DATA");
        QFile qconfigpri(baseDir + QLatin1String("/mkspecs/qconfig.pri"));
        if (qconfigpri.exists()) {
            qconfigpri.open(QIODevice::ReadOnly | QIODevice::Text);
            QTextStream stream(&qconfigpri);
            while (!stream.atEnd()) {
                QString line = stream.readLine().trimmed();
                if (line.startsWith(QLatin1String("CONFIG"))) {
                    m_defaultConfigIsDebugAndRelease = false;
                    QStringList values = line.split(QLatin1Char('=')).at(1).trimmed().split(" ");
                    foreach(const QString &value, values) {
                        if (value == "debug")
                            m_defaultConfigIsDebug = true;
                        else if (value == "release")
                            m_defaultConfigIsDebug = false;
                        else if (value == "build_all")
                            m_defaultConfigIsDebugAndRelease = true;
                    }
                }
            }
        }
    }
    m_versionInfoUpToDate = true;
}

bool QtVersion::isInstalled() const
{
    updateVersionInfo();
    return !m_notInstalled;
}

void QtVersion::updateMkSpec() const
{
    if (m_mkspecUpToDate)
        return;
    //qDebug()<<"Finding mkspec for"<<path();

    QString mkspec;
    // no .qmake.cache so look at the default mkspec
    QString mkspecPath = versionInfo().value("QMAKE_MKSPECS");
    if (mkspecPath.isEmpty())
        mkspecPath = versionInfo().value("QT_INSTALL_DATA") + "/mkspecs/default";
    else
        mkspecPath = mkspecPath + "/default";
//     qDebug() << "default mkspec is located at" << mkspecPath;
#ifdef Q_OS_WIN
    QFile f2(mkspecPath + "/qmake.conf");
    if (f2.exists() && f2.open(QIODevice::ReadOnly)) {
        while (!f2.atEnd()) {
            QByteArray line = f2.readLine();
            if (line.startsWith("QMAKESPEC_ORIGINAL")) {
                const QList<QByteArray> &temp = line.split('=');
                if (temp.size() == 2) {
                    mkspec = temp.at(1).trimmed();
                }
                break;
            }
        }
        f2.close();
    }
#elif defined(Q_OS_MAC)
    QFile f2(mkspecPath + "/qmake.conf");
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
                        mkspec = "macx-g++";
                    } else {
                        //resolve mkspec link
                        QFileInfo f3(mkspecPath);
                        if (f3.isSymLink()) {
                            mkspec = f3.symLinkTarget();
                        }
                    }
                }
                break;
            }
        }
        f2.close();
    }
#else
    QFileInfo f2(mkspecPath);
    if (f2.isSymLink()) {
        mkspec = f2.symLinkTarget();
    }
#endif

    m_mkspecFullPath = mkspec;
    int index = mkspec.lastIndexOf('/');
    if (index == -1)
        index = mkspec.lastIndexOf('\\');
    QString mkspecDir = QDir(versionInfo().value("QT_INSTALL_DATA") + "/mkspecs/").canonicalPath();
    if (index >= 0 && QDir(mkspec.left(index)).canonicalPath() == mkspecDir)
        mkspec = mkspec.mid(index+1).trimmed();

    m_mkspec = mkspec;
    m_mkspecUpToDate = true;
//    qDebug()<<"mkspec for "<<versionInfo().value("QT_INSTALL_DATA")<<" is "<<mkspec;
}

void QtVersion::updateQMakeCXX() const
{
    if (m_qmakeCXXUpToDate)
        return;
    ProFileReader *reader = new ProFileReader();
    reader->setCumulative(false);
    reader->setParsePreAndPostFiles(false);
    reader->readProFile(mkspecPath() + "/qmake.conf");
    m_qmakeCXX = reader->value("QMAKE_CXX");

    delete reader;
    m_qmakeCXXUpToDate = true;
}

ProjectExplorer::ToolChain *QtVersion::createToolChain(ProjectExplorer::ToolChain::ToolChainType type) const
{
    ProjectExplorer::ToolChain *tempToolchain = 0;
    if (type == ProjectExplorer::ToolChain::MinGW) {
        QString qmake_cxx = qmakeCXX();
        ProjectExplorer::Environment env = ProjectExplorer::Environment::systemEnvironment();
        //addToEnvironment(env);
        env.prependOrSetPath(mingwDirectory()+"/bin");
        qmake_cxx = env.searchInPath(qmake_cxx);
        tempToolchain = ProjectExplorer::ToolChain::createMinGWToolChain(qmake_cxx, mingwDirectory());
        //qDebug()<<"Mingw ToolChain";
    } else if(type == ProjectExplorer::ToolChain::MSVC) {
        tempToolchain = ProjectExplorer::ToolChain::createMSVCToolChain(msvcVersion(), isQt64Bit());
        //qDebug()<<"MSVC ToolChain ("<<version->msvcVersion()<<")";
    } else if(type == ProjectExplorer::ToolChain::WINCE) {
        tempToolchain = ProjectExplorer::ToolChain::createWinCEToolChain(msvcVersion(), wincePlatform());
        //qDebug()<<"WinCE ToolChain ("<<version->msvcVersion()<<","<<version->wincePlatform()<<")";
    } else if(type == ProjectExplorer::ToolChain::GCC || type == ProjectExplorer::ToolChain::LinuxICC) {
        QString qmake_cxx = qmakeCXX();
        ProjectExplorer::Environment env = ProjectExplorer::Environment::systemEnvironment();
        //addToEnvironment(env);
        qmake_cxx = env.searchInPath(qmake_cxx);
        if (qmake_cxx.isEmpty()) {
            // macx-xcode mkspec resets the value of QMAKE_CXX.
            // Unfortunately, we need a valid QMAKE_CXX to configure the parser.
            qmake_cxx = QLatin1String("cc");
        }
        tempToolchain = ProjectExplorer::ToolChain::createGccToolChain(qmake_cxx);
        //qDebug()<<"GCC ToolChain ("<<qmake_cxx<<")";
#ifdef QTCREATOR_WITH_S60
    } else if (type == ProjectExplorer::ToolChain::WINSCW) {
        tempToolchain = S60Manager::instance()->createWINSCWToolChain(this);
    } else if (type == ProjectExplorer::ToolChain::GCCE) {
        tempToolchain = S60Manager::instance()->createGCCEToolChain(this);
    } else if (type == ProjectExplorer::ToolChain::RVCT_ARMV5
               || type == ProjectExplorer::ToolChain::RVCT_ARMV6) {
        tempToolchain = S60Manager::instance()->createRVCTToolChain(this, type);
#endif
    } else {
        qDebug()<<"Could not create ToolChain for"<<mkspec();
        qDebug()<<"Qt Creator doesn't know about the system includes, nor the systems defines.";
    }
    return tempToolchain;
}


QString QtVersion::findQtBinary(const QStringList &possibleCommands) const
{
    const QString qtdirbin = versionInfo().value(QLatin1String("QT_INSTALL_BINS")) + QLatin1Char('/');
    foreach (const QString &possibleCommand, possibleCommands) {
        const QString fullPath = qtdirbin + possibleCommand;
        if (QFileInfo(fullPath).isFile())
            return QDir::cleanPath(fullPath);
    }
    return QString::null;
}

QString QtVersion::uicCommand() const
{
    if (!isValid())
        return QString::null;
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
        return QString::null;
    if (m_designerCommand.isNull())
        m_designerCommand = findQtBinary(possibleGuiBinaries(QLatin1String("designer")));
    return m_designerCommand;
}

QString QtVersion::linguistCommand() const
{
    if (!isValid())
        return QString::null;
    if (m_linguistCommand.isNull())
        m_linguistCommand = findQtBinary(possibleGuiBinaries(QLatin1String("linguist")));
    return m_linguistCommand;
}

QList<ProjectExplorer::ToolChain::ToolChainType> QtVersion::possibleToolChainTypes() const
{
    QList<ProjectExplorer::ToolChain::ToolChainType> toolChains;
    if (!isValid())
        return toolChains << ProjectExplorer::ToolChain::INVALID;
    const QString &spec = mkspec();
    if (spec.contains("win32-msvc") || spec.contains(QLatin1String("win32-icc")))
        toolChains << ProjectExplorer::ToolChain::MSVC;
    else if (spec.contains("win32-g++"))
        toolChains << ProjectExplorer::ToolChain::MinGW;
    else if (spec == QString::null)
        toolChains << ProjectExplorer::ToolChain::INVALID;
    else if (spec.contains("wince"))
        toolChains << ProjectExplorer::ToolChain::WINCE;
    else if (spec.contains("linux-icc"))
        toolChains << ProjectExplorer::ToolChain::LinuxICC;
#ifdef QTCREATOR_WITH_S60
    else if (spec.contains("symbian-abld"))
        toolChains << ProjectExplorer::ToolChain::GCCE
                << ProjectExplorer::ToolChain::RVCT_ARMV5
                << ProjectExplorer::ToolChain::RVCT_ARMV6
                << ProjectExplorer::ToolChain::WINSCW;
#endif
    else
        toolChains << ProjectExplorer::ToolChain::GCC;
    return toolChains;
}

ProjectExplorer::ToolChain::ToolChainType QtVersion::defaultToolchainType() const
{
    return possibleToolChainTypes().at(0);
}

#ifdef QTCREATOR_WITH_S60
QString QtVersion::mwcDirectory() const
{
    return m_mwcDirectory;
}

void QtVersion::setMwcDirectory(const QString &directory)
{
    m_mwcDirectory = directory;
}
QString QtVersion::s60SDKDirectory() const
{
    return m_s60SDKDirectory;
}

void QtVersion::setS60SDKDirectory(const QString &directory)
{
    m_s60SDKDirectory = directory;
}
#endif

QString QtVersion::mingwDirectory() const
{
    return m_mingwDirectory;
}

void QtVersion::setMingwDirectory(const QString &directory)
{
    m_mingwDirectory = directory;
}

QString QtVersion::msvcVersion() const
{
    return m_msvcVersion;
}

QString QtVersion::wincePlatform() const
{
//    qDebug()<<"QtVersion::wincePlatform returning"<<ProjectExplorer::CeSdkHandler::platformName(mkspecPath() + "/qmake.conf");
    return ProjectExplorer::CeSdkHandler::platformName(mkspecPath() + "/qmake.conf");
}

void QtVersion::setMsvcVersion(const QString &version)
{
    m_msvcVersion = version;
}

void QtVersion::addToEnvironment(ProjectExplorer::Environment &env) const
{
    env.set("QTDIR", versionInfo().value("QT_INSTALL_DATA"));
    QString qtdirbin = versionInfo().value("QT_INSTALL_BINS");
    env.prependOrSetPath(qtdirbin);
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
    return (!(m_id == -1 || m_qmakeCommand == QString::null || m_name == QString::null || mkspec() == QString::null) && !m_notInstalled);
}

QtVersion::QmakeBuildConfig QtVersion::defaultBuildConfig() const
{
    updateVersionInfo();
    QtVersion::QmakeBuildConfig result = QtVersion::QmakeBuildConfig(0);
    if (m_defaultConfigIsDebugAndRelease)
        result = QtVersion::BuildAll;
    if (m_defaultConfigIsDebug)
        result = QtVersion::QmakeBuildConfig(result | QtVersion::DebugBuild);
    return result;
}

bool QtVersion::hasDebuggingHelper() const
{
    return m_hasDebuggingHelper;
}

QString QtVersion::debuggingHelperLibrary() const
{
    QString qtInstallData = versionInfo().value("QT_INSTALL_DATA");
    if (qtInstallData.isEmpty())
        return QString::null;
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
        return QString::null;
    ProjectExplorer::Environment env = ProjectExplorer::Environment::systemEnvironment();
    addToEnvironment(env);

    // TODO: the debugging helper doesn't comply to actual tool chain yet
    ProjectExplorer::ToolChain *tc = createToolChain(defaultToolchainType());
    tc->addToEnvironment(env);
    QString output;
    QString directory = DebuggingHelperLibrary::copyDebuggingHelperLibrary(qtInstallData, &output);
    if (!directory.isEmpty())
        output += DebuggingHelperLibrary::buildDebuggingHelperLibrary(directory, tc->makeCommand(), qmakeCommand(), mkspec(), env);
    m_hasDebuggingHelper = !debuggingHelperLibrary().isEmpty();
    delete tc;
    return output;
}

