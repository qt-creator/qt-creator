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

#include "baseqtversion.h"
#include "qmlobservertool.h"
#include "qmldumptool.h"
#include "qmldebugginglibrary.h"
#include "qtkitinformation.h"

#include "qtversionmanager.h"
#include "profilereader.h"
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchainmanager.h>
#include <qtsupport/debugginghelper.h>
#include <qtsupport/qtsupportconstants.h>

#include <utils/persistentsettings.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/synchronousprocess.h>

#include <QDir>
#include <QUrl>
#include <QFileInfo>
#include <QCoreApplication>
#include <QProcess>

using namespace QtSupport;
using namespace QtSupport::Internal;
using namespace Utils;

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
    const QString validChars = QLatin1String("0123456789.");
    foreach (const QChar &c, version) {
        if (!validChars.contains(c))
            return false;
        if (c == QLatin1Char('.'))
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

BaseQtVersion::BaseQtVersion(const FileName &qmakeCommand, bool isAutodetected, const QString &autodetectionSource)
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
    ctor(FileName());
}

void BaseQtVersion::ctor(const FileName &qmakePath)
{
    m_qmakeCommand = qmakePath;
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

QString BaseQtVersion::defaultDisplayName(const QString &versionString, const FileName &qmakePath,
                                          bool fromPath)
{
    QString location;
    if (qmakePath.isEmpty()) {
        location = QCoreApplication::translate("QtVersion", "<unknown>");
    } else {
        // Deduce a description from '/foo/qt-folder/[qtbase]/bin/qmake' -> '/foo/qt-folder'.
        // '/usr' indicates System Qt 4.X on Linux.
        QDir dir = qmakePath.toFileInfo().absoluteDir();
        do {
            const QString dirName = dir.dirName();
            if (dirName == QLatin1String("usr")) { // System-installed Qt.
                location = QCoreApplication::translate("QtVersion", "System");
                break;
            }
            location = dirName;
            // Also skip default checkouts named 'qt'. Parent dir might have descriptive name.
            if (dirName.compare(QLatin1String("bin"), Qt::CaseInsensitive)
                && dirName.compare(QLatin1String("qtbase"), Qt::CaseInsensitive)
                && dirName.compare(QLatin1String("qt"), Qt::CaseInsensitive)) {
                break;
            }
        } while (dir.cdUp());
    }

    return fromPath ?
        QCoreApplication::translate("QtVersion", "Qt %1 in PATH (%2)").arg(versionString, location) :
        QCoreApplication::translate("QtVersion", "Qt %1 (%2)").arg(versionString, location);
}

Core::FeatureSet BaseQtVersion::availableFeatures() const
{
    Core::FeatureSet features = Core::FeatureSet(QtSupport::Constants::FEATURE_QWIDGETS)
            | Core::FeatureSet(QtSupport::Constants::FEATURE_QT)
            | Core::FeatureSet(QtSupport::Constants::FEATURE_QT_WEBKIT)
            | Core::FeatureSet(QtSupport::Constants::FEATURE_QT_CONSOLE);

     if (qtVersion() >= QtSupport::QtVersionNumber(4, 7, 0)) {
         features |= Core::FeatureSet(QtSupport::Constants::FEATURE_QT_QUICK);
         features |= Core::FeatureSet(QtSupport::Constants::FEATURE_QT_QUICK_1);
     }
     if (qtVersion() >= QtSupport::QtVersionNumber(4, 7, 1))
         features |= Core::FeatureSet(QtSupport::Constants::FEATURE_QT_QUICK_1_1);
     if (qtVersion() >= QtSupport::QtVersionNumber(5, 0, 0))
         features |= Core::FeatureSet(QtSupport::Constants::FEATURE_QT_QUICK_2);

     return features;
}

QString BaseQtVersion::platformName() const
{
    return QString();
}

QString BaseQtVersion::platformDisplayName() const
{
    return platformName();
}

bool BaseQtVersion::supportsPlatform(const QString &platform) const
{
    if (platform.isEmpty()) // empty target == target independent
        return true;
    return platform == platformName();
}

QList<ProjectExplorer::Task> BaseQtVersion::validateKit(const ProjectExplorer::Kit *k)
{
    QList<ProjectExplorer::Task> result;

    BaseQtVersion *version = QtKitInformation::qtVersion(k);
    Q_ASSERT(version == this);

    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(k);

    const QList<ProjectExplorer::Abi> qtAbis = version->qtAbis();
    if (tc && !qtAbis.contains(tc->targetAbi())) {
        QString qtAbiString;
        foreach (const ProjectExplorer::Abi &qtAbi, qtAbis) {
            if (!qtAbiString.isEmpty())
                qtAbiString.append(QLatin1Char(' '));
            qtAbiString.append(qtAbi.toString());
        }
        const QString message = QCoreApplication::translate("BaseQtVersion",
                                                            "The compiler '%1' (%2) cannot produce code for the Qt version '%3' (%4).").
                                                            arg(tc->displayName(),
                                                                tc->targetAbi().toString(),
                                                                version->displayName(),
                                                                qtAbiString);
        result << ProjectExplorer::Task(ProjectExplorer::Task::Error,
                                        message, FileName(), -1,
                                        Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    } // Abi mismatch
    return result;
}

FileName BaseQtVersion::headerPath() const
{
    return Utils::FileName::fromUserInput(qmakeProperty("QT_INSTALL_HEADERS"));
}

FileName BaseQtVersion::libraryPath() const
{
    return Utils::FileName::fromUserInput(qmakeProperty("QT_INSTALL_LIBS"));
}

FileName BaseQtVersion::binPath() const
{
    return Utils::FileName::fromUserInput(qmakeProperty("QT_HOST_BINS"));
}

Utils::FileName QtSupport::BaseQtVersion::mkspecsPath() const
{
    Utils::FileName result = Utils::FileName::fromUserInput(qmakeProperty("QT_HOST_DATA"));
    if (result.isEmpty())
        result = Utils::FileName::fromUserInput(qmakeProperty("QMAKE_MKSPECS"));
    else
        result.appendPath(QLatin1String("mkspecs"));
    return result;
}

QString QtSupport::BaseQtVersion::qtNamespace() const
{
    return qmakeProperty("QT_NAMESPACE");
}

QString QtSupport::BaseQtVersion::qtLibInfix() const
{
    return qmakeProperty("QT_LIBINFIX");
}

void BaseQtVersion::setId(int id)
{
    m_id = id;
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
    QString string = map.value(QLatin1String(QTVERSIONQMAKEPATH)).toString();
    if (string.startsWith(QLatin1Char('~')))
        string.remove(0, 1).prepend(QDir::homePath());
    ctor(FileName::fromString(string));
}

QVariantMap BaseQtVersion::toMap() const
{
    QVariantMap result;
    result.insert(QLatin1String(QTVERSIONID), uniqueId());
    result.insert(QLatin1String(QTVERSIONNAME), displayName());
    result.insert(QLatin1String(QTVERSIONAUTODETECTED), isAutodetected());
    if (isAutodetected())
        result.insert(QLatin1String(QTVERSIONAUTODETECTIONSOURCE), autodetectionSource());
    result.insert(QLatin1String(QTVERSIONQMAKEPATH), qmakeCommand().toString());
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
            && !qmakeProperty("QT_HOST_BINS").isNull()
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
    if (qmakeProperty("QT_HOST_BINS").isNull())
        return QCoreApplication::translate("QtVersion",
                                           "Could not determine the path to the binaries of the Qt installation, maybe the qmake path is wrong?");
    if (m_mkspecUpToDate && m_mkspecFullPath.isEmpty())
        return QCoreApplication::translate("QtVersion", "The default mkspec symlink is broken.");
    return QString();
}

QStringList BaseQtVersion::warningReason() const
{
    QStringList ret;
    if (qtAbis().count() == 1 && qtAbis().first().isNull())
        ret << QCoreApplication::translate("QtVersion", "ABI detection failed: Make sure to use a matching compiler when building.");
    if (m_versionInfo.value(QLatin1String("QT_INSTALL_PREFIX/get"))
        != m_versionInfo.value(QLatin1String("QT_INSTALL_PREFIX"))) {
        ret << QCoreApplication::translate("QtVersion", "Non-installed -prefix build - for internal development only.");
    }
    return ret;
}

ProjectExplorer::ToolChain *BaseQtVersion::preferredToolChain(const FileName &ms) const
{
    const FileName spec = ms.isEmpty() ? mkspec() : ms;
    QList<ProjectExplorer::ToolChain *> tcList = ProjectExplorer::ToolChainManager::instance()->toolChains();
    ProjectExplorer::ToolChain *possibleTc = 0;
    foreach (ProjectExplorer::ToolChain *tc, tcList) {
        if (!qtAbis().contains(tc->targetAbi()))
            continue;
        if (tc->suggestedMkspecList().contains(spec))
            return tc; // perfect match
        if (!possibleTc)
            possibleTc = tc; // first possible match
    }
    return possibleTc;
}

FileName BaseQtVersion::qmakeCommand() const
{
    return m_qmakeCommand;
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
        str << "<tr><td colspan=2><b>"
            << QCoreApplication::translate("BaseQtVersion", "Invalid Qt version")
            << "</b></td></tr>";
    } else {
        str << "<tr><td><b>" << QCoreApplication::translate("BaseQtVersion", "ABI:")
            << "</b></td>";
        const QList<ProjectExplorer::Abi> abis = qtAbis();
        for (int i = 0; i < abis.size(); ++i) {
            if (i)
                str << "<tr><td></td>";
            str << "<td>" << abis.at(i).toString() << "</td></tr>";
        }
        str << "<tr><td><b>" << QCoreApplication::translate("BaseQtVersion", "Source:")
            << "</b></td><td>" << sourcePath().toUserOutput() << "</td></tr>";
        str << "<tr><td><b>" << QCoreApplication::translate("BaseQtVersion", "mkspec:")
            << "</b></td><td>" << mkspec().toUserOutput() << "</td></tr>";
        str << "<tr><td><b>" << QCoreApplication::translate("BaseQtVersion", "qmake:")
            << "</b></td><td>" << m_qmakeCommand.toUserOutput() << "</td></tr>";
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
                QStringList keys = vInfo.keys();
                keys.sort();
                foreach (QString variableName, keys) {
                    const QString &value = vInfo.value(variableName);
                    if (variableName != QLatin1String("QMAKE_MKSPECS")
                        && !variableName.endsWith(QLatin1String("/raw"))) {
                        bool isPath = false;
                        if (variableName.contains(QLatin1String("_HOST_"))
                            || variableName.contains(QLatin1String("_INSTALL_"))) {
                            if (!variableName.endsWith(QLatin1String("/get")))
                                continue;
                            variableName.chop(4);
                            isPath = true;
                        } else if (variableName == QLatin1String("QT_SYSROOT")) {
                            isPath = true;
                        }
                        str << "<tr><td><pre>" << variableName <<  "</pre></td><td>";
                        if (value.isEmpty())
                            isPath = false;
                        if (isPath) {
                            str << "<a href=\"" << QUrl::fromLocalFile(value).toString()
                                << "\">" << QDir::toNativeSeparators(value) << "</a>";
                        } else {
                            str << value;
                        }
                        str << "</td></tr>";
                    }
                }
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
    const QString installData = qmakeProperty("QT_INSTALL_PREFIX");
    QString sourcePath = installData;
    QFile qmakeCache(installData + QLatin1String("/.qmake.cache"));
    if (qmakeCache.exists()) {
        qmakeCache.open(QIODevice::ReadOnly | QIODevice::Text);
        QTextStream stream(&qmakeCache);
        while (!stream.atEnd()) {
            QString line = stream.readLine().trimmed();
            if (line.startsWith(QLatin1String("QT_SOURCE_TREE"))) {
                sourcePath = line.split(QLatin1Char('=')).at(1).trimmed();
                if (sourcePath.startsWith(QLatin1String("$$quote("))) {
                    sourcePath.remove(0, 8);
                    sourcePath.chop(1);
                }
                break;
            }
        }
    }
    m_sourcePath = FileName::fromUserInput(sourcePath);
}

FileName BaseQtVersion::sourcePath() const
{
    updateSourcePath();
    return m_sourcePath;
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

QString BaseQtVersion::qmlsceneCommand() const
{
    if (!isValid())
        return QString();

    if (m_qmlsceneCommand.isNull())
        m_qmlsceneCommand = findQtBinary(QmlScene);
    return m_qmlsceneCommand;
}

QString BaseQtVersion::qmlviewerCommand() const
{
    if (!isValid())
        return QString();

    if (m_qmlviewerCommand.isNull())
        m_qmlviewerCommand = findQtBinary(QmlViewer);
    return m_qmlviewerCommand;
}

QString BaseQtVersion::findQtBinary(Binaries binary) const
{
    QString baseDir;
    if (qtVersion() < QtVersionNumber(5, 0, 0)) {
        baseDir = qmakeProperty("QT_HOST_BINS");
    } else {
        ensureMkSpecParsed();
        switch (binary) {
        case QmlScene:
            baseDir = m_mkspecValues.value(QLatin1String("QT.qml.bins"));
            break;
        case QmlViewer:
            baseDir = m_mkspecValues.value(QLatin1String("QT.declarative.bins"));
            break;
        case Designer:
        case Linguist:
            baseDir = m_mkspecValues.value(QLatin1String("QT.designer.bins"));
            break;
        case Uic:
            baseDir = qmakeProperty("QT_HOST_BINS");
            break;
        default:
            // Can't happen
            Q_ASSERT(false);
        }
    }

    if (baseDir.isEmpty())
        return QString();
    if (!baseDir.endsWith(QLatin1Char('/')))
        baseDir += QLatin1Char('/');

    QStringList possibleCommands;
    switch (binary) {
    case QmlScene: {
        if (HostOsInfo::isWindowsHost())
            possibleCommands << QLatin1String("qmlscene.exe");
        else
            possibleCommands << QLatin1String("qmlscene");
    }
    case QmlViewer: {
        if (HostOsInfo::isWindowsHost())
            possibleCommands << QLatin1String("qmlviewer.exe");
        else if (HostOsInfo::isMacHost())
            possibleCommands << QLatin1String("QMLViewer.app/Contents/MacOS/QMLViewer");
        else
            possibleCommands << QLatin1String("qmlviewer");
    }
        break;
    case Designer:
        if (HostOsInfo::isWindowsHost())
            possibleCommands << QLatin1String("designer.exe");
        else if (HostOsInfo::isMacHost())
            possibleCommands << QLatin1String("Designer.app/Contents/MacOS/Designer");
        else
            possibleCommands << QLatin1String("designer");
        break;
    case Linguist:
        if (HostOsInfo::isWindowsHost())
            possibleCommands << QLatin1String("linguist.exe");
        else if (HostOsInfo::isMacHost())
            possibleCommands << QLatin1String("Linguist.app/Contents/MacOS/Linguist");
        else
            possibleCommands << QLatin1String("linguist");
        break;
    case Uic:
        if (HostOsInfo::isWindowsHost()) {
            possibleCommands << QLatin1String("uic.exe");
        } else {
            possibleCommands << QLatin1String("uic-qt4") << QLatin1String("uic4")
                             << QLatin1String("uic");
        }
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

void BaseQtVersion::updateMkspec() const
{
    if (uniqueId() == -1 || m_mkspecUpToDate)
        return;

    m_mkspecUpToDate = true;
    m_mkspecFullPath = mkspecFromVersionInfo(versionInfo());

    m_mkspec = m_mkspecFullPath;
    if (m_mkspecFullPath.isEmpty())
        return;

    FileName baseMkspecDir = mkspecDirectoryFromVersionInfo(versionInfo());

    if (m_mkspec.isChildOf(baseMkspecDir)) {
        m_mkspec = m_mkspec.relativeChildPath(baseMkspecDir);
//        qDebug() << "Setting mkspec to"<<mkspec;
    } else {
        FileName sourceMkSpecPath = sourcePath().appendPath(QLatin1String("mkspecs"));
        if (m_mkspec.isChildOf(sourceMkSpecPath)) {
            m_mkspec = m_mkspec.relativeChildPath(sourceMkSpecPath);
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

    ProFileGlobals option;
    option.setProperties(versionInfo());
    ProMessageHandler msgHandler(true);
    ProFileCacheManager::instance()->incRefCount();
    QMakeParser parser(ProFileCacheManager::instance()->cache(), &msgHandler);
    ProFileEvaluator evaluator(&option, &parser, &msgHandler);
    evaluator.loadNamedSpec(mkspecPath().toString(), false);

    parseMkSpec(&evaluator);

    ProFileCacheManager::instance()->decRefCount();
}

void BaseQtVersion::parseMkSpec(ProFileEvaluator *evaluator) const
{
    QStringList configValues = evaluator->values(QLatin1String("CONFIG"));
    m_defaultConfigIsDebugAndRelease = false;
    foreach (const QString &value, configValues) {
        if (value == QLatin1String("debug"))
            m_defaultConfigIsDebug = true;
        else if (value == QLatin1String("release"))
            m_defaultConfigIsDebug = false;
        else if (value == QLatin1String("build_all"))
            m_defaultConfigIsDebugAndRelease = true;
    }
    const QString designerBins = QLatin1String("QT.designer.bins");
    const QString qmlBins = QLatin1String("QT.qml.bins");
    const QString declarativeBins = QLatin1String("QT.declarative.bins");
    m_mkspecValues.insert(designerBins, evaluator->value(designerBins));
    m_mkspecValues.insert(qmlBins, evaluator->value(qmlBins));
    m_mkspecValues.insert(declarativeBins, evaluator->value(declarativeBins));
}

FileName BaseQtVersion::mkspec() const
{
    updateMkspec();
    return m_mkspec;
}

FileName BaseQtVersion::mkspecFor(ProjectExplorer::ToolChain *tc) const
{
    Utils::FileName versionSpec = mkspec();
    if (!tc)
        return versionSpec;

    const QList<FileName> tcSpecList = tc->suggestedMkspecList();
    if (tcSpecList.contains(versionSpec))
        return versionSpec;
    foreach (const FileName &tcSpec, tcSpecList) {
        if (hasMkspec(tcSpec))
            return tcSpec;
    }

    return versionSpec;
}

FileName BaseQtVersion::mkspecPath() const
{
    updateMkspec();
    return m_mkspecFullPath;
}

bool BaseQtVersion::hasMkspec(const FileName &spec) const
{
    updateVersionInfo();
    QFileInfo fi;
    fi.setFile(QDir::fromNativeSeparators(qmakeProperty("QT_HOST_DATA"))
               + QLatin1String("/mkspecs/") + spec.toString());
    if (fi.isDir())
        return true;
    fi.setFile(sourcePath().toString() + QLatin1String("/mkspecs/") + spec.toString());
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
    updateVersionInfo();
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
                 qPrintable(qmakeCommand().toString()));
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

    if (!queryQMakeVariables(qmakeCommand(), qmakeRunEnvironment(), &m_versionInfo)) {
        m_qmakeIsExecutable = false;
        return;
    }
    m_qmakeIsExecutable = true;

    const QString qtInstallData = qmakeProperty("QT_INSTALL_DATA");
    const QString qtInstallBins = qmakeProperty("QT_INSTALL_BINS");
    const QString qtHeaderData = qmakeProperty("QT_INSTALL_HEADERS");
    if (!qtInstallData.isNull()) {
        if (!qtInstallData.isEmpty()) {
            m_hasDebuggingHelper = !QtSupport::DebuggingHelperLibrary::debuggingHelperLibraryByInstallData(qtInstallData).isEmpty();
            m_hasQmlDump
                    = !QmlDumpTool::toolForQtPaths(qtInstallData, qtInstallBins, qtHeaderData, false).isEmpty()
                    || !QmlDumpTool::toolForQtPaths(qtInstallData, qtInstallBins, qtHeaderData, true).isEmpty();
            m_hasQmlDebuggingLibrary
                    = !QmlDebuggingLibrary::libraryByInstallData(qtInstallData, false).isEmpty()
                || !QmlDebuggingLibrary::libraryByInstallData(qtInstallData, true).isEmpty();
            m_hasQmlObserver = !QmlObserverTool::toolByInstallData(qtInstallData).isEmpty();
        }
    }

    // Now check for a qt that is configured with a prefix but not installed
    QString installDir = qmakeProperty("QT_HOST_BINS");
    if (!installDir.isNull()) {
        QFileInfo fi(installDir);
        if (!fi.exists())
            m_installed = false;
    }
    if (!qtHeaderData.isNull()) {
        const QFileInfo fi(qtHeaderData);
        if (!fi.exists())
            m_installed = false;
    }
    const QString qtInstallDocs = qmakeProperty("QT_INSTALL_DOCS");
    if (!qtInstallDocs.isNull()) {
        const QFileInfo fi(qtInstallDocs);
        if (fi.exists())
            m_hasDocumentation = true;
    }
    const QString qtInstallExamples = qmakeProperty("QT_INSTALL_EXAMPLES");
    if (!qtInstallExamples.isNull()) {
        const QFileInfo fi(qtInstallExamples);
        if (fi.exists())
            m_hasExamples = true;
    }
    const QString qtInstallDemos = qmakeProperty("QT_INSTALL_DEMOS");
    if (!qtInstallDemos.isNull()) {
        const QFileInfo fi(qtInstallDemos);
        if (fi.exists())
            m_hasDemos = true;
    }
    m_qtVersionString = qmakeProperty("QT_VERSION");

    m_versionInfoUpToDate = true;
}

QHash<QString,QString> BaseQtVersion::versionInfo() const
{
    updateVersionInfo();
    return m_versionInfo;
}

QString BaseQtVersion::qmakeProperty(const QHash<QString,QString> &versionInfo, const QByteArray &name)
{
    QString val = versionInfo.value(QString::fromLatin1(name + "/get"));
    if (!val.isNull())
        return val;
    return versionInfo.value(QString::fromLatin1(name));
}

QString BaseQtVersion::qmakeProperty(const QByteArray &name) const
{
    return qmakeProperty(m_versionInfo, name);
}

bool BaseQtVersion::hasDocumentation() const
{
    updateVersionInfo();
    return m_hasDocumentation;
}

QString BaseQtVersion::documentationPath() const
{
    updateVersionInfo();
    return qmakeProperty("QT_INSTALL_DOCS");
}

bool BaseQtVersion::hasDemos() const
{
    updateVersionInfo();
    return m_hasDemos;
}

QString BaseQtVersion::demosPath() const
{
    updateVersionInfo();
    return qmakeProperty("QT_INSTALL_DEMOS");
}

QString BaseQtVersion::frameworkInstallPath() const
{
    if (HostOsInfo::isMacHost()) {
        updateVersionInfo();
        return m_versionInfo.value(QLatin1String("QT_INSTALL_LIBS"));
    }
    return QString();
}

bool BaseQtVersion::hasExamples() const
{
    updateVersionInfo();
    return m_hasExamples;
}

QString BaseQtVersion::examplesPath() const
{
    updateVersionInfo();
    return qmakeProperty("QT_INSTALL_EXAMPLES");
}

QList<ProjectExplorer::HeaderPath> BaseQtVersion::systemHeaderPathes(const ProjectExplorer::Kit *k) const
{
    Q_UNUSED(k);
    QList<ProjectExplorer::HeaderPath> result;
    result.append(ProjectExplorer::HeaderPath(mkspecPath().toString(), ProjectExplorer::HeaderPath::GlobalHeaderPath));
    return result;
}

void BaseQtVersion::addToEnvironment(const ProjectExplorer::Kit *k, Environment &env) const
{
    Q_UNUSED(k);
    env.set(QLatin1String("QTDIR"), QDir::toNativeSeparators(qmakeProperty("QT_HOST_DATA")));
    env.prependOrSetPath(qmakeProperty("QT_HOST_BINS"));
}

// Some Qt versions may require environment settings for qmake to work
//
// One such example is Blackberry which for some reason decided to always use the same
// qmake and use environment variables embedded in their mkspecs to make that point to
// the different Qt installations.
Utils::Environment BaseQtVersion::qmakeRunEnvironment() const
{
    return Utils::Environment::systemEnvironment();
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

bool BaseQtVersion::needsQmlDump() const
{
    updateVersionInfo();
    return qtVersion() < QtVersionNumber(4, 8, 0);
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

Environment BaseQtVersion::qmlToolsEnvironment() const
{
    // FIXME: This seems broken!
    Environment environment = Environment::systemEnvironment();
#if 0 // FIXME: Fix this!
    addToEnvironment(environment);
#endif

    // add preferred tool chain, as that is how the tools are built, compare QtVersion::buildDebuggingHelperLibrary
    if (!qtAbis().isEmpty()) {
        QList<ProjectExplorer::ToolChain *> alltc =
                ProjectExplorer::ToolChainManager::instance()->findToolChains(qtAbis().at(0));
        if (!alltc.isEmpty())
            alltc.first()->addToEnvironment(environment);
    }

    return environment;
}

QString BaseQtVersion::gdbDebuggingHelperLibrary() const
{
    QString qtInstallData = qmakeProperty("QT_INSTALL_DATA");
    if (qtInstallData.isEmpty())
        return QString();
    return QtSupport::DebuggingHelperLibrary::debuggingHelperLibraryByInstallData(qtInstallData);
}

QString BaseQtVersion::qmlDumpTool(bool debugVersion) const
{
    const QString qtInstallData = qmakeProperty("QT_INSTALL_DATA");
    if (qtInstallData.isEmpty())
        return QString();
    const QString qtInstallBins = qmakeProperty("QT_INSTALL_BINS");
    const QString qtHeaderData = qmakeProperty("QT_INSTALL_HEADERS");
    return QmlDumpTool::toolForQtPaths(qtInstallData, qtInstallBins, qtHeaderData, debugVersion);
}

QString BaseQtVersion::qmlDebuggingHelperLibrary(bool debugVersion) const
{
    QString qtInstallData = qmakeProperty("QT_INSTALL_DATA");
    if (qtInstallData.isEmpty())
        return QString();
    return QmlDebuggingLibrary::libraryByInstallData(qtInstallData, debugVersion);
}

QString BaseQtVersion::qmlObserverTool() const
{
    QString qtInstallData = qmakeProperty("QT_INSTALL_DATA");
    if (qtInstallData.isEmpty())
        return QString();
    return QmlObserverTool::toolByInstallData(qtInstallData);
}

QStringList BaseQtVersion::debuggingHelperLibraryLocations() const
{
    QString qtInstallData = qmakeProperty("QT_INSTALL_DATA");
    if (qtInstallData.isEmpty())
        return QStringList();
    return QtSupport::DebuggingHelperLibrary::debuggingHelperLibraryDirectories(qtInstallData);
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

QList<ProjectExplorer::Task> BaseQtVersion::reportIssuesImpl(const QString &proFile, const QString &buildDir) const
{
    QList<ProjectExplorer::Task> results;

    QString tmpBuildDir = QDir(buildDir).absolutePath();
    if (!tmpBuildDir.endsWith(QLatin1Char('/')))
        tmpBuildDir.append(QLatin1Char('/'));

    if (!isValid()) {
        //: %1: Reason for being invalid
        const QString msg = QCoreApplication::translate("Qt4ProjectManager::QtVersion", "The Qt version is invalid: %1").arg(invalidReason());
        results.append(ProjectExplorer::Task(ProjectExplorer::Task::Error, msg, FileName(), -1,
                                             Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
    }

    QFileInfo qmakeInfo = qmakeCommand().toFileInfo();
    if (!qmakeInfo.exists() ||
        !qmakeInfo.isExecutable()) {
        //: %1: Path to qmake executable
        const QString msg = QCoreApplication::translate("Qt4ProjectManager::QtVersion",
                                                        "The qmake command \"%1\" was not found or is not executable.").arg(qmakeCommand().toUserOutput());
        results.append(ProjectExplorer::Task(ProjectExplorer::Task::Error, msg, FileName(), -1,
                                             Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
    }

    QString sourcePath = QFileInfo(proFile).absolutePath();
    const QChar slash = QLatin1Char('/');
    if (!sourcePath.endsWith(slash))
        sourcePath.append(slash);
    if ((tmpBuildDir.startsWith(sourcePath)) && (tmpBuildDir != sourcePath)) {
        const QString msg = QCoreApplication::translate("Qt4ProjectManager::QtVersion",
                                                        "Qmake does not support build directories below the source directory.");
        results.append(ProjectExplorer::Task(ProjectExplorer::Task::Warning, msg, FileName(), -1,
                                             Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
    } else if (tmpBuildDir.count(slash) != sourcePath.count(slash) && qtVersion() < QtVersionNumber(4,8, 0)) {
        const QString msg = QCoreApplication::translate("Qt4ProjectManager::QtVersion",
                                                        "The build directory needs to be at the same level as the source directory.");

        results.append(ProjectExplorer::Task(ProjectExplorer::Task::Warning, msg, FileName(), -1,
                                             Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
    }

    return results;
}

QList<ProjectExplorer::Task>
BaseQtVersion::reportIssues(const QString &proFile, const QString &buildDir) const
{
    QList<ProjectExplorer::Task> results = reportIssuesImpl(proFile, buildDir);
    qSort(results);
    return results;
}

QtConfigWidget *BaseQtVersion::createConfigurationWidget() const
{
    return 0;
}

static QByteArray runQmakeQuery(const FileName &binary, const Environment &env,
                                QString *error)
{
    QTC_ASSERT(error, return QByteArray());

    const int timeOutMS = 30000; // Might be slow on some machines.

    QProcess process;
    process.setEnvironment(env.toStringList());
    process.start(binary.toString(), QStringList(QLatin1String("-query")), QIODevice::ReadOnly);

    if (!process.waitForStarted()) {
        *error = QCoreApplication::translate("QtVersion", "Cannot start '%1': %2").arg(binary.toUserOutput()).arg(process.errorString());
        return QByteArray();
    }
    if (!process.waitForFinished(timeOutMS)) {
        SynchronousProcess::stopProcess(process);
        *error = QCoreApplication::translate("QtVersion", "Timeout running '%1' (%2ms).").arg(binary.toUserOutput()).arg(timeOutMS);
        return QByteArray();
    }
    if (process.exitStatus() != QProcess::NormalExit) {
        *error = QCoreApplication::translate("QtVersion", "'%1' crashed.").arg(binary.toUserOutput());
        return QByteArray();
    }

    error->clear();
    return process.readAllStandardOutput();
}

bool BaseQtVersion::queryQMakeVariables(const FileName &binary, const Environment &env,
                                        QHash<QString, QString> *versionInfo, QString *error)
{
    QString tmp;
    if (!error)
        error = &tmp;

    const QFileInfo qmake = binary.toFileInfo();
    if (!qmake.exists() || !qmake.isExecutable() || qmake.isDir()) {
        *error = QCoreApplication::translate("QtVersion", "qmake '%1' is not a executable").arg(binary.toUserOutput());
        return false;
    }

    QByteArray output;
    output = runQmakeQuery(binary, env, error);

    if (output.isNull() && !error->isEmpty()) {
        // Note: Don't rerun if we were able to execute the binary before.

        // Try running qmake with all kinds of tool chains set up in the environment.
        // This is required to make non-static qmakes work on windows where every tool chain
        // tries to be incompatible with any other.
        QList<ProjectExplorer::Abi> abiList = ProjectExplorer::Abi::abisOfBinary(binary);
        QList<ProjectExplorer::ToolChain *> tcList = ProjectExplorer::ToolChainManager::instance()->toolChains();
        foreach (ProjectExplorer::ToolChain *tc, tcList) {
            if (!abiList.contains(tc->targetAbi()))
                continue;
            Environment realEnv = env;
            tc->addToEnvironment(realEnv);
            output = runQmakeQuery(binary, realEnv, error);
            if (error->isEmpty())
                break;
        }
    }

    if (output.isNull())
        return false;

    QTextStream stream(&output);
    while (!stream.atEnd()) {
        const QString line = stream.readLine();
        const int index = line.indexOf(QLatin1Char(':'));
        if (index != -1) {
            QString name = line.left(index);
            QString value = QDir::fromNativeSeparators(line.mid(index+1));
            if (value.isNull())
                value = QLatin1String(""); // Make sure it is not null, to discern from missing keys
            versionInfo->insert(name, value);
            if (name.startsWith(QLatin1String("QT_")) && !name.contains(QLatin1Char('/'))) {
                if (name.startsWith(QLatin1String("QT_INSTALL_"))) {
                    versionInfo->insert(name + QLatin1String("/raw"), value);
                    versionInfo->insert(name + QLatin1String("/get"), value);
                    if (name == QLatin1String("QT_INSTALL_PREFIX")
                        || name == QLatin1String("QT_INSTALL_DATA")
                        || name == QLatin1String("QT_INSTALL_BINS")) {
                        name.replace(3, 7, QLatin1String("HOST"));
                        versionInfo->insert(name, value);
                        versionInfo->insert(name + QLatin1String("/get"), value);
                    }
                } else if (name.startsWith(QLatin1String("QT_HOST_"))) {
                    versionInfo->insert(name + QLatin1String("/get"), value);
                }
            }
        }
    }
    return true;
}

FileName BaseQtVersion::mkspecDirectoryFromVersionInfo(const QHash<QString, QString> &versionInfo)
{
    QString dataDir = qmakeProperty(versionInfo, "QT_HOST_DATA");
    if (dataDir.isEmpty())
        return FileName();
    return FileName::fromUserInput(dataDir + QLatin1String("/mkspecs"));
}

FileName BaseQtVersion::mkspecFromVersionInfo(const QHash<QString, QString> &versionInfo)
{
    FileName baseMkspecDir = mkspecDirectoryFromVersionInfo(versionInfo);
    if (baseMkspecDir.isEmpty())
        return FileName();

    bool qt5 = false;
    QString theSpec = qmakeProperty(versionInfo, "QMAKE_XSPEC");
    if (theSpec.isEmpty())
        theSpec = QLatin1String("default");
    else
        qt5 = true;

    FileName mkspecFullPath = baseMkspecDir;
    mkspecFullPath.appendPath(theSpec);

    // qDebug() << "default mkspec is located at" << mkspecFullPath;

    if (HostOsInfo::isWindowsHost()) {
        if (!qt5) {
            QFile f2(mkspecFullPath.toString() + QLatin1String("/qmake.conf"));
            if (f2.exists() && f2.open(QIODevice::ReadOnly)) {
                while (!f2.atEnd()) {
                    QByteArray line = f2.readLine();
                    if (line.startsWith("QMAKESPEC_ORIGINAL")) {
                        const QList<QByteArray> &temp = line.split('=');
                        if (temp.size() == 2) {
                            QString possibleFullPath = QString::fromLocal8Bit(temp.at(1).trimmed().constData());
                            // We sometimes get a mix of different slash styles here...
                            possibleFullPath = possibleFullPath.replace(QLatin1Char('\\'), QLatin1Char('/'));
                            if (QFileInfo(possibleFullPath).exists()) // Only if the path exists
                                mkspecFullPath = FileName::fromUserInput(possibleFullPath);
                        }
                        break;
                    }
                }
                f2.close();
            }
        }
    } else {
        if (HostOsInfo::isMacHost()) {
            QFile f2(mkspecFullPath.toString() + QLatin1String("/qmake.conf"));
            if (f2.exists() && f2.open(QIODevice::ReadOnly)) {
                while (!f2.atEnd()) {
                    QByteArray line = f2.readLine();
                    if (line.startsWith("MAKEFILE_GENERATOR")) {
                        const QList<QByteArray> &temp = line.split('=');
                        if (temp.size() == 2) {
                            const QByteArray &value = temp.at(1);
                            if (value.contains("XCODE")) {
                                // we don't want to generate xcode projects...
                                // qDebug() << "default mkspec is xcode, falling back to g++";
                                return baseMkspecDir.appendPath(QLatin1String("macx-g++"));
                            }
                        }
                        break;
                    }
                }
                f2.close();
            }
        }
        if (!qt5) {
            //resolve mkspec link
            QString rspec = mkspecFullPath.toFileInfo().readLink();
            if (!rspec.isEmpty())
                mkspecFullPath = FileName::fromUserInput(
                            QDir(baseMkspecDir.toString()).absoluteFilePath(rspec));
        }
    }
    return mkspecFullPath;
}

FileName BaseQtVersion::qtCorePath(const QHash<QString,QString> &versionInfo, const QString &versionString)
{
    QStringList dirs;
    dirs << qmakeProperty(versionInfo, "QT_INSTALL_LIBS")
         << qmakeProperty(versionInfo, "QT_INSTALL_BINS");

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
                FileName lib(info);
                lib.appendPath(file.left(file.lastIndexOf(QLatin1Char('.'))));
                return lib;
            }
            if (info.isReadable()) {
                if (file.startsWith(QLatin1String("libQtCore"))
                        || file.startsWith(QLatin1String("libQt5Core"))
                        || file.startsWith(QLatin1String("QtCore"))
                        || file.startsWith(QLatin1String("Qt5Core"))) {
                    // Only handle static libs if we can not find dynamic ones:
                    if (file.endsWith(QLatin1String(".a")) || file.endsWith(QLatin1String(".lib")))
                        staticLibs.append(info);
                    else if (file.endsWith(QLatin1String(".dll"))
                             || file.endsWith(QString::fromLatin1(".so.") + versionString)
                             || file.endsWith(QLatin1String(".so"))
                             || file.endsWith(QLatin1Char('.') + versionString + QLatin1String(".dylib")))
                        return FileName(info);
                }
            }
        }
    }
    // Return path to first static library found:
    if (!staticLibs.isEmpty())
        return FileName(staticLibs.at(0));
    return FileName();
}

QList<ProjectExplorer::Abi> BaseQtVersion::qtAbisFromLibrary(const FileName &coreLibrary)
{
    return ProjectExplorer::Abi::abisOfBinary(coreLibrary);
}
