/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "qtversionmanager.h"

#include "projectexplorerconstants.h"
#include "cesdkhandler.h"

#include "projectexplorer.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <help/helpplugin.h>
#include <utils/qtcassert.h>

#include <QtCore/QProcess>
#include <QtCore/QSettings>
#include <QtCore/QTime>
#include <QtGui/QApplication>
#include <QtGui/QDesktopServices>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

static const char *QtVersionsSectionName = "QtVersions";
static const char *defaultQtVersionKey = "DefaultQtVersion";
static const char *newQtVersionsKey = "NewQtVersions";



QtVersionManager::QtVersionManager()
    : m_emptyVersion(new QtVersion)
{
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
        else if (id > m_idcount)
            m_idcount = id;
        QtVersion *version = new QtVersion(s->value("Name").toString(),
                                           s->value("Path").toString(),
                                           id,
                                           s->value("IsSystemVersion", false).toBool());
        version->setMingwDirectory(s->value("MingwDirectory").toString());
        version->setMsvcVersion(s->value("msvcVersion").toString());
        m_versions.append(version);
    }
    s->endArray();
    updateUniqueIdToIndexMap();

    ++m_idcount;
    addNewVersionsFromInstaller();
    updateSystemVersion();

    writeVersionsIntoSettings();
    updateDocumentation();
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
    return ProjectExplorerPlugin::instance()->qtVersionManager();
}

void QtVersionManager::addVersion(QtVersion *version)
{
    m_versions.append(version);
    emit qtVersionsChanged();
    writeVersionsIntoSettings();
}

void QtVersionManager::updateDocumentation()
{
    Help::HelpManager *helpManager
        = ExtensionSystem::PluginManager::instance()->getObject<Help::HelpManager>();
    Q_ASSERT(helpManager);
    QStringList fileEndings = QStringList() << "/qch/qt.qch" << "/qch/qmake.qch" << "/qch/designer.qch";
    QStringList files;
    foreach (QtVersion *version, m_versions) {
        QString docPath = version->versionInfo().value("QT_INSTALL_DOCS");
        foreach (const QString &fileEnding, fileEndings)
            files << docPath+fileEnding;
    }
    helpManager->registerDocumentation(files);
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
        s->setArrayIndex(i);
        s->setValue("Name", m_versions.at(i)->name());
        s->setValue("Path", m_versions.at(i)->path());
        s->setValue("Id", m_versions.at(i)->uniqueId());
        s->setValue("MingwDirectory", m_versions.at(i)->mingwDirectory());
        s->setValue("msvcVersion", m_versions.at(i)->msvcVersion());
        s->setValue("IsSystemVersion", m_versions.at(i)->isSystemVersion());
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
    // NewQtVersions="qt 4.3.2=c:\\qt\\qt432;qt embedded=c:\\qtembedded;"
    // or NewQtVersions="qt 4.3.2=c:\\qt\\qt432=c:\\qtcreator\\mingw\\=prependToPath;
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
            if (QDir(newVersionData[1]).exists()) {
                QtVersion *version = new QtVersion(newVersionData[0], newVersionData[1], m_idcount++ );
                if (newVersionData.count() >= 3)
                    version->setMingwDirectory(newVersionData[2]);

                bool versionWasAlreadyInList = false;
                foreach(const QtVersion * const it, m_versions) {
                    if (QDir(version->path()).canonicalPath() == QDir(it->path()).canonicalPath()) {
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
    foreach (QtVersion *version, m_versions) {
        if (version->isSystemVersion()) {
            version->setPath(findSystemQt());
            version->setName(tr("Auto-detected Qt"));
            haveSystemVersion = true;
        }
    }
    if (haveSystemVersion)
        return;
    QtVersion *version = new QtVersion(tr("Auto-detected Qt"),
                                       findSystemQt(),
                                       getUniqueId(),
                                       true);
    m_versions.prepend(version);
    updateUniqueIdToIndexMap();
    if (m_versions.size() > 1) // we had other versions before adding system version
        ++m_defaultVersion;
}

QStringList QtVersionManager::possibleQMakeCommands()
{
    // On windows noone has renamed qmake, right?
#ifdef Q_OS_WIN
    return QStringList() << "qmake.exe";
#endif
    // On unix some distributions renamed qmake to avoid clashes
    QStringList result;
    result << "qmake-qt4" << "qmake4" << "qmake";
    return result;
}

QString QtVersionManager::qtVersionForQMake(const QString &qmakePath)
{
    QProcess qmake;
    qmake.start(qmakePath, QStringList()<<"--version");
    if (!qmake.waitForFinished())
        return false;
    QString output = qmake.readAllStandardOutput();
    QRegExp regexp("(QMake version|Qmake version:)[\\s]*([\\d.]*)");
    regexp.indexIn(output);
    if (regexp.cap(2).startsWith("2.")) {
        QRegExp regexp2("Using Qt version[\\s]*([\\d\\.]*)");
        regexp2.indexIn(output);
        return regexp2.cap(1);
    }
    return QString();
}

QString QtVersionManager::findSystemQt() const
{
    Environment env = Environment::systemEnvironment();
    QStringList paths = env.path();
    foreach (const QString &path, paths) {
        foreach (const QString &possibleCommand, possibleQMakeCommands()) {
            QFileInfo qmake(path + "/" + possibleCommand);
            if (qmake.exists()) {
                if (!qtVersionForQMake(qmake.absoluteFilePath()).isNull()) {
                    QDir dir(qmake.absoluteDir());
                    dir.cdUp();
                    return dir.absolutePath();
                }
            }
        }
    }
    return tr("<not found>");
}

QtVersion *QtVersionManager::currentQtVersion() const
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
            if (m_versions.at(i)->path() != newVersions.at(i)->path()) {
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
    if (emitDefaultChanged)
        emit defaultQtVersionChanged();

    writeVersionsIntoSettings();
}



///
/// QtVersion
///

QtVersion::QtVersion(const QString &name, const QString &path, int id, bool isSystemVersion)
    : m_name(name),
    m_isSystemVersion(isSystemVersion),
    m_notInstalled(false),
    m_defaultConfigIsDebug(true),
    m_defaultConfigIsDebugAndRelease(true),
    m_hasDebuggingHelper(false)
{
    setPath(path);
    if (id == -1)
        m_id = getUniqueId();
    else
        m_id = id;
}

QtVersion::QtVersion(const QString &name, const QString &path)
    : m_name(name),
    m_versionInfoUpToDate(false),
    m_mkspecUpToDate(false),
    m_isSystemVersion(false),
    m_hasDebuggingHelper(false)
{
    setPath(path);
    m_id = getUniqueId();
}

QString QtVersion::name() const
{
    return m_name;
}

QString QtVersion::path() const
{
    return m_path;
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
    qmakeCommand();
    return m_qtVersionString;
}

QHash<QString,QString> QtVersion::versionInfo() const
{
    updateVersionInfo();
    return m_versionInfo;
}

void QtVersion::setName(const QString &name)
{
    m_name = name;
}

void QtVersion::setPath(const QString &path)
{
    m_path = QDir::cleanPath(path);
    updateSourcePath();
    m_versionInfoUpToDate = false;
    m_mkspecUpToDate = false;
    m_qmakeCommand = QString::null;
// TODO do i need to optimize this?
    m_hasDebuggingHelper = !dumperLibrary().isEmpty();
}

QString QtVersion::dumperLibrary() const
{
    uint hash = qHash(path());
    QString qtInstallData = versionInfo().value("QT_INSTALL_DATA");
    if (qtInstallData.isEmpty())
        qtInstallData = path();
    QStringList directories;
    directories
            << (qtInstallData + "/qtc-debugging-helper/")
            << (QApplication::applicationDirPath() + "/../qtc-debugging-helper/" + QString::number(hash)) + "/"
            << (QDesktopServices::storageLocation(QDesktopServices::DataLocation) + "/qtc-debugging-helper/" + QString::number(hash)) + "/";
    foreach(const QString &directory, directories) {
#if defined(Q_OS_WIN)
        QFileInfo fi(directory + "debug/gdbmacros.dll");
#elif defined(Q_OS_MAC)
        QFileInfo fi(directory + "libgdbmacros.dylib");
#else // generic UNIX
        QFileInfo fi(directory + "libgdbmacros.so");
#endif
        if (fi.exists())
            return fi.filePath();
    }
    return QString();
}

void QtVersion::updateSourcePath()
{
    m_sourcePath = m_path;
    QFile qmakeCache(m_path + QLatin1String("/.qmake.cache"));
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
}

// Returns the version that was used to build the project in that directory
// That is returns the directory
// To find out wheter we already have a qtversion for that directory call
// QtVersion *QtVersionManager::qtVersionForDirectory(const QString directory);
QString QtVersionManager::findQtVersionFromMakefile(const QString &directory)
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
                QFileInfo binDir(qmake.absolutePath());
                QString qtDir = binDir.absolutePath();
                if (debugAdding)
                    qDebug() << "#~~ QtDir:"<<qtDir;
                return qtDir;
            }
        }
        makefile.close();
    }
    return QString::null;
}

QtVersion *QtVersionManager::qtVersionForDirectory(const QString &directory)
{
   foreach(QtVersion *v, versions()) {
        if (v->path() == directory) {
            return v;
            break;
        }
    }
   return 0;
}

QtVersion::QmakeBuildConfig QtVersionManager::scanMakefileForQmakeConfig(const QString &directory, QtVersion::QmakeBuildConfig defaultBuildConfig)
{
    bool debugScan = false;
    QtVersion::QmakeBuildConfig result = QtVersion::NoBuild;
    QFile makefile(directory + "/Makefile" );
    if (makefile.exists() && makefile.open(QFile::ReadOnly)) {
        QTextStream ts(&makefile);
        while (!ts.atEnd()) {
            QString line = ts.readLine();
            if (line.startsWith("# Command:")) {
                // if nothing is specified
                result = defaultBuildConfig;

                // Actually parsing that line is not trivial in the general case
                // There might things like this
                // # Command: /home/dteske/git/bqt-45/bin/qmake -unix CONFIG+=debug\ release CONFIG\ +=\ debug_and_release\ debug -o Makefile test.pro
                // which sets debug_and_release and debug
                // or something like this:
                //[...] CONFIG+=debug\ release CONFIG\ +=\ debug_and_release\ debug CONFIG\ -=\ debug_and_release CONFIG\ -=\ debug -o Makefile test.pro
                // which sets -build_all and release

                // To parse that, we search for the first CONFIG, then look for " " which is not after a "\" or the end
                // And then look at each config individually
                // we then remove all "\ " with just " "
                // += sets adding flags
                // -= sets removing flags
                // and then split the string after the =
                // and go over each item separetly
                // debug sets/removes the flag DebugBuild
                // release removes/sets the flag DebugBuild
                // debug_and_release sets/removes the flag BuildAll
                int pos = line.indexOf("CONFIG");
                if (pos != -1) {
                    // Chopped of anything that is not interesting
                    line = line.mid(pos);
                    line = line.trimmed();
                    if (debugScan)
                        qDebug()<<"chopping line :"<<line;

                    //Now chop into parts that are intresting
                    QStringList parts;
                    int lastpos = 0;
                    for (int i = 1; i < line.size(); ++i) {
                        if (line.at(i) == QLatin1Char(' ') && line.at(i-1) != QLatin1Char('\\')) {
                            // found a part
                            parts.append(line.mid(lastpos, i-lastpos));
                            if (debugScan)
                                qDebug()<<"part appended:"<<line.mid(lastpos, i-lastpos);
                            lastpos = i + 1; // Nex one starts after the space
                        }
                    }
                    parts.append(line.mid(lastpos));
                    if (debugScan)
                        qDebug()<<"part appended:"<<line.mid(lastpos);

                    foreach(const QString &part, parts) {
                        if (debugScan)
                            qDebug()<<"now interpreting part"<<part;
                        bool setFlags;
                        // Now try to understand each part for that we do a rather stupid approach, optimize it if you care
                        if (part.startsWith("CONFIG")) {
                            // Yep something interesting
                            if (part.indexOf("+=") != -1) {
                                setFlags = true;
                            } else if (part.indexOf("-=") != -1) {
                                setFlags = false;
                            } else {
                                setFlags = true;
                                if (debugScan)
                                    qDebug()<<"This can never happen, except if we can't parse Makefiles...";
                            }
                            if (debugScan)
                                qDebug()<<"part has setFlags:"<<setFlags;
                            // now loop forward, looking for something that looks like debug, release or debug_and_release

                            for (int i = 0; i < part.size(); ++i) {
                                int left = part.size() - i;
                                if (left >= 17  && QStringRef(&part, i, 17) == "debug_and_release") {
                                        if (setFlags)
                                            result = QtVersion::QmakeBuildConfig(result | QtVersion::BuildAll);
                                        else
                                            result = QtVersion::QmakeBuildConfig(result & ~QtVersion::BuildAll);
                                        if (debugScan)
                                            qDebug()<<"found debug_and_release new value"<<result;
                                        i += 17;
                                } else if (left >=7 && QStringRef(&part, i, 7) ==  "release") {
                                        if (setFlags)
                                            result = QtVersion::QmakeBuildConfig(result & ~QtVersion::DebugBuild);
                                        else
                                            result = QtVersion::QmakeBuildConfig(result | QtVersion::DebugBuild);
                                        if (debugScan)
                                            qDebug()<<"found release new value"<<result;
                                        i += 7;
                                } else if (left >= 5 && QStringRef(&part, i, 5) == "debug") {
                                        if (setFlags)
                                            result = QtVersion::QmakeBuildConfig(result  | QtVersion::DebugBuild);
                                        else
                                            result = QtVersion::QmakeBuildConfig(result  & ~QtVersion::DebugBuild);
                                        if (debugScan)
                                            qDebug()<<"found debug new value"<<result;
                                        i += 5;
                                }
                            }
                        }
                    }
                }
                if (debugScan)
                    qDebug()<<"returning: "<<result;
                if (debugScan)
                    qDebug()<<"buildall = "<<bool(result & QtVersion::BuildAll);
                if (debugScan)
                    qDebug()<<"debug ="<<bool(result & QtVersion::DebugBuild);
            }
        }
        makefile.close();
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
    QFileInfo qmake(qmakeCommand());
    if (qmake.exists()) {
        static const char * const variables[] = {
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

        // Parse qconfigpri
        QString baseDir = m_versionInfo.contains("QT_INSTALL_DATA") ?
                           m_versionInfo.value("QT_INSTALL_DATA") :
                           m_path;
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
//    QFile f(path() + "/.qmake.cache");
//    if (f.exists() && f.open(QIODevice::ReadOnly)) {
//        while (!f.atEnd()) {
//            QByteArray line = f.readLine();
//            if (line.startsWith("QMAKESPEC")) {
//                const QList<QByteArray> &temp = line.split('=');
//                if (temp.size() == 2) {
//                    mkspec = temp.at(1).trimmed();
//                    if (mkspec.startsWith("$$QT_BUILD_TREE/mkspecs/"))
//                        mkspec = mkspec.mid(QString("$$QT_BUILD_TREE/mkspecs/").length());
//                    else if (mkspec.startsWith("$$QT_BUILD_TREE\\mkspecs\\"))
//                        mkspec = mkspec.mid(QString("$$QT_BUILD_TREE\\mkspecs\\").length());
//                    mkspec = QDir::fromNativeSeparators(mkspec);
//                }
//                break;
//            }
//        }
//        f.close();
//    } else {
        // no .qmake.cache so look at the default mkspec
        QString mkspecPath = versionInfo().value("QMAKE_MKSPECS");
        if (mkspecPath.isEmpty())
            mkspecPath = path() + "/mkspecs/default";
        else
            mkspecPath = mkspecPath + "/default";
//        qDebug() << "default mkspec is located at" << mkspecPath;
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
//                            qDebug() << "default mkspec is xcode, falling back to g++";
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
//    }

    m_mkspecFullPath = mkspec;
    int index = mkspec.lastIndexOf('/');
    if (index == -1)
        index = mkspec.lastIndexOf('\\');
    QString mkspecDir = QDir(m_path + "/mkspecs/").canonicalPath();
    if (index >= 0 && QDir(mkspec.left(index)).canonicalPath() == mkspecDir)
        mkspec = mkspec.mid(index+1).trimmed();

    m_mkspec = mkspec;
    m_mkspecUpToDate = true;
//    qDebug()<<"mkspec for "<<m_path<<" is "<<mkspec;
}

QString QtVersion::qmakeCommand() const
{
    // We can't use versionInfo QT_INSTALL_BINS here
    // because that functions calls us to find out the values for versionInfo
    if (!m_qmakeCommand.isNull())
        return m_qmakeCommand;

    QDir qtDir = path() + "/bin/";
    foreach (const QString &possibleCommand, QtVersionManager::possibleQMakeCommands()) {
        QString s = qtDir.absoluteFilePath(possibleCommand);
        QFileInfo qmake(s);
        if (qmake.exists() && qmake.isExecutable()) {
            QString qtVersion = QtVersionManager::qtVersionForQMake(qmake.absoluteFilePath());
            if (!qtVersion.isNull()) {
                m_qtVersionString = qtVersion;
                m_qmakeCommand = qmake.absoluteFilePath();
                return qmake.absoluteFilePath();
            }
        }
    }
    return QString::null;
}

ProjectExplorer::ToolChain::ToolChainType QtVersion::toolchainType() const
{
    if (!isValid())
        return ProjectExplorer::ToolChain::INVALID;
    const QString &spec = mkspec();
//    qDebug()<<"spec="<<spec;
    if (spec.contains("win32-msvc") || spec.contains(QLatin1String("win32-icc")))
        return ProjectExplorer::ToolChain::MSVC;
    else if (spec.contains("win32-g++"))
        return ProjectExplorer::ToolChain::MinGW;
    else if (spec == QString::null)
        return ProjectExplorer::ToolChain::INVALID;
    else if (spec.contains("wince"))
        return ProjectExplorer::ToolChain::WINCE;
    else if (spec.contains("linux-icc"))
        return ProjectExplorer::ToolChain::LinuxICC;
    else
        return ProjectExplorer::ToolChain::GCC;
}

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

void QtVersion::addToEnvironment(Environment &env)
{
    env.set("QTDIR", m_path);
    QString qtdirbin = versionInfo().value("QT_INSTALL_BINS");
    env.prependOrSetPath(qtdirbin);
    // add libdir, includedir and bindir
    // or add Mingw dirs
    // or do nothing on other
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
    return (!(m_id == -1 || m_path == QString::null || m_name == QString::null || mkspec() == QString::null) && !m_notInstalled);
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


// TODO buildDebuggingHelperLibrary needs to be accessible outside of the
// qt4versionmanager
// That probably means moving qt4version management into either the projectexplorer
// (The Projectexplorer plugin probably needs some splitting up, most of the stuff
// could be in a plugin shared by qt4projectmanager, cmakemanager and debugger.)
QString QtVersion::buildDebuggingHelperLibrary()
{
// Locations to try:
//    $QTDIR/qtc-debugging-helper
//    $APPLICATION-DIR/qtc-debugging-helper/$hash
//    $USERDIR/qtc-debugging-helper/$hash

    QString output;
    uint hash = qHash(path());
    QString qtInstallData = versionInfo().value("QT_INSTALL_DATA");
    if (qtInstallData.isEmpty())
        qtInstallData = path();
    QStringList directories;
    directories
            << qtInstallData + "/qtc-debugging-helper/"
            << QApplication::applicationDirPath() + "/../qtc-debugging-helper/" + QString::number(hash) +"/"
            << QDesktopServices::storageLocation (QDesktopServices::DataLocation) + "/qtc-debugging-helper/" + QString::number(hash) +"/";

    QStringList files;
    files << "gdbmacros.cpp" << "gdbmacros.pro"
          << "LICENSE.LGPL" << "LGPL_EXCEPTION.TXT";

    foreach(const QString &directory, directories) {
        QString dumperPath = Core::ICore::instance()->resourcePath() + "/gdbmacros/";
        bool success = true;
        QDir().mkpath(directory);
        foreach (const QString &file, files) {
            QString source = dumperPath + file;
            QString dest = directory + file;
            QFileInfo destInfo(dest);
            if (destInfo.exists()) {
                if (destInfo.lastModified() >= QFileInfo(source).lastModified())
                    continue;
                success &= QFile::remove(dest);
            }
            success &= QFile::copy(source, dest);
        }
        if (!success)
            continue;

        // Setup process
        QProcess proc;

        ProjectExplorer::Environment env = ProjectExplorer::Environment::systemEnvironment();
        addToEnvironment(env);
        // TODO this is a hack to get, to be removed and rewritten for 1.2
        // For MSVC and MINGW, we need a toolchain to get the right environment
        ProjectExplorer::ToolChain *toolChain = 0;
        ProjectExplorer::ToolChain::ToolChainType t = toolchainType();
        if (t == ProjectExplorer::ToolChain::MinGW)
            toolChain = ProjectExplorer::ToolChain::createMinGWToolChain("g++", mingwDirectory());
        else if(t == ProjectExplorer::ToolChain::MSVC)
            toolChain = ProjectExplorer::ToolChain::createMSVCToolChain(msvcVersion());
        if (toolChain) {
            toolChain->addToEnvironment(env);
            delete toolChain;
            toolChain = 0;
        }

        proc.setEnvironment(env.toStringList());
        proc.setWorkingDirectory(directory);
        proc.setProcessChannelMode(QProcess::MergedChannels);

        output += QString("Building debugging helper library in %1\n").arg(directory);
        output += "\n";

        QString make;
        // TODO this is butt ugly
        // only qt4projects have a toolchain() method. (Reason mostly, that in order to create
        // the toolchain, we need to have the path to gcc
        // which might depend on environment settings of the project
        // so we hardcode the toolchainType to make conversation here
        // and think about how to fix that later
        if (t == ProjectExplorer::ToolChain::MinGW)
            make = "mingw32-make.exe";
        else if(t == ProjectExplorer::ToolChain::MSVC || t == ProjectExplorer::ToolChain::WINCE)
            make = "nmake.exe";
        else if (t == ProjectExplorer::ToolChain::GCC || t == ProjectExplorer::ToolChain::LinuxICC)
            make = "make";

        QString makeFullPath = env.searchInPath(make);
        if (!makeFullPath.isEmpty()) {
            output += QString("Running %1 clean...\n").arg(makeFullPath);
            proc.start(makeFullPath, QStringList() << "clean");
            proc.waitForFinished();
            output += proc.readAll();
        } else {
            output += QString("%1 not found in PATH\n").arg(make);
            break;
        }

        output += QString("\nRunning %1 ...\n").arg(qmakeCommand());

        proc.start(qmakeCommand(), QStringList()<<"-spec"<< mkspec() <<"gdbmacros.pro");
        proc.waitForFinished();

        output += proc.readAll();

        output += "\n";
        if (!makeFullPath.isEmpty()) {
            output += QString("Running %1 ...\n").arg(makeFullPath);
            proc.start(makeFullPath, QStringList());
            proc.waitForFinished();
            output += proc.readAll();
        } else {
            output += QString("%1 not found in PATH\n").arg(make);
        }
        break;
    }
    m_hasDebuggingHelper = !dumperLibrary().isEmpty();
    return output;
}
