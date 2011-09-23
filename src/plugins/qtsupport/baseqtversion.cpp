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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "baseqtversion.h"
#include "qmlobservertool.h"
#include "qmldumptool.h"
#include "qmldebugginglibrary.h"

#include "qtversionmanager.h"
#include "profilereader.h"
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/debugginghelper.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchainmanager.h>

#include <utils/persistentsettings.h>
#include <utils/environment.h>
#include <utils/synchronousprocess.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>

using namespace QtSupport;
using namespace QtSupport::Internal;

static const char QTVERSIONID[] = "Id";
static const char QTVERSIONNAME[] = "Name";
static const char QTVERSIONAUTODETECTED[] = "isAutodetected";
static const char QTVERSIONAUTODETECTIONSOURCE []= "autodetectionSource";
static const char QTVERSIONQMAKEPATH[] = "QMakePath";

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

///////////////
// QtConfigWidget
///////////////
QtConfigWidget::QtConfigWidget()
{

}

///////////////
// BaseQtVersion
///////////////
int BaseQtVersion::getUniqueId()
{
    return QtVersionManager::instance()->getUniqueId();
}

BaseQtVersion::BaseQtVersion(const QString &qmakeCommand, bool isAutodetected, const QString &autodetectionSource)
    : m_id(getUniqueId()),
      m_isAutodetected(isAutodetected),
      m_autodetectionSource(autodetectionSource),
      m_hasDebuggingHelper(false),
      m_hasQmlDump(false),
      m_hasQmlDebuggingLibrary(false),
      m_hasQmlObserver(false),
      m_mkspecUpToDate(false),
      m_mkspecReadUpToDate(false),
      m_defaultConfigIsDebug(true),
      m_defaultConfigIsDebugAndRelease(true),
      m_versionInfoUpToDate(false),
      m_installed(true),
      m_hasExamples(false),
      m_hasDemos(false),
      m_hasDocumentation(false),
      m_qmakeIsExecutable(true)
{
    ctor(qmakeCommand);
    setDisplayName(defaultDisplayName(qtVersionString(), qmakeCommand, false));
}

BaseQtVersion::BaseQtVersion()
    :  m_id(-1), m_isAutodetected(false),
    m_hasDebuggingHelper(false),
    m_hasQmlDump(false),
    m_hasQmlDebuggingLibrary(false),
    m_hasQmlObserver(false),
    m_mkspecUpToDate(false),
    m_mkspecReadUpToDate(false),
    m_defaultConfigIsDebug(true),
    m_defaultConfigIsDebugAndRelease(true),
    m_versionInfoUpToDate(false),
    m_installed(true),
    m_hasExamples(false),
    m_hasDemos(false),
    m_hasDocumentation(false),
    m_qmakeIsExecutable(true)
{
    ctor(QString());
}

void BaseQtVersion::ctor(const QString& qmakePath)
{
    m_qmakeCommand = QDir::fromNativeSeparators(qmakePath);
#ifdef Q_OS_WIN
    m_qmakeCommand = m_qmakeCommand.toLower();
#endif
    m_designerCommand.clear();
    m_linguistCommand.clear();
    m_qmlviewerCommand.clear();
    m_uicCommand.clear();
    m_mkspecUpToDate = false;
    m_mkspecReadUpToDate = false;
    m_versionInfoUpToDate = false;
    m_qtVersionString.clear();
    m_sourcePath.clear();
}

BaseQtVersion::~BaseQtVersion()
{
}

QString BaseQtVersion::defaultDisplayName(const QString &versionString, const QString &qmakePath,
                                          bool fromPath)
{
    QString location;
    if (qmakePath.isEmpty()) {
        location = QCoreApplication::translate("QtVersion", "<unknown>");
    } else {
        // Deduce a description from '/foo/qt-folder/[qtbase]/bin/qmake' -> '/foo/qt-folder'.
        // '/usr' indicates System Qt 4.X on Linux.
        QDir dir = QFileInfo(qmakePath).absoluteDir();
        do {
            const QString dirName = dir.dirName();
            if (dirName == QLatin1String("usr")) { // System-installed Qt.
                location = QCoreApplication::translate("QtVersion", "System");
                break;
            }
            if (dirName.compare(QLatin1String("bin"), Qt::CaseInsensitive)
                && dirName.compare(QLatin1String("qtbase"), Qt::CaseInsensitive)) {
                location = dirName;
                break;
            }
        } while (dir.cdUp());
    }

    return fromPath ?
        QCoreApplication::translate("QtVersion", "Qt %1 in PATH (%2)").arg(versionString, location) :
        QCoreApplication::translate("QtVersion", "Qt %1 (%2)").arg(versionString, location);
}

void BaseQtVersion::setId(int id)
{
    m_id = id;
}

void BaseQtVersion::restoreLegacySettings(QSettings *s)
{
    Q_UNUSED(s);
}

void BaseQtVersion::fromMap(const QVariantMap &map)
{
    m_id = map.value(QLatin1String(QTVERSIONID)).toInt();
    if (m_id == -1) // this happens on adding from installer, see updateFromInstaller => get a new unique id
        m_id = QtVersionManager::instance()->getUniqueId();
    m_displayName = map.value(QLatin1String(QTVERSIONNAME)).toString();
    m_isAutodetected = map.value(QLatin1String(QTVERSIONAUTODETECTED)).toBool();
    if (m_isAutodetected)
        m_autodetectionSource = map.value(QLatin1String(QTVERSIONAUTODETECTIONSOURCE)).toString();
    ctor(map.value(QLatin1String(QTVERSIONQMAKEPATH)).toString());
}

QVariantMap BaseQtVersion::toMap() const
{
    QVariantMap result;
    result.insert(QLatin1String(QTVERSIONID), uniqueId());
    result.insert(QLatin1String(QTVERSIONNAME), displayName());
    result.insert(QLatin1String(QTVERSIONAUTODETECTED), isAutodetected());
    if (isAutodetected())
        result.insert(QLatin1String(QTVERSIONAUTODETECTIONSOURCE), autodetectionSource());
    result.insert(QLatin1String(QTVERSIONQMAKEPATH), qmakeCommand());
    return result;
}

bool BaseQtVersion::isValid() const
{
    if (uniqueId() == -1 || displayName().isEmpty())
        return false;
    updateVersionInfo();
    updateMkspec();

    return  !qmakeCommand().isEmpty()
            && m_installed
            && m_versionInfo.contains("QT_INSTALL_BINS")
            && !m_mkspecFullPath.isEmpty()
            && m_qmakeIsExecutable;
}

QString BaseQtVersion::invalidReason() const
{
    if (displayName().isEmpty())
        return QCoreApplication::translate("QtVersion", "Qt version has no name");
    if (qmakeCommand().isEmpty())
        return QCoreApplication::translate("QtVersion", "No qmake path set");
    if (!m_qmakeIsExecutable)
        return QCoreApplication::translate("QtVersion", "qmake does not exist or is not executable");
    if (!m_installed)
        return QCoreApplication::translate("QtVersion", "Qt version is not properly installed, please run make install");
    if (!m_versionInfo.contains("QT_INSTALL_BINS"))
        return QCoreApplication::translate("QtVersion",
                                           "Could not determine the path to the binaries of the Qt installation, maybe the qmake path is wrong?");
    if (m_mkspecUpToDate && m_mkspecFullPath.isEmpty())
        return QCoreApplication::translate("QtVersion", "The default mkspec symlink is broken.");
    return QString();
}

QString BaseQtVersion::warningReason() const
{
    return QString();
}

QString BaseQtVersion::qmakeCommand() const
{
    return m_qmakeCommand;
}

bool BaseQtVersion::toolChainAvailable(const QString &id) const
{
    Q_UNUSED(id)
    if (!isValid())
        return false;
    foreach (const ProjectExplorer::Abi &abi, qtAbis())
        if (!ProjectExplorer::ToolChainManager::instance()->findToolChains(abi).isEmpty())
            return true;
    return false;
}

QList<ProjectExplorer::Abi> BaseQtVersion::qtAbis() const
{
    if (m_qtAbis.isEmpty())
        m_qtAbis = detectQtAbis();
    if (m_qtAbis.isEmpty())
        m_qtAbis.append(ProjectExplorer::Abi()); // add empty ABI by default: This is compatible with all TCs.
    return m_qtAbis;
}

bool BaseQtVersion::equals(BaseQtVersion *other)
{
    if (type() != other->type())
        return false;
    if (uniqueId() != other->uniqueId())
        return false;
    if (displayName() != other->displayName())
        return false;

    return true;
}

int BaseQtVersion::uniqueId() const
{
    return m_id;
}

bool BaseQtVersion::isAutodetected() const
{
    return m_isAutodetected;
}

QString BaseQtVersion::autodetectionSource() const
{
    return m_autodetectionSource;
}

void BaseQtVersion::setAutoDetectionSource(const QString &autodetectionSource)
{
    m_autodetectionSource = autodetectionSource;
}

QString BaseQtVersion::displayName() const
{
    return m_displayName;
}

void BaseQtVersion::setDisplayName(const QString &name)
{
    m_displayName = name;
}

QString BaseQtVersion::toHtml(bool verbose) const
{
    QString rc;
    QTextStream str(&rc);
    str << "<html><body><table>";
    str << "<tr><td><b>" << QCoreApplication::translate("BaseQtVersion", "Name:")
        << "</b></td><td>" << displayName() << "</td></tr>";
    if (!isValid()) {
        str << "<tr><td colspan=2><b>" + QCoreApplication::translate("BaseQtVersion", "Invalid Qt version") +"</b></td></tr>";
    } else {
        QString prefix = QLatin1String("<tr><td><b>") + QCoreApplication::translate("BaseQtVersion", "ABI:") + QLatin1String("</b></td>");
        foreach (const ProjectExplorer::Abi &abi, qtAbis()) {
            str << prefix << "<td>" << abi.toString() << "</td></tr>";
            prefix = QLatin1String("<tr><td></td>");
        }
        str << "<tr><td><b>" << QCoreApplication::translate("BaseQtVersion", "Source:")
            << "</b></td><td>" << sourcePath() << "</td></tr>";
        str << "<tr><td><b>" << QCoreApplication::translate("BaseQtVersion", "mkspec:")
            << "</b></td><td>" << mkspec() << "</td></tr>";
        str << "<tr><td><b>" << QCoreApplication::translate("BaseQtVersion", "qmake:")
            << "</b></td><td>" << m_qmakeCommand << "</td></tr>";
        ensureMkSpecParsed();
        if (!mkspecPath().isEmpty()) {
            if (m_defaultConfigIsDebug || m_defaultConfigIsDebugAndRelease) {
                str << "<tr><td><b>" << QCoreApplication::translate("BaseQtVersion", "Default:") << "</b></td><td>"
                    << (m_defaultConfigIsDebug ? "debug" : "release");
                if (m_defaultConfigIsDebugAndRelease)
                    str << " debug_and_release";
                str << "</td></tr>";
            } // default config.
        }
        str << "<tr><td><b>" << QCoreApplication::translate("BaseQtVersion", "Version:")
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

void BaseQtVersion::updateSourcePath() const
{
    if (!m_sourcePath.isEmpty())
        return;
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

QString BaseQtVersion::sourcePath() const
{
    updateSourcePath();
    return m_sourcePath;
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

QString BaseQtVersion::designerCommand() const
{
    if (!isValid())
        return QString();
    if (m_designerCommand.isNull())
        m_designerCommand = findQtBinary(Designer);
    return m_designerCommand;
}

QString BaseQtVersion::linguistCommand() const
{
    if (!isValid())
        return QString();
    if (m_linguistCommand.isNull())
        m_linguistCommand = findQtBinary(Linguist);
    return m_linguistCommand;
}

QString BaseQtVersion::qmlviewerCommand() const
{
    if (!isValid())
        return QString();

    if (m_qmlviewerCommand.isNull())
        m_qmlviewerCommand = findQtBinary(QmlViewer);
    return m_qmlviewerCommand;
}

QString BaseQtVersion::findQtBinary(BINARIES binary) const
{
    QString baseDir;
    if (qtVersion() < QtVersionNumber(5, 0, 0)) {
        baseDir = versionInfo().value(QLatin1String("QT_INSTALL_BINS"));
    } else {
        ensureMkSpecParsed();
        switch (binary) {
        case QmlViewer:
            baseDir = m_mkspecValues.value("QT.declarative.bins");
            break;
        case Designer:
        case Linguist:
            baseDir = m_mkspecValues.value("QT.designer.bins");
            break;
        case Uic:
            baseDir = versionInfo().value(QLatin1String("QT_INSTALL_BINS"));
            break;
        default:
            // Can't happen
            Q_ASSERT(false);
        }
    }

    if (baseDir.isEmpty())
        return QString();
    if (!baseDir.endsWith('/'))
        baseDir += QLatin1Char('/');

    QStringList possibleCommands;
    switch (binary) {
    case QmlViewer: {
        if (qtVersion() < QtVersionNumber(5, 0, 0)) {
            possibleCommands << possibleGuiBinaries(QLatin1String("qmlviewer"));
        } else {
#if defined(Q_OS_WIN)
            possibleCommands << QLatin1String("qmlscene.exe");
#else
            possibleCommands << QLatin1String("qmlscene");
#endif
        }
    }
        break;
    case Designer:
        possibleCommands << possibleGuiBinaries(QLatin1String("designer"));
        break;
    case Linguist:
        possibleCommands << possibleGuiBinaries(QLatin1String("linguist"));
        break;
    case Uic:
#ifdef Q_OS_WIN
        possibleCommands << QLatin1String("uic.exe");
#else
        possibleCommands << QLatin1String("uic-qt4") << QLatin1String("uic4") << QLatin1String("uic");
#endif
        break;
    default:
        Q_ASSERT(false);
    }
    foreach (const QString &possibleCommand, possibleCommands) {
        const QString fullPath = baseDir + possibleCommand;
        if (QFileInfo(fullPath).isFile())
            return QDir::cleanPath(fullPath);
    }
    return QString();
}

QString BaseQtVersion::uicCommand() const
{
    if (!isValid())
        return QString();
    if (!m_uicCommand.isNull())
        return m_uicCommand;
    m_uicCommand = findQtBinary(Uic);
    return m_uicCommand;
}

QString BaseQtVersion::systemRoot() const
{
    return QString();
}

void BaseQtVersion::updateMkspec() const
{
    if (uniqueId() == -1 || m_mkspecUpToDate)
        return;

    m_mkspecUpToDate = true;
    m_mkspecFullPath = mkspecFromVersionInfo(versionInfo());

    m_mkspec = m_mkspecFullPath;
    if (m_mkspecFullPath.isEmpty())
        return;

    QString baseMkspecDir = versionInfo().value("QMAKE_MKSPECS");
    if (baseMkspecDir.isEmpty())
        baseMkspecDir = versionInfo().value("QT_INSTALL_DATA") + "/mkspecs";

#ifdef Q_OS_WIN
    baseMkspecDir = baseMkspecDir.toLower();
#endif

    if (m_mkspec.startsWith(baseMkspecDir)) {
        m_mkspec = m_mkspec.mid(baseMkspecDir.length() + 1);
//        qDebug() << "Setting mkspec to"<<mkspec;
    } else {
        QString sourceMkSpecPath = sourcePath() + "/mkspecs";
        if (m_mkspec.startsWith(sourceMkSpecPath)) {
            m_mkspec = m_mkspec.mid(sourceMkSpecPath.length() + 1);
        } else {
            // Do nothing
        }
    }
}

void BaseQtVersion::ensureMkSpecParsed() const
{
    if (m_mkspecReadUpToDate)
        return;
    m_mkspecReadUpToDate = true;

    if (mkspecPath().isEmpty())
        return;

    ProFileOption option;
    option.properties = versionInfo();
    ProMessageHandler msgHandler(true);
    ProFileCacheManager::instance()->incRefCount();
    ProFileParser parser(ProFileCacheManager::instance()->cache(), &msgHandler);
    ProFileEvaluator evaluator(&option, &parser, &msgHandler);
    if (ProFile *pro = parser.parsedProFile(mkspecPath() + "/qmake.conf")) {
        evaluator.setCumulative(false);
        evaluator.accept(pro, ProFileEvaluator::LoadProOnly);
        pro->deref();
    }

    parseMkSpec(&evaluator);

    ProFileCacheManager::instance()->decRefCount();
}

void BaseQtVersion::parseMkSpec(ProFileEvaluator *evaluator) const
{
    QStringList configValues = evaluator->values("CONFIG");
    m_defaultConfigIsDebugAndRelease = false;
    foreach (const QString &value, configValues) {
        if (value == "debug")
            m_defaultConfigIsDebug = true;
        else if (value == "release")
            m_defaultConfigIsDebug = false;
        else if (value == "build_all")
            m_defaultConfigIsDebugAndRelease = true;
    }

    m_mkspecValues.insert("QT.designer.bins", evaluator->value("QT.designer.bins"));
    m_mkspecValues.insert("QT.declarative.bins", evaluator->value("QT.declarative.bins"));
}

QString BaseQtVersion::mkspec() const
{
    updateMkspec();
    return m_mkspec;
}

QString BaseQtVersion::mkspecPath() const
{
    updateMkspec();
    return m_mkspecFullPath;
}

bool BaseQtVersion::hasMkspec(const QString &spec) const
{
    updateVersionInfo();
    QFileInfo fi;
    fi.setFile(QDir::fromNativeSeparators(m_versionInfo.value("QMAKE_MKSPECS")) + '/' + spec);
    if (fi.isDir())
        return true;
    fi.setFile(sourcePath() + QLatin1String("/mkspecs/") + spec);
    return fi.isDir();
}

BaseQtVersion::QmakeBuildConfigs BaseQtVersion::defaultBuildConfig() const
{
    ensureMkSpecParsed();
    BaseQtVersion::QmakeBuildConfigs result = BaseQtVersion::QmakeBuildConfig(0);

    if (m_defaultConfigIsDebugAndRelease)
        result = BaseQtVersion::BuildAll;
    if (m_defaultConfigIsDebug)
        result = result | BaseQtVersion::DebugBuild;
    return result;
}

QString BaseQtVersion::qtVersionString() const
{
    if (!m_qtVersionString.isNull())
        return m_qtVersionString;
    m_qtVersionString.clear();
    if (m_qmakeIsExecutable) {
        const QString qmake = QFileInfo(qmakeCommand()).absoluteFilePath();
        m_qtVersionString =
            ProjectExplorer::DebuggingHelperLibrary::qtVersionForQMake(qmake, &m_qmakeIsExecutable);
    } else {
        qWarning("Cannot determine the Qt version: %s cannot be run.", qPrintable(qmakeCommand()));
    }
    return m_qtVersionString;
}

QtVersionNumber BaseQtVersion::qtVersion() const
{
    return QtVersionNumber(qtVersionString());
}

void BaseQtVersion::updateVersionInfo() const
{
    if (m_versionInfoUpToDate)
        return;
    if (!m_qmakeIsExecutable) {
        qWarning("Cannot update Qt version information: %s cannot be run.",
                 qPrintable(qmakeCommand()));
        return;
    }

    // extract data from qmake executable
    m_versionInfo.clear();
    m_installed = true;
    m_hasExamples = false;
    m_hasDocumentation = false;
    m_hasDebuggingHelper = false;
    m_hasQmlDump = false;
    m_hasQmlDebuggingLibrary = false;
    m_hasQmlObserver = false;

    if (!queryQMakeVariables(qmakeCommand(), &m_versionInfo, &m_qmakeIsExecutable))
        return;

    if (m_versionInfo.contains("QT_INSTALL_DATA")) {
        QString qtInstallData = m_versionInfo.value("QT_INSTALL_DATA");
        QString qtHeaderData = m_versionInfo.value("QT_INSTALL_HEADERS");
        m_versionInfo.insert("QMAKE_MKSPECS", QDir::cleanPath(qtInstallData+"/mkspecs"));

        if (!qtInstallData.isEmpty()) {
            m_hasDebuggingHelper = !ProjectExplorer::DebuggingHelperLibrary::debuggingHelperLibraryByInstallData(qtInstallData).isEmpty();
            m_hasQmlDump
                    = !QmlDumpTool::toolByInstallData(qtInstallData, qtHeaderData, false).isEmpty()
                    || !QmlDumpTool::toolByInstallData(qtInstallData, qtHeaderData, true).isEmpty();
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
            m_installed = false;
    }
    if (m_versionInfo.contains("QT_INSTALL_HEADERS")){
        QFileInfo fi(m_versionInfo.value("QT_INSTALL_HEADERS"));
        if (!fi.exists())
            m_installed = false;
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

QHash<QString,QString> BaseQtVersion::versionInfo() const
{
    updateVersionInfo();
    return m_versionInfo;
}

bool BaseQtVersion::hasDocumentation() const
{
    updateVersionInfo();
    return m_hasDocumentation;
}

QString BaseQtVersion::documentationPath() const
{
    updateVersionInfo();
    return m_versionInfo["QT_INSTALL_DOCS"];
}

bool BaseQtVersion::hasDemos() const
{
    updateVersionInfo();
    return m_hasDemos;
}

QString BaseQtVersion::demosPath() const
{
    updateVersionInfo();
    return m_versionInfo["QT_INSTALL_DEMOS"];
}

QString BaseQtVersion::frameworkInstallPath() const
{
#ifdef Q_OS_MAC
    updateVersionInfo();
    return m_versionInfo["QT_INSTALL_LIBS"];
#else
    return QString();
#endif
}

bool BaseQtVersion::hasExamples() const
{
    updateVersionInfo();
    return m_hasExamples;
}

QString BaseQtVersion::examplesPath() const
{
    updateVersionInfo();
    return m_versionInfo["QT_INSTALL_EXAMPLES"];
}

QList<ProjectExplorer::HeaderPath> BaseQtVersion::systemHeaderPathes() const
{
    QList<ProjectExplorer::HeaderPath> result;
    result.append(ProjectExplorer::HeaderPath(mkspecPath(), ProjectExplorer::HeaderPath::GlobalHeaderPath));
    return result;
}

void BaseQtVersion::addToEnvironment(Utils::Environment &env) const
{
    env.set("QTDIR", QDir::toNativeSeparators(versionInfo().value("QT_INSTALL_DATA")));
    env.prependOrSetPath(versionInfo().value("QT_INSTALL_BINS"));
    env.prependOrSetLibrarySearchPath(versionInfo().value("QT_INSTALL_LIBS"));
}

bool BaseQtVersion::hasGdbDebuggingHelper() const
{
    updateVersionInfo();
    return m_hasDebuggingHelper;
}


bool BaseQtVersion::hasQmlDump() const
{
    updateVersionInfo();
    return m_hasQmlDump;
}

bool BaseQtVersion::hasQmlDebuggingLibrary() const
{
    updateVersionInfo();
    return m_hasQmlDebuggingLibrary;
}

bool BaseQtVersion::needsQmlDebuggingLibrary() const
{
    updateVersionInfo();
    return qtVersion() < QtVersionNumber(4, 8, 0);
}

bool BaseQtVersion::hasQmlObserver() const
{
    updateVersionInfo();
    return m_hasQmlObserver;
}

Utils::Environment BaseQtVersion::qmlToolsEnvironment() const
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

QString BaseQtVersion::gdbDebuggingHelperLibrary() const
{
    QString qtInstallData = versionInfo().value("QT_INSTALL_DATA");
    if (qtInstallData.isEmpty())
        return QString();
    return ProjectExplorer::DebuggingHelperLibrary::debuggingHelperLibraryByInstallData(qtInstallData);
}

QString BaseQtVersion::qmlDumpTool(bool debugVersion) const
{
    QString qtInstallData = versionInfo().value("QT_INSTALL_DATA");
    QString qtHeaderData = versionInfo().value("QT_INSTALL_HEADERS");
    if (qtInstallData.isEmpty())
        return QString();
    return QmlDumpTool::toolByInstallData(qtInstallData, qtHeaderData, debugVersion);
}

QString BaseQtVersion::qmlDebuggingHelperLibrary(bool debugVersion) const
{
    QString qtInstallData = versionInfo().value("QT_INSTALL_DATA");
    if (qtInstallData.isEmpty())
        return QString();
    return QmlDebuggingLibrary::libraryByInstallData(qtInstallData, debugVersion);
}

QString BaseQtVersion::qmlObserverTool() const
{
    QString qtInstallData = versionInfo().value("QT_INSTALL_DATA");
    if (qtInstallData.isEmpty())
        return QString();
    return QmlObserverTool::toolByInstallData(qtInstallData);
}

QStringList BaseQtVersion::debuggingHelperLibraryLocations() const
{
    QString qtInstallData = versionInfo().value("QT_INSTALL_DATA");
    if (qtInstallData.isEmpty())
        return QStringList();
    return ProjectExplorer::DebuggingHelperLibrary::locationsByInstallData(qtInstallData);
}

bool BaseQtVersion::supportsBinaryDebuggingHelper() const
{
    if (!isValid())
        return false;
    return true;
}

void BaseQtVersion::recheckDumper()
{
    m_versionInfoUpToDate = false;
}

bool BaseQtVersion::supportsShadowBuilds() const
{
    return true;
}

QList<ProjectExplorer::Task> BaseQtVersion::reportIssuesImpl(const QString &proFile, const QString &buildDir)
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

    return results;
}

QList<ProjectExplorer::Task>
BaseQtVersion::reportIssues(const QString &proFile, const QString &buildDir)
{
    QList<ProjectExplorer::Task> results = reportIssuesImpl(proFile, buildDir);
    qSort(results);
    return results;
}

ProjectExplorer::IOutputParser *BaseQtVersion::createOutputParser() const
{
    return new ProjectExplorer::GnuMakeParser;
}

QtConfigWidget *BaseQtVersion::createConfigurationWidget() const
{
    return 0;
}

bool BaseQtVersion::queryQMakeVariables(const QString &binary, QHash<QString, QString> *versionInfo)
{
    bool qmakeIsExecutable;
    return BaseQtVersion::queryQMakeVariables(binary, versionInfo, &qmakeIsExecutable);
}

bool BaseQtVersion::queryQMakeVariables(const QString &binary, QHash<QString, QString> *versionInfo,
                                        bool *qmakeIsExecutable)
{
    const int timeOutMS = 30000; // Might be slow on some machines.
    const QFileInfo qmake(binary);
    *qmakeIsExecutable = qmake.exists() && qmake.isExecutable() && !qmake.isDir();
    if (!*qmakeIsExecutable)
        return false;
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
    const QString queryArg = QLatin1String("-query");
    QStringList args;
    for (uint i = 0; i < sizeof variables / sizeof variables[0]; ++i)
        args << queryArg << variables[i];
    QProcess process;
    process.start(qmake.absoluteFilePath(), args, QIODevice::ReadOnly);
    if (!process.waitForStarted()) {
        *qmakeIsExecutable = false;
        qWarning("Cannot start '%s': %s", qPrintable(binary), qPrintable(process.errorString()));
        return false;
    }
    if (!process.waitForFinished(timeOutMS)) {
        Utils::SynchronousProcess::stopProcess(process);
        qWarning("Timeout running '%s' (%dms).", qPrintable(binary), timeOutMS);
        return false;
    }
    if (process.exitStatus() != QProcess::NormalExit) {
        *qmakeIsExecutable = false;
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

QString BaseQtVersion::mkspecFromVersionInfo(const QHash<QString, QString> &versionInfo)
{
    QString baseMkspecDir = versionInfo.value("QMAKE_MKSPECS");
    if (baseMkspecDir.isEmpty())
        baseMkspecDir = versionInfo.value("QT_INSTALL_DATA") + "/mkspecs";
    if (baseMkspecDir.isEmpty())
        return QString();

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
                    mkspecFullPath = QFileInfo(mkspecFullPath).canonicalFilePath();
                }
                break;
            }
        }
        f2.close();
    }
#else
    mkspecFullPath = QFileInfo(mkspecFullPath).canonicalFilePath();
#endif

#ifdef Q_OS_WIN
    mkspecFullPath = mkspecFullPath.toLower();
#endif
    return mkspecFullPath;
}

QString BaseQtVersion::qtCorePath(const QHash<QString,QString> &versionInfo, const QString &versionString)
{
    QStringList dirs;
    dirs << versionInfo.value(QLatin1String("QT_INSTALL_LIBS"))
         << versionInfo.value(QLatin1String("QT_INSTALL_BINS"));

    QFileInfoList staticLibs;
    foreach (const QString &dir, dirs) {
        if (dir.isEmpty())
            continue;
        QDir d(dir);
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
                             || file.endsWith(QString::fromLatin1(".so.") + versionString)
                             || file.endsWith(QLatin1Char('.') + versionString + QLatin1String(".dylib")))

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

QList<ProjectExplorer::Abi> BaseQtVersion::qtAbisFromLibrary(const QString &coreLibrary)
{
    return ProjectExplorer::Abi::abisOfBinary(coreLibrary);
}
