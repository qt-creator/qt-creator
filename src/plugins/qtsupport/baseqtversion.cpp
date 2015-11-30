/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "baseqtversion.h"
#include "qtconfigwidget.h"
#include "qmldumptool.h"
#include "qtkitinformation.h"

#include "qtversionmanager.h"
#include "profilereader.h"
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <proparser/qmakevfs.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/headerpath.h>
#include <qtsupport/debugginghelperbuildtask.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>
#include <utils/synchronousprocess.h>
#include <utils/winutils.h>
#include <utils/algorithm.h>

#include <QDir>
#include <QUrl>
#include <QFileInfo>
#include <QFuture>
#include <QCoreApplication>
#include <QProcess>
#include <QRegExp>

using namespace Core;
using namespace QtSupport;
using namespace QtSupport::Internal;
using namespace ProjectExplorer;
using namespace Utils;

static const char QTVERSIONAUTODETECTED[] = "isAutodetected";
static const char QTVERSIONAUTODETECTIONSOURCE []= "autodetectionSource";
static const char QTVERSIONQMAKEPATH[] = "QMakePath";

static const char MKSPEC_VALUE_LIBINFIX[] = "QT_LIBINFIX";
static const char MKSPEC_VALUE_NAMESPACE[] = "QT_NAMESPACE";

///////////////
// QtVersionNumber
///////////////
QtVersionNumber::QtVersionNumber(int ma, int mi, int p)
    : majorVersion(ma), minorVersion(mi), patchVersion(p)
{
}

QtVersionNumber::QtVersionNumber(const QString &versionString)
{
    if (::sscanf(versionString.toLatin1().constData(), "%d.%d.%d",
           &majorVersion, &minorVersion, &patchVersion) != 3)
        majorVersion = minorVersion = patchVersion = -1;
}

QtVersionNumber::QtVersionNumber()
{
    majorVersion = minorVersion = patchVersion = -1;
}

FeatureSet QtVersionNumber::features() const
{
    return FeatureSet::versionedFeatures(Constants::FEATURE_QT_PREFIX, majorVersion, minorVersion);
}

bool QtVersionNumber::matches(int major, int minor, int patch) const
{
    if (major < 0)
        return true;
    if (major != majorVersion)
        return false;

    if (minor < 0)
        return true;
    if (minor != minorVersion)
        return false;

    if (patch < 0)
        return true;
    return (patch == patchVersion);
}

bool QtVersionNumber::operator <(const QtVersionNumber &b) const
{
    if (majorVersion != b.majorVersion)
        return majorVersion < b.majorVersion;
    if (minorVersion != b.minorVersion)
        return minorVersion < b.minorVersion;
    return patchVersion < b.patchVersion;
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
// BaseQtVersion
///////////////
int BaseQtVersion::getUniqueId()
{
    return QtVersionManager::getUniqueId();
}

BaseQtVersion::BaseQtVersion(const FileName &qmakeCommand, bool isAutodetected, const QString &autodetectionSource)
    : m_id(getUniqueId()),
      m_isAutodetected(isAutodetected),
      m_hasQmlDump(false),
      m_mkspecUpToDate(false),
      m_mkspecReadUpToDate(false),
      m_defaultConfigIsDebug(true),
      m_defaultConfigIsDebugAndRelease(true),
      m_frameworkBuild(false),
      m_versionInfoUpToDate(false),
      m_installed(true),
      m_hasExamples(false),
      m_hasDemos(false),
      m_hasDocumentation(false),
      m_qmakeIsExecutable(true),
      m_hasQtAbis(false),
      m_autodetectionSource(autodetectionSource)
{
    ctor(qmakeCommand);
}

BaseQtVersion::BaseQtVersion(const BaseQtVersion &other) :
    m_id(other.m_id),
    m_isAutodetected(other.m_isAutodetected),
    m_hasQmlDump(other.m_hasQmlDump),
    m_mkspecUpToDate(other.m_mkspecUpToDate),
    m_mkspecReadUpToDate(other.m_mkspecReadUpToDate),
    m_defaultConfigIsDebug(other.m_defaultConfigIsDebug),
    m_defaultConfigIsDebugAndRelease(other.m_defaultConfigIsDebugAndRelease),
    m_frameworkBuild(other.m_frameworkBuild),
    m_versionInfoUpToDate(other.m_versionInfoUpToDate),
    m_installed(other.m_installed),
    m_hasExamples(other.m_hasExamples),
    m_hasDemos(other.m_hasDemos),
    m_hasDocumentation(other.m_hasDocumentation),
    m_qmakeIsExecutable(other.m_qmakeIsExecutable),
    m_hasQtAbis(other.m_hasQtAbis),
    m_configValues(other.m_configValues),
    m_qtConfigValues(other.m_qtConfigValues),
    m_unexpandedDisplayName(other.m_unexpandedDisplayName),
    m_autodetectionSource(other.m_autodetectionSource),
    m_sourcePath(other.m_sourcePath),
    m_mkspec(other.m_mkspec),
    m_mkspecFullPath(other.m_mkspecFullPath),
    m_mkspecValues(other.m_mkspecValues),
    m_versionInfo(other.m_versionInfo),
    m_qmakeCommand(other.m_qmakeCommand),
    m_qtVersionString(other.m_qtVersionString),
    m_uicCommand(other.m_uicCommand),
    m_designerCommand(other.m_designerCommand),
    m_linguistCommand(other.m_linguistCommand),
    m_qmlsceneCommand(other.m_qmlsceneCommand),
    m_qmlviewerCommand(other.m_qmlviewerCommand),
    m_qtAbis(other.m_qtAbis)
{
    setupExpander();
}

BaseQtVersion::BaseQtVersion()
    :  m_id(-1), m_isAutodetected(false),
    m_hasQmlDump(false),
    m_mkspecUpToDate(false),
    m_mkspecReadUpToDate(false),
    m_defaultConfigIsDebug(true),
    m_defaultConfigIsDebugAndRelease(true),
    m_frameworkBuild(false),
    m_versionInfoUpToDate(false),
    m_installed(true),
    m_hasExamples(false),
    m_hasDemos(false),
    m_hasDocumentation(false),
    m_qmakeIsExecutable(true),
    m_hasQtAbis(false)
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
    m_hasQtAbis = false;
    m_qtVersionString.clear();
    m_sourcePath.clear();
    setupExpander();
}

void BaseQtVersion::setupExpander()
{
    m_expander.setDisplayName(
        QtKitInformation::tr("Qt version"));

    m_expander.registerVariable("Qt:Version",
        QtKitInformation::tr("The version string of the current Qt version."),
        [this] { return qtVersionString(); });

    m_expander.registerVariable("Qt:Type",
        QtKitInformation::tr("The type of the current Qt version."),
        [this] { return type(); });

    m_expander.registerVariable("Qt:Mkspec",
        QtKitInformation::tr("The mkspec of the current Qt version."),
        [this] { return mkspec().toUserOutput(); });

    m_expander.registerVariable("Qt:QT_INSTALL_PREFIX",
        QtKitInformation::tr("The installation prefix of the current Qt version."),
        [this] { return qmakeProperty(m_versionInfo, "QT_INSTALL_PREFIX"); });

    m_expander.registerVariable("Qt:QT_INSTALL_DATA",
        QtKitInformation::tr("The installation location of the current Qt version's data."),
        [this] { return qmakeProperty(m_versionInfo, "QT_INSTALL_DATA"); });

    m_expander.registerVariable("Qt:QT_INSTALL_HEADERS",
        QtKitInformation::tr("The installation location of the current Qt version's header files."),
        [this] { return qmakeProperty(m_versionInfo, "QT_INSTALL_HEADERS"); });

    m_expander.registerVariable("Qt:QT_INSTALL_LIBS",
        QtKitInformation::tr("The installation location of the current Qt version's library files."),
        [this] { return qmakeProperty(m_versionInfo, "QT_INSTALL_LIBS"); });

    m_expander.registerVariable("Qt:QT_INSTALL_DOCS",
        QtKitInformation::tr("The installation location of the current Qt version's documentation files."),
        [this] { return qmakeProperty(m_versionInfo, "QT_INSTALL_DOCS"); });

    m_expander.registerVariable("Qt:QT_INSTALL_BINS",
        QtKitInformation::tr("The installation location of the current Qt version's executable files."),
        [this] { return qmakeProperty(m_versionInfo, "QT_INSTALL_BINS"); });

    m_expander.registerVariable("Qt:QT_INSTALL_PLUGINS",
        QtKitInformation::tr("The installation location of the current Qt version's plugins."),
        [this] { return qmakeProperty(m_versionInfo, "QT_INSTALL_PLUGINS"); });

    m_expander.registerVariable("Qt:QT_INSTALL_IMPORTS",
        QtKitInformation::tr("The installation location of the current Qt version's imports."),
        [this] { return qmakeProperty(m_versionInfo, "QT_INSTALL_IMPORTS"); });

    m_expander.registerVariable("Qt:QT_INSTALL_TRANSLATIONS",
        QtKitInformation::tr("The installation location of the current Qt version's translation files."),
        [this] { return qmakeProperty(m_versionInfo, "QT_INSTALL_TRANSLATIONS"); });

    m_expander.registerVariable("Qt:QT_INSTALL_CONFIGURATION",
        QtKitInformation::tr("The installation location of the current Qt version's translation files."),
        [this] { return qmakeProperty(m_versionInfo, "QT_INSTALL_CONFIGURATION"); });

    m_expander.registerVariable("Qt:QT_INSTALL_EXAMPLES",
        QtKitInformation::tr("The installation location of the current Qt version's examples."),
        [this] { return qmakeProperty(m_versionInfo, "QT_INSTALL_EXAMPLES"); });

    m_expander.registerVariable("Qt:QT_INSTALL_DEMOS",
        QtKitInformation::tr("The installation location of the current Qt version's demos."),
        [this] { return qmakeProperty(m_versionInfo, "QT_INSTALL_DEMOS"); });

    m_expander.registerVariable("Qt:QMAKE_MKSPECS",
        QtKitInformation::tr("The current Qt version's default mkspecs."),
        [this] { return qmakeProperty(m_versionInfo, "QMAKE_MKSPECS"); });

    m_expander.registerVariable("Qt:QMAKE_VERSION",
        QtKitInformation::tr("The current Qt's qmake version."),
        [this] { return qmakeProperty(m_versionInfo, "QMAKE_VERSION"); });

//    FIXME: Re-enable once we can detect expansion loops.
//    m_expander.registerVariable("Qt:Name",
//        QtKitInformation::tr("The display name of the current Qt version."),
//        [this] { return displayName(); });
}

BaseQtVersion::~BaseQtVersion()
{
}

QString BaseQtVersion::defaultUnexpandedDisplayName(const FileName &qmakePath, bool fromPath)
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
        } while (!dir.isRoot() && dir.cdUp());
    }

    return fromPath ?
        QCoreApplication::translate("QtVersion", "Qt %{Qt:Version} in PATH (%2)").arg(location) :
        QCoreApplication::translate("QtVersion", "Qt %{Qt:Version} (%2)").arg(location);
}

FeatureSet BaseQtVersion::availableFeatures() const
{
    FeatureSet features = qtVersion().features(); // Qt Version features

    features |= (Feature(Constants::FEATURE_QWIDGETS)
                 | Feature(Constants::FEATURE_QT_WEBKIT)
                 | Feature(Constants::FEATURE_QT_CONSOLE));

    if (qtVersion() < QtVersionNumber(4, 7, 0))
        return features;

    features |= FeatureSet::versionedFeatures(Constants::FEATURE_QT_QUICK_PREFIX, 1, 0);

    if (qtVersion().matches(4, 7, 0))
        return features;

    features |= FeatureSet::versionedFeatures(Constants::FEATURE_QT_QUICK_PREFIX, 1, 1);

    if (qtVersion().matches(4))
        return features;

    features |= FeatureSet::versionedFeatures(Constants::FEATURE_QT_QUICK_PREFIX, 2, 0);

    if (qtVersion().matches(5, 0))
        return features;

    features |= FeatureSet::versionedFeatures(Constants::FEATURE_QT_QUICK_PREFIX, 2, 1);
    features |= FeatureSet::versionedFeatures(Constants::FEATURE_QT_QUICK_CONTROLS_PREFIX, 1, 0);

    if (qtVersion().matches(5, 1))
        return features;

    features |= FeatureSet::versionedFeatures(Constants::FEATURE_QT_QUICK_PREFIX, 2, 2);
    features |= FeatureSet::versionedFeatures(Constants::FEATURE_QT_QUICK_CONTROLS_PREFIX, 1, 1);

    if (qtVersion().matches(5, 2))
        return features;

    features |= FeatureSet::versionedFeatures(Constants::FEATURE_QT_QUICK_PREFIX, 2, 3);
    features |= FeatureSet::versionedFeatures(Constants::FEATURE_QT_QUICK_CONTROLS_PREFIX, 1, 2);

    if (qtVersion().matches(5, 3))
        return features;

    features |= Feature(Constants::FEATURE_QT_QUICK_UI_FILES);

    features |= FeatureSet::versionedFeatures(Constants::FEATURE_QT_QUICK_PREFIX, 2, 4);
    features |= FeatureSet::versionedFeatures(Constants::FEATURE_QT_QUICK_CONTROLS_PREFIX, 1, 3);

    if (qtVersion().matches(5, 4))
        return features;

    features |= Feature(Constants::FEATURE_QT_3D);
    features |= Feature(Constants::FEATURE_QT_CANVAS3D);

    features |= FeatureSet::versionedFeatures(Constants::FEATURE_QT_QUICK_PREFIX, 2, 5);
    features |= FeatureSet::versionedFeatures(Constants::FEATURE_QT_QUICK_CONTROLS_PREFIX, 1, 4);

    if (qtVersion().matches(5, 5))
        return features;

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

QList<Task> BaseQtVersion::validateKit(const Kit *k)
{
    QList<Task> result;

    BaseQtVersion *version = QtKitInformation::qtVersion(k);
    Q_ASSERT(version == this);

    const QList<Abi> qtAbis = version->qtAbis();
    if (qtAbis.isEmpty()) // No need to test if Qt does not know anyway...
        return result;

    ToolChain *tc = ToolChainKitInformation::toolChain(k);
    if (tc) {
        Abi targetAbi = tc->targetAbi();
        bool fuzzyMatch = false;
        bool fullMatch = false;

        QString qtAbiString;
        foreach (const Abi &qtAbi, qtAbis) {
            if (!qtAbiString.isEmpty())
                qtAbiString.append(QLatin1Char(' '));
            qtAbiString.append(qtAbi.toString());

            if (!fullMatch)
                fullMatch = (targetAbi == qtAbi);
            if (!fuzzyMatch)
                fuzzyMatch = targetAbi.isCompatibleWith(qtAbi);
        }

        QString message;
        if (!fullMatch) {
            if (!fuzzyMatch)
                message = QCoreApplication::translate("BaseQtVersion",
                                                      "The compiler \"%1\" (%2) cannot produce code for the Qt version \"%3\" (%4).");
            else
                message = QCoreApplication::translate("BaseQtVersion",
                                                      "The compiler \"%1\" (%2) may not produce code compatible with the Qt version \"%3\" (%4).");
            message = message.arg(tc->displayName(), targetAbi.toString(),
                                  version->displayName(), qtAbiString);
            result << Task(fuzzyMatch ? Task::Warning : Task::Error, message, FileName(), -1,
                           ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);
        }
    }
    return result;
}

FileName BaseQtVersion::headerPath() const
{
    return FileName::fromUserInput(qmakeProperty("QT_INSTALL_HEADERS"));
}

FileName BaseQtVersion::docsPath() const
{
    return FileName::fromUserInput(qmakeProperty("QT_INSTALL_DOCS"));
}

FileName BaseQtVersion::libraryPath() const
{
    return FileName::fromUserInput(qmakeProperty("QT_INSTALL_LIBS"));
}

FileName BaseQtVersion::pluginPath() const
{
    return FileName::fromUserInput(qmakeProperty("QT_INSTALL_PLUGINS"));
}

FileName BaseQtVersion::binPath() const
{
    return FileName::fromUserInput(qmakeProperty("QT_HOST_BINS"));
}

FileName BaseQtVersion::mkspecsPath() const
{
    FileName result = FileName::fromUserInput(qmakeProperty("QT_HOST_DATA"));
    if (result.isEmpty())
        result = FileName::fromUserInput(qmakeProperty("QMAKE_MKSPECS"));
    else
        result.appendPath(QLatin1String("mkspecs"));
    return result;
}

QString BaseQtVersion::qtNamespace() const
{
    ensureMkSpecParsed();
    return m_mkspecValues.value(QLatin1String(MKSPEC_VALUE_NAMESPACE));
}

QString BaseQtVersion::qtLibInfix() const
{
    ensureMkSpecParsed();
    return m_mkspecValues.value(QLatin1String(MKSPEC_VALUE_LIBINFIX));
}

bool BaseQtVersion::isFrameworkBuild() const
{
    ensureMkSpecParsed();
    return m_frameworkBuild;
}

bool BaseQtVersion::hasDebugBuild() const
{
    return m_defaultConfigIsDebug || m_defaultConfigIsDebugAndRelease;
}

bool BaseQtVersion::hasReleaseBuild() const
{
    return !m_defaultConfigIsDebug || m_defaultConfigIsDebugAndRelease;
}

void BaseQtVersion::setId(int id)
{
    m_id = id;
}

void BaseQtVersion::fromMap(const QVariantMap &map)
{
    m_id = map.value(QLatin1String(Constants::QTVERSIONID)).toInt();
    if (m_id == -1) // this happens on adding from installer, see updateFromInstaller => get a new unique id
        m_id = QtVersionManager::getUniqueId();
    m_unexpandedDisplayName = map.value(QLatin1String(Constants::QTVERSIONNAME)).toString();
    m_isAutodetected = map.value(QLatin1String(QTVERSIONAUTODETECTED)).toBool();
    if (m_isAutodetected)
        m_autodetectionSource = map.value(QLatin1String(QTVERSIONAUTODETECTIONSOURCE)).toString();
    QString string = map.value(QLatin1String(QTVERSIONQMAKEPATH)).toString();
    if (string.startsWith(QLatin1Char('~')))
        string.remove(0, 1).prepend(QDir::homePath());

    QFileInfo fi(string);
    if (BuildableHelperLibrary::isQtChooser(fi)) {
        // we don't want to treat qtchooser as a normal qmake
        // see e.g. QTCREATORBUG-9841, also this lead to users changing what
        // qtchooser forwards too behind our backs, which will inadvertly lead to bugs
        string = BuildableHelperLibrary::qtChooserToQmakePath(fi.symLinkTarget());
    }

    ctor(FileName::fromString(string));
}

QVariantMap BaseQtVersion::toMap() const
{
    QVariantMap result;
    result.insert(QLatin1String(Constants::QTVERSIONID), uniqueId());
    result.insert(QLatin1String(Constants::QTVERSIONNAME), unexpandedDisplayName());
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
    if (qtAbis().isEmpty())
        ret << QCoreApplication::translate("QtVersion", "ABI detection failed: Make sure to use a matching compiler when building.");
    if (m_versionInfo.value(QLatin1String("QT_INSTALL_PREFIX/get"))
        != m_versionInfo.value(QLatin1String("QT_INSTALL_PREFIX"))) {
        ret << QCoreApplication::translate("QtVersion", "Non-installed -prefix build - for internal development only.");
    }
    return ret;
}

FileName BaseQtVersion::qmakeCommand() const
{
    return m_qmakeCommand;
}

QList<Abi> BaseQtVersion::qtAbis() const
{
    if (!m_hasQtAbis) {
        m_qtAbis = detectQtAbis();
        m_hasQtAbis = true;
    }
    return m_qtAbis;
}

bool BaseQtVersion::equals(BaseQtVersion *other)
{
    if (m_qmakeCommand != other->m_qmakeCommand)
        return false;
    if (type() != other->type())
        return false;
    if (uniqueId() != other->uniqueId())
        return false;
    if (displayName() != other->displayName())
        return false;
    if (isValid() != other->isValid())
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
    return m_expander.expand(m_unexpandedDisplayName);
}

QString BaseQtVersion::unexpandedDisplayName() const
{
    return m_unexpandedDisplayName;
}

void BaseQtVersion::setUnexpandedDisplayName(const QString &name)
{
    m_unexpandedDisplayName = name;
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
        const QList<Abi> abis = qtAbis();
        if (abis.isEmpty()) {
            str << "<td>" << Abi().toString() << "</td></tr>";
        } else {
            for (int i = 0; i < abis.size(); ++i) {
                if (i)
                    str << "<tr><td></td>";
                str << "<td>" << abis.at(i).toString() << "</td></tr>";
            }
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
    m_sourcePath = sourcePath(m_versionInfo);
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
    case QmlScene:
        possibleCommands << HostOsInfo::withExecutableSuffix(QLatin1String("qmlscene"));
        break;
    case QmlViewer: {
        if (HostOsInfo::isMacHost())
            possibleCommands << QLatin1String("QMLViewer.app/Contents/MacOS/QMLViewer");
        else
            possibleCommands << HostOsInfo::withExecutableSuffix(QLatin1String("qmlviewer"));
    }
        break;
    case Designer:
        if (HostOsInfo::isMacHost())
            possibleCommands << QLatin1String("Designer.app/Contents/MacOS/Designer");
        else
            possibleCommands << HostOsInfo::withExecutableSuffix(QLatin1String("designer"));
        break;
    case Linguist:
        if (HostOsInfo::isMacHost())
            possibleCommands << QLatin1String("Linguist.app/Contents/MacOS/Linguist");
            possibleCommands << HostOsInfo::withExecutableSuffix(QLatin1String("linguist"));
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

    QMakeVfs vfs;
    ProFileGlobals option;
    option.setProperties(versionInfo());
    option.environment = qmakeRunEnvironment().toProcessEnvironment();
    ProMessageHandler msgHandler(true);
    ProFileCacheManager::instance()->incRefCount();
    QMakeParser parser(ProFileCacheManager::instance()->cache(), &vfs, &msgHandler);
    ProFileEvaluator evaluator(&option, &parser, &vfs, &msgHandler);
    evaluator.loadNamedSpec(mkspecPath().toString(), false);

    parseMkSpec(&evaluator);

    ProFileCacheManager::instance()->decRefCount();
}

void BaseQtVersion::parseMkSpec(ProFileEvaluator *evaluator) const
{
    m_configValues = evaluator->values(QLatin1String("CONFIG"));
    m_qtConfigValues = evaluator->values(QLatin1String("QT_CONFIG"));
    m_defaultConfigIsDebugAndRelease = false;
    m_frameworkBuild = false;
    foreach (const QString &value, m_configValues) {
        if (value == QLatin1String("debug"))
            m_defaultConfigIsDebug = true;
        else if (value == QLatin1String("release"))
            m_defaultConfigIsDebug = false;
        else if (value == QLatin1String("build_all"))
            m_defaultConfigIsDebugAndRelease = true;
        else if (value == QLatin1String("qt_framework"))
            m_frameworkBuild = true;
    }
    const QString designerBins = QLatin1String("QT.designer.bins");
    const QString qmlBins = QLatin1String("QT.qml.bins");
    const QString declarativeBins = QLatin1String("QT.declarative.bins");
    const QString libinfix = QLatin1String(MKSPEC_VALUE_LIBINFIX);
    const QString ns = QLatin1String(MKSPEC_VALUE_NAMESPACE);
    m_mkspecValues.insert(designerBins, evaluator->value(designerBins));
    m_mkspecValues.insert(qmlBins, evaluator->value(qmlBins));
    m_mkspecValues.insert(declarativeBins, evaluator->value(declarativeBins));
    m_mkspecValues.insert(libinfix, evaluator->value(libinfix));
    m_mkspecValues.insert(ns, evaluator->value(ns));
}

FileName BaseQtVersion::mkspec() const
{
    updateMkspec();
    return m_mkspec;
}

FileName BaseQtVersion::mkspecFor(ToolChain *tc) const
{
    FileName versionSpec = mkspec();
    if (!tc)
        return versionSpec;

    const FileNameList tcSpecList = tc->suggestedMkspecList();
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
    if (!m_qmakeIsExecutable)
        return;

    // extract data from qmake executable
    m_versionInfo.clear();
    m_installed = true;
    m_hasExamples = false;
    m_hasDocumentation = false;
    m_hasQmlDump = false;

    if (!queryQMakeVariables(qmakeCommand(), qmakeRunEnvironment(), &m_versionInfo)) {
        m_qmakeIsExecutable = false;
        qWarning("Cannot update Qt version information: %s cannot be run.",
                 qPrintable(qmakeCommand().toString()));
        return;
    }
    m_qmakeIsExecutable = true;

    const QString qtInstallData = qmakeProperty(m_versionInfo, "QT_INSTALL_DATA");
    const QString qtInstallBins = qmakeProperty(m_versionInfo, "QT_INSTALL_BINS");
    const QString qtHeaderData = qmakeProperty(m_versionInfo, "QT_INSTALL_HEADERS");
    if (!qtInstallData.isNull()) {
        if (!qtInstallData.isEmpty()) {
            m_hasQmlDump
                    = !QmlDumpTool::toolForQtPaths(qtInstallData, qtInstallBins, qtHeaderData, false).isEmpty()
                    || !QmlDumpTool::toolForQtPaths(qtInstallData, qtInstallBins, qtHeaderData, true).isEmpty();
        }
    }

    // Now check for a qt that is configured with a prefix but not installed
    QString installDir = qmakeProperty(m_versionInfo, "QT_HOST_BINS");
    if (!installDir.isNull()) {
        if (!QFileInfo::exists(installDir))
            m_installed = false;
    }
    // Framework builds for Qt 4.8 don't use QT_INSTALL_HEADERS
    // so we don't check on mac
    if (!HostOsInfo::isMacHost()) {
        if (!qtHeaderData.isNull()) {
            if (!QFileInfo::exists(qtHeaderData))
                m_installed = false;
        }
    }
    const QString qtInstallDocs = qmakeProperty(m_versionInfo, "QT_INSTALL_DOCS");
    if (!qtInstallDocs.isNull()) {
        if (QFileInfo::exists(qtInstallDocs))
            m_hasDocumentation = true;
    }
    const QString qtInstallExamples = qmakeProperty(m_versionInfo, "QT_INSTALL_EXAMPLES");
    if (!qtInstallExamples.isNull()) {
        if (QFileInfo::exists(qtInstallExamples))
            m_hasExamples = true;
    }
    const QString qtInstallDemos = qmakeProperty(m_versionInfo, "QT_INSTALL_DEMOS");
    if (!qtInstallDemos.isNull()) {
        if (QFileInfo::exists(qtInstallDemos))
            m_hasDemos = true;
    }
    m_qtVersionString = qmakeProperty(m_versionInfo, "QT_VERSION");

    m_versionInfoUpToDate = true;
}

QHash<QString,QString> BaseQtVersion::versionInfo() const
{
    updateVersionInfo();
    return m_versionInfo;
}

QString BaseQtVersion::qmakeProperty(const QHash<QString,QString> &versionInfo, const QByteArray &name,
                                     PropertyVariant variant)
{
    QString val = versionInfo.value(QString::fromLatin1(
            name + (variant == PropertyVariantGet ? "/get" : "/src")));
    if (!val.isNull())
        return val;
    return versionInfo.value(QString::fromLatin1(name));
}

QString BaseQtVersion::qmakeProperty(const QByteArray &name) const
{
    updateVersionInfo();
    return qmakeProperty(m_versionInfo, name);
}

bool BaseQtVersion::hasDocumentation() const
{
    updateVersionInfo();
    return m_hasDocumentation;
}

QString BaseQtVersion::documentationPath() const
{
    return qmakeProperty("QT_INSTALL_DOCS");
}

bool BaseQtVersion::hasDemos() const
{
    updateVersionInfo();
    return m_hasDemos;
}

QString BaseQtVersion::demosPath() const
{
    return QFileInfo(qmakeProperty("QT_INSTALL_DEMOS")).canonicalFilePath();
}

QString BaseQtVersion::frameworkInstallPath() const
{
    if (HostOsInfo::isMacHost())
        return qmakeProperty("QT_INSTALL_LIBS");
    return QString();
}

bool BaseQtVersion::hasExamples() const
{
    updateVersionInfo();
    return m_hasExamples;
}

QString BaseQtVersion::examplesPath() const
{
    return QFileInfo(qmakeProperty("QT_INSTALL_EXAMPLES")).canonicalFilePath();
}

QStringList BaseQtVersion::configValues() const
{
    ensureMkSpecParsed();
    return m_configValues;
}

QStringList BaseQtVersion::qtConfigValues() const
{
    ensureMkSpecParsed();
    return m_qtConfigValues;
}

MacroExpander *BaseQtVersion::macroExpander() const
{
    return &m_expander;
}

void BaseQtVersion::addToEnvironment(const Kit *k, Environment &env) const
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
Environment BaseQtVersion::qmakeRunEnvironment() const
{
    return Environment::systemEnvironment();
}

bool BaseQtVersion::hasQmlDump() const
{
    updateVersionInfo();
    return m_hasQmlDump;
}

bool BaseQtVersion::hasQmlDumpWithRelocatableFlag() const
{
    return ((qtVersion() > QtVersionNumber(4, 8, 4) && qtVersion() < QtVersionNumber(5, 0, 0))
            || qtVersion() >= QtVersionNumber(5, 1, 0));
}

bool BaseQtVersion::needsQmlDump() const
{
    return qtVersion() < QtVersionNumber(4, 8, 0);
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
        QList<ToolChain *> alltc = ToolChainManager::findToolChains(qtAbis().at(0));
        if (!alltc.isEmpty())
            alltc.first()->addToEnvironment(environment);
    }

    return environment;
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

void BaseQtVersion::recheckDumper()
{
    m_versionInfoUpToDate = false;
}

QList<Task> BaseQtVersion::reportIssuesImpl(const QString &proFile, const QString &buildDir) const
{
    QList<Task> results;

    QString tmpBuildDir = QDir(buildDir).absolutePath();
    if (!tmpBuildDir.endsWith(QLatin1Char('/')))
        tmpBuildDir.append(QLatin1Char('/'));

    if (!isValid()) {
        //: %1: Reason for being invalid
        const QString msg = QCoreApplication::translate("QmakeProjectManager::QtVersion", "The Qt version is invalid: %1").arg(invalidReason());
        results.append(Task(Task::Error, msg, FileName(), -1,
                            ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    }

    QFileInfo qmakeInfo = qmakeCommand().toFileInfo();
    if (!qmakeInfo.exists() ||
        !qmakeInfo.isExecutable()) {
        //: %1: Path to qmake executable
        const QString msg = QCoreApplication::translate("QmakeProjectManager::QtVersion",
                                                        "The qmake command \"%1\" was not found or is not executable.").arg(qmakeCommand().toUserOutput());
        results.append(Task(Task::Error, msg, FileName(), -1,
                            ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    }

    QString sourcePath = QFileInfo(proFile).absolutePath();
    const QChar slash = QLatin1Char('/');
    if (!sourcePath.endsWith(slash))
        sourcePath.append(slash);
    if ((tmpBuildDir.startsWith(sourcePath)) && (tmpBuildDir != sourcePath) && qtVersion() < QtVersionNumber(5, 2, 0)) {
        const QString msg = QCoreApplication::translate("QmakeProjectManager::QtVersion",
                                                        "Qmake does not support build directories below the source directory.");
        results.append(Task(Task::Warning, msg, FileName(), -1,
                             ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    } else if (tmpBuildDir.count(slash) != sourcePath.count(slash) && qtVersion() < QtVersionNumber(4,8, 0)) {
        const QString msg = QCoreApplication::translate("QmakeProjectManager::QtVersion",
                                                        "The build directory needs to be at the same level as the source directory.");

        results.append(Task(Task::Warning, msg, FileName(), -1,
                            ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    }

    return results;
}

QList<Task> BaseQtVersion::reportIssues(const QString &proFile, const QString &buildDir) const
{
    QList<Task> results = reportIssuesImpl(proFile, buildDir);
    Utils::sort(results);
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

    // Prevent e.g. qmake 4.x on MinGW to show annoying errors about missing dll's.
    WindowsCrashDialogBlocker crashDialogBlocker;

    QProcess process;
    process.setEnvironment(env.toStringList());
    process.start(binary.toString(), QStringList(QLatin1String("-query")), QIODevice::ReadOnly);

    if (!process.waitForStarted()) {
        *error = QCoreApplication::translate("QtVersion", "Cannot start \"%1\": %2").arg(binary.toUserOutput()).arg(process.errorString());
        return QByteArray();
    }
    if (!process.waitForFinished(timeOutMS)) {
        SynchronousProcess::stopProcess(process);
        *error = QCoreApplication::translate("QtVersion", "Timeout running \"%1\" (%2 ms).").arg(binary.toUserOutput()).arg(timeOutMS);
        return QByteArray();
    }
    if (process.exitStatus() != QProcess::NormalExit) {
        *error = QCoreApplication::translate("QtVersion", "\"%1\" crashed.").arg(binary.toUserOutput());
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
        *error = QCoreApplication::translate("QtVersion", "qmake \"%1\" is not an executable.").arg(binary.toUserOutput());
        return false;
    }

    QByteArray output;
    output = runQmakeQuery(binary, env, error);

    if (output.isNull() && !error->isEmpty()) {
        // Note: Don't rerun if we were able to execute the binary before.

        // Try running qmake with all kinds of tool chains set up in the environment.
        // This is required to make non-static qmakes work on windows where every tool chain
        // tries to be incompatible with any other.
        QList<Abi> abiList = Abi::abisOfBinary(binary);
        QList<ToolChain *> tcList = ToolChainManager::toolChains();
        foreach (ToolChain *tc, tcList) {
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
    QString dataDir = qmakeProperty(versionInfo, "QT_HOST_DATA", PropertyVariantSrc);
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
                            if (possibleFullPath.contains(QLatin1Char('$'))) { // QTBUG-28792
                                const QRegExp rex(QLatin1String("\\binclude\\(([^)]+)/qmake\\.conf\\)"));
                                if (rex.indexIn(QString::fromLocal8Bit(f2.readAll())) != -1) {
                                    possibleFullPath = mkspecFullPath.toString() + QLatin1Char('/')
                                            + rex.cap(1);
                                }
                            }
                            // We sometimes get a mix of different slash styles here...
                            possibleFullPath = possibleFullPath.replace(QLatin1Char('\\'), QLatin1Char('/'));
                            if (QFileInfo::exists(possibleFullPath)) // Only if the path exists
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

FileName BaseQtVersion::sourcePath(const QHash<QString, QString> &versionInfo)
{
    const QString qt5Source = qmakeProperty(versionInfo, "QT_INSTALL_PREFIX/src");
    if (!qt5Source.isEmpty())
        return Utils::FileName::fromString(QFileInfo(qt5Source).canonicalFilePath());

    const QString installData = qmakeProperty(versionInfo, "QT_INSTALL_PREFIX");
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
    return FileName::fromUserInput(QFileInfo(sourcePath).canonicalFilePath());
}

bool BaseQtVersion::isInSourceDirectory(const Utils::FileName &filePath)
{
    const Utils::FileName &source = sourcePath();
    if (source.isEmpty())
        return false;
    QDir dir = QDir(source.toString());
    if (dir.dirName() == QLatin1String("qtbase"))
        dir.cdUp();
    return filePath.isChildOf(dir);
}

bool BaseQtVersion::isSubProject(const Utils::FileName &filePath)
{
    const Utils::FileName &source = sourcePath();
    if (!source.isEmpty()) {
        QDir dir = QDir(source.toString());
        if (dir.dirName() == QLatin1String("qtbase"))
            dir.cdUp();

        if (filePath.isChildOf(dir))
            return true;
    }

    const QString &examples = examplesPath();
    if (!examples.isEmpty() && filePath.isChildOf(QDir(examples)))
        return true;

    const QString &demos = demosPath();
    if (!demos.isEmpty() && filePath.isChildOf(QDir(demos)))
        return true;

    return false;
}

bool BaseQtVersion::isQmlDebuggingSupported(Kit *k, QString *reason)
{
    QTC_ASSERT(k, return false);
    BaseQtVersion *version = QtKitInformation::qtVersion(k);
    if (!version) {
        if (reason)
            *reason = QCoreApplication::translate("BaseQtVersion", "No Qt version.");
        return false;
    }
    return version->isQmlDebuggingSupported(reason);
}

bool BaseQtVersion::isQmlDebuggingSupported(QString *reason) const
{
    if (!isValid()) {
        if (reason)
            *reason = QCoreApplication::translate("BaseQtVersion", "Invalid Qt version.");
        return false;
    }

    if (qtVersion() < QtVersionNumber(5, 0, 0)) {
        if (reason)
            *reason = QCoreApplication::translate("BaseQtVersion", "Requires Qt 5.0.0 or newer.");
        return false;
    }

    return true;
}

bool BaseQtVersion::isQtQuickCompilerSupported(Kit *k, QString *reason)
{
    QTC_ASSERT(k, return false);
    BaseQtVersion *version = QtKitInformation::qtVersion(k);
    if (!version) {
        if (reason)
            *reason = QCoreApplication::translate("BaseQtVersion", "No Qt version.");
        return false;
    }
    return version->isQtQuickCompilerSupported(reason);
}

bool BaseQtVersion::isQtQuickCompilerSupported(QString *reason) const
{
    if (!isValid()) {
        if (reason)
            *reason = QCoreApplication::translate("BaseQtVersion", "Invalid Qt version.");
        return false;
    }

    if (qtVersion() < QtVersionNumber(5, 3, 0)) {
        if (reason)
            *reason = QCoreApplication::translate("BaseQtVersion", "Requires Qt 5.3.0 or newer.");
        return false;
    }

    const QString qtQuickCompilerExecutable =
            HostOsInfo::withExecutableSuffix(binPath().toString() + QLatin1String("/qtquickcompiler"));
    if (!QFileInfo::exists(qtQuickCompilerExecutable)) {
        if (reason)
            *reason = QCoreApplication::translate("BaseQtVersion", "This Qt Version does not contain Qt Quick Compiler.");
        return false;
    }

    return true;
}

FileNameList BaseQtVersion::qtCorePaths(const QHash<QString,QString> &versionInfo, const QString &versionString)
{
    QStringList dirs;
    dirs << qmakeProperty(versionInfo, "QT_INSTALL_LIBS")
         << qmakeProperty(versionInfo, "QT_INSTALL_BINS");

    FileNameList staticLibs;
    FileNameList dynamicLibs;
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
                dynamicLibs.append(lib.appendPath(file.left(file.lastIndexOf(QLatin1Char('.')))));
            } else if (info.isReadable()) {
                if (file.startsWith(QLatin1String("libQtCore"))
                        || file.startsWith(QLatin1String("libQt5Core"))
                        || file.startsWith(QLatin1String("QtCore"))
                        || file.startsWith(QLatin1String("Qt5Core"))) {
                    if (file.endsWith(QLatin1String(".a")) || file.endsWith(QLatin1String(".lib")))
                        staticLibs.append(FileName(info));
                    else if (file.endsWith(QLatin1String(".dll"))
                             || file.endsWith(QString::fromLatin1(".so.") + versionString)
                             || file.endsWith(QLatin1String(".so"))
                             || file.endsWith(QLatin1Char('.') + versionString + QLatin1String(".dylib")))
                        dynamicLibs.append(FileName(info));
                }
            }
        }
    }
    // Only handle static libs if we can not find dynamic ones:
    if (dynamicLibs.isEmpty())
        return staticLibs;
    return dynamicLibs;
}

QList<Abi> BaseQtVersion::qtAbisFromLibrary(const FileNameList &coreLibraries)
{
    QList<Abi> res;
    foreach (const FileName &library, coreLibraries)
        foreach (const Abi &abi, Abi::abisOfBinary(library))
            if (!res.contains(abi))
                res.append(abi);
    return res;
}
