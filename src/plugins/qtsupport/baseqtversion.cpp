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

#include "baseqtversion.h"
#include "qtconfigwidget.h"
#include "qmldumptool.h"
#include "qtkitinformation.h"

#include "qtversionmanager.h"
#include "profilereader.h"
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <proparser/qmakevfs.h>
#include <projectexplorer/deployablefile.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>
#include <utils/synchronousprocess.h>
#include <utils/winutils.h>
#include <utils/fileinprojectfinder.h>

#include <resourceeditor/resourcenode.h>

#include <QDir>
#include <QUrl>
#include <QFileInfo>
#include <QFuture>
#include <QCoreApplication>
#include <QProcess>
#include <QRegularExpression>

#include <algorithm>

using namespace Core;
using namespace QtSupport;
using namespace QtSupport::Internal;
using namespace ProjectExplorer;
using namespace Utils;

static const char QTVERSIONAUTODETECTED[] = "isAutodetected";
static const char QTVERSIONAUTODETECTIONSOURCE[] = "autodetectionSource";
static const char QTVERSION_OVERRIDE_FEATURES[] = "overrideFeatures";
static const char QTVERSIONQMAKEPATH[] = "QMakePath";
static const char QTVERSIONSOURCEPATH[] = "SourcePath";

static const char QTVERSION_ABIS[] = "Abis";

static const char MKSPEC_VALUE_LIBINFIX[] = "QT_LIBINFIX";
static const char MKSPEC_VALUE_NAMESPACE[] = "QT_NAMESPACE";

static QSet<Id> versionedIds(const QByteArray &prefix, int major, int minor)
{
    QSet<Id> result;
    result.insert(Id::fromName(prefix));

    if (major < 0)
        return result;

    const QByteArray majorStr = QString::number(major).toLatin1();
    const QByteArray featureMajor = prefix + majorStr;
    const QByteArray featureDotMajor = prefix + '.' + majorStr;

    result.insert(Id::fromName(featureMajor));
    result.insert(Id::fromName(featureDotMajor));

    for (int i = 0; i <= minor; ++i) {
        const QByteArray minorStr = QString::number(i).toLatin1();
        result.insert(Id::fromName(featureMajor + '.' + minorStr));
        result.insert(Id::fromName(featureDotMajor + '.' + minorStr));
    }

    return result;
}

///////////////
// MacroExpanderWrapper
///////////////
MacroExpander *MacroExpanderWrapper::macroExpander(const BaseQtVersion *qtversion) const
{
    if (!m_expander)
        m_expander = BaseQtVersion::createMacroExpander([qtversion]() { return qtversion; });
    return m_expander.get();
}

///////////////
// QtVersionNumber
///////////////
QtVersionNumber::QtVersionNumber(int ma, int mi, int p)
    : majorVersion(ma), minorVersion(mi), patchVersion(p)
{ }

QtVersionNumber::QtVersionNumber(const QString &versionString)
{
    if (::sscanf(versionString.toLatin1().constData(), "%d.%d.%d",
           &majorVersion, &minorVersion, &patchVersion) != 3)
        majorVersion = minorVersion = patchVersion = -1;
}

QSet<Id> QtVersionNumber::features() const
{
    return versionedIds(Constants::FEATURE_QT_PREFIX, majorVersion, minorVersion);
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

BaseQtVersion::BaseQtVersion(const FileName &qmakeCommand, bool isAutodetected, const QString &autodetectionSource)
    : m_id(QtVersionManager::getUniqueId()),
      m_isAutodetected(isAutodetected),
      m_autodetectionSource(autodetectionSource),
      m_qmakeCommand(qmakeCommand)
{ }
BaseQtVersion::BaseQtVersion(const BaseQtVersion &other) = default;
BaseQtVersion::BaseQtVersion() = default;

BaseQtVersion::~BaseQtVersion() = default;

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
            if (dirName == "usr") { // System-installed Qt.
                location = QCoreApplication::translate("QtVersion", "System");
                break;
            }
            location = dirName;
            // Also skip default checkouts named 'qt'. Parent dir might have descriptive name.
            if (dirName.compare("bin", Qt::CaseInsensitive)
                && dirName.compare("qtbase", Qt::CaseInsensitive)
                && dirName.compare("qt", Qt::CaseInsensitive)) {
                break;
            }
        } while (!dir.isRoot() && dir.cdUp());
    }

    return fromPath ?
        QCoreApplication::translate("QtVersion", "Qt %{Qt:Version} in PATH (%2)").arg(location) :
        QCoreApplication::translate("QtVersion", "Qt %{Qt:Version} (%2)").arg(location);
}

QSet<Id> BaseQtVersion::availableFeatures() const
{
    QSet<Id> features = qtVersion().features(); // Qt Version features

    features.insert(Constants::FEATURE_QWIDGETS);
    features.insert(Constants::FEATURE_QT_WEBKIT);
    features.insert(Constants::FEATURE_QT_CONSOLE);

    if (qtVersion() < QtVersionNumber(4, 7, 0))
        return features;

    features.unite(versionedIds(Constants::FEATURE_QT_QUICK_PREFIX, 1, 0));

    if (qtVersion().matches(4, 7, 0))
        return features;

    features.unite(versionedIds(Constants::FEATURE_QT_QUICK_PREFIX, 1, 1));

    if (qtVersion().matches(4))
        return features;

    features.unite(versionedIds(Constants::FEATURE_QT_QUICK_PREFIX, 2, 0));

    if (qtVersion().matches(5, 0))
        return features;

    features.unite(versionedIds(Constants::FEATURE_QT_QUICK_PREFIX, 2, 1));
    features.unite(versionedIds(Constants::FEATURE_QT_QUICK_CONTROLS_PREFIX, 1, 0));

    if (qtVersion().matches(5, 1))
        return features;

    features.unite(versionedIds(Constants::FEATURE_QT_QUICK_PREFIX, 2, 2));
    features.unite(versionedIds(Constants::FEATURE_QT_QUICK_CONTROLS_PREFIX, 1, 1));

    if (qtVersion().matches(5, 2))
        return features;

    features.unite(versionedIds(Constants::FEATURE_QT_QUICK_PREFIX, 2, 3));
    features.unite(versionedIds(Constants::FEATURE_QT_QUICK_CONTROLS_PREFIX, 1, 2));

    if (qtVersion().matches(5, 3))
        return features;

    features.insert(Constants::FEATURE_QT_QUICK_UI_FILES);

    features.unite(versionedIds(Constants::FEATURE_QT_QUICK_PREFIX, 2, 4));
    features.unite(versionedIds(Constants::FEATURE_QT_QUICK_CONTROLS_PREFIX, 1, 3));

    if (qtVersion().matches(5, 4))
        return features;

    features.insert(Constants::FEATURE_QT_3D);

    features.unite(versionedIds(Constants::FEATURE_QT_QUICK_PREFIX, 2, 5));
    features.unite(versionedIds(Constants::FEATURE_QT_QUICK_CONTROLS_PREFIX, 1, 4));
    features.unite(versionedIds(Constants::FEATURE_QT_CANVAS3D_PREFIX, 1, 0));

    if (qtVersion().matches(5, 5))
        return features;

    features.unite(versionedIds(Constants::FEATURE_QT_QUICK_PREFIX, 2, 6));
    features.unite(versionedIds(Constants::FEATURE_QT_QUICK_CONTROLS_PREFIX, 1, 5));
    features.unite(versionedIds(Constants::FEATURE_QT_LABS_CONTROLS_PREFIX, 1, 0));
    features.unite(versionedIds(Constants::FEATURE_QT_CANVAS3D_PREFIX, 1, 1));

    if (qtVersion().matches(5, 6))
        return features;

    features.unite(versionedIds(Constants::FEATURE_QT_QUICK_PREFIX, 2, 7));
    features.unite(versionedIds(Constants::FEATURE_QT_QUICK_CONTROLS_2_PREFIX, 2, 0));
    features.subtract(versionedIds(Constants::FEATURE_QT_LABS_CONTROLS_PREFIX, 1, 0));

    if (qtVersion().matches(5, 7))
        return features;

    features.unite(versionedIds(Constants::FEATURE_QT_QUICK_PREFIX, 2, 8));
    features.unite(versionedIds(Constants::FEATURE_QT_QUICK_CONTROLS_2_PREFIX, 2, 1));

    if (qtVersion().matches(5, 8))
        return features;

    features.unite(versionedIds(Constants::FEATURE_QT_QUICK_PREFIX, 2, 9));
    features.unite(versionedIds(Constants::FEATURE_QT_QUICK_CONTROLS_2_PREFIX, 2, 2));

    if (qtVersion().matches(5, 9))
        return features;

    features.unite(versionedIds(Constants::FEATURE_QT_QUICK_PREFIX, 2, 10));
    features.unite(versionedIds(Constants::FEATURE_QT_QUICK_CONTROLS_2_PREFIX, 2, 3));

    if (qtVersion().matches(5, 10))
        return features;

    features.unite(versionedIds(Constants::FEATURE_QT_QUICK_PREFIX, 2, 11));
    features.unite(versionedIds(Constants::FEATURE_QT_QUICK_CONTROLS_2_PREFIX, 2, 4));

    if (qtVersion().matches(5, 11))
        return features;

    features.unite(versionedIds(Constants::FEATURE_QT_QUICK_PREFIX, 2, 12));
    features.unite(versionedIds(Constants::FEATURE_QT_QUICK_CONTROLS_2_PREFIX, 2, 5));

    if (qtVersion().matches(5, 12))
        return features;

    features.unite(versionedIds(Constants::FEATURE_QT_QUICK_PREFIX, 2, 13));
    features.unite(versionedIds(Constants::FEATURE_QT_QUICK_CONTROLS_2_PREFIX, 2, 13));

    if (qtVersion().matches(5, 13))
        return features;

    return features;
}

QList<Task> BaseQtVersion::validateKit(const Kit *k)
{
    QList<Task> result;

    BaseQtVersion *version = QtKitInformation::qtVersion(k);
    Q_ASSERT(version == this);

    const QList<Abi> qtAbis = version->qtAbis();
    if (qtAbis.isEmpty()) // No need to test if Qt does not know anyway...
        return result;

    const Id dt = DeviceTypeKitInformation::deviceTypeId(k);
    const QSet<Id> tdt = targetDeviceTypes();
    if (!tdt.isEmpty() && !tdt.contains(dt)) {
        result << Task(Task::Warning,
                       QCoreApplication::translate("BaseQtVersion",
                                                   "Device type is not supported by Qt version."),
                       FileName(), -1, ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);
    }

    ToolChain *tc = ToolChainKitInformation::toolChain(k, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    if (tc) {
        Abi targetAbi = tc->targetAbi();
        bool fuzzyMatch = false;
        bool fullMatch = false;

        QString qtAbiString;
        foreach (const Abi &qtAbi, qtAbis) {
            if (!qtAbiString.isEmpty())
                qtAbiString.append(' ');
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

FileName BaseQtVersion::qmlPath() const
{
    return FileName::fromUserInput(qmakeProperty("QT_INSTALL_QML"));
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
        result.appendPath("mkspecs");
    return result;
}

FileName BaseQtVersion::qmlBinPath() const
{
    return FileName::fromUserInput(m_mkspecValues.value("QT.qml.bins"));
}

FileName BaseQtVersion::librarySearchPath() const
{
    return HostOsInfo::isWindowsHost()
            ? FileName::fromUserInput(qmakeProperty("QT_INSTALL_BINS")) : libraryPath();
}

FileNameList BaseQtVersion::directoriesToIgnoreInProjectTree() const
{
    FileNameList result;
    const FileName mkspecPathGet = mkspecsPath();
    result.append(mkspecPathGet);

    FileName mkspecPathSrc = FileName::fromUserInput(qmakeProperty("QT_HOST_DATA", PropertyVariantSrc));
    if (!mkspecPathSrc.isEmpty()) {
        mkspecPathSrc.appendPath("mkspecs");
        if (mkspecPathSrc != mkspecPathGet)
            result.append(mkspecPathSrc);
    }

    return result;
}

QString BaseQtVersion::qtNamespace() const
{
    ensureMkSpecParsed();
    return m_mkspecValues.value(MKSPEC_VALUE_NAMESPACE);
}

QString BaseQtVersion::qtLibInfix() const
{
    ensureMkSpecParsed();
    return m_mkspecValues.value(MKSPEC_VALUE_LIBINFIX);
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
    m_id = map.value(Constants::QTVERSIONID).toInt();
    if (m_id == -1) // this happens on adding from installer, see updateFromInstaller => get a new unique id
        m_id = QtVersionManager::getUniqueId();
    m_unexpandedDisplayName = map.value(Constants::QTVERSIONNAME).toString();
    m_isAutodetected = map.value(QTVERSIONAUTODETECTED).toBool();
    if (m_isAutodetected)
        m_autodetectionSource = map.value(QTVERSIONAUTODETECTIONSOURCE).toString();
    m_overrideFeatures = Core::Id::fromStringList(map.value(QTVERSION_OVERRIDE_FEATURES).toStringList());
    QString string = map.value(QTVERSIONQMAKEPATH).toString();
    if (string.startsWith('~'))
        string.remove(0, 1).prepend(QDir::homePath());

    m_qtSources = Utils::FileName::fromUserInput(
                map.value(QTVERSIONSOURCEPATH).toString());

    // Handle ABIs provided by the SDKTool:
    // Note: Creator does not write these settings itself, so it has to come from the SDKTool!
    m_qtAbis = Utils::transform(map.value(QTVERSION_ABIS, QStringList()).toStringList(), &Abi::fromString);
    m_qtAbis = Utils::filtered(m_qtAbis, &Abi::isValid);
    m_hasQtAbis = !m_qtAbis.isEmpty();

    QFileInfo fi(string);
    if (BuildableHelperLibrary::isQtChooser(fi)) {
        // we don't want to treat qtchooser as a normal qmake
        // see e.g. QTCREATORBUG-9841, also this lead to users changing what
        // qtchooser forwards too behind our backs, which will inadvertly lead to bugs
        string = BuildableHelperLibrary::qtChooserToQmakePath(fi.symLinkTarget());
    }

    m_qmakeCommand = Utils::FileName::fromString(string);
}

QVariantMap BaseQtVersion::toMap() const
{
    QVariantMap result;
    result.insert(Constants::QTVERSIONID, uniqueId());
    result.insert(Constants::QTVERSIONNAME, unexpandedDisplayName());
    result.insert(QTVERSIONAUTODETECTED, isAutodetected());
    if (isAutodetected())
        result.insert(QTVERSIONAUTODETECTIONSOURCE, autodetectionSource());
    if (!m_overrideFeatures.isEmpty())
        result.insert(QTVERSION_OVERRIDE_FEATURES, Core::Id::toStringList(m_overrideFeatures));

    result.insert(QTVERSIONQMAKEPATH, qmakeCommand().toString());
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

BaseQtVersion::Predicate BaseQtVersion::isValidPredicate(const BaseQtVersion::Predicate &predicate)
{
    if (predicate)
        return [predicate](const BaseQtVersion *v) { return v->isValid() && predicate(v); };
    return [](const BaseQtVersion *v) { return v->isValid(); };
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
    if (m_versionInfo.value(ProKey("QT_INSTALL_PREFIX/get"))
        != m_versionInfo.value(ProKey("QT_INSTALL_PREFIX"))) {
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
    return macroExpander()->expand(m_unexpandedDisplayName);
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
            const QHash<ProKey, ProString> vInfo = versionInfo();
            if (!vInfo.isEmpty()) {
                QList<ProKey> keys = vInfo.keys();
                Utils::sort(keys);
                foreach (const ProKey &key, keys) {
                    const QString &value = vInfo.value(key).toQString();
                    QString variableName = key.toQString();
                    if (variableName != "QMAKE_MKSPECS"
                        && !variableName.endsWith("/raw")) {
                        bool isPath = false;
                        if (variableName.contains("_HOST_")
                            || variableName.contains("_INSTALL_")) {
                            if (!variableName.endsWith("/get"))
                                continue;
                            variableName.chop(4);
                            isPath = true;
                        } else if (variableName == "QT_SYSROOT") {
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

FileName BaseQtVersion::qtPackageSourcePath() const
{
    return m_qtSources;
}

QString BaseQtVersion::designerCommand() const
{
    if (!isValid())
        return QString();
    if (m_designerCommand.isNull())
        m_designerCommand = findHostBinary(Designer);
    return m_designerCommand;
}

QString BaseQtVersion::linguistCommand() const
{
    if (!isValid())
        return QString();
    if (m_linguistCommand.isNull())
        m_linguistCommand = findHostBinary(Linguist);
    return m_linguistCommand;
}

QString BaseQtVersion::qscxmlcCommand() const
{
    if (!isValid())
        return QString();

    if (m_qscxmlcCommand.isNull())
        m_qscxmlcCommand = findHostBinary(QScxmlc);
    return m_qscxmlcCommand;
}

QString BaseQtVersion::findHostBinary(HostBinaries binary) const
{
    QString baseDir;
    if (qtVersion() < QtVersionNumber(5, 0, 0)) {
        baseDir = qmakeProperty("QT_HOST_BINS");
    } else {
        ensureMkSpecParsed();
        switch (binary) {
        case Designer:
        case Linguist:
            baseDir = m_mkspecValues.value("QT.designer.bins");
            break;
        case Uic:
        case QScxmlc:
            baseDir = qmakeProperty("QT_HOST_BINS");
            break;
        default:
            // Can't happen
            Q_ASSERT(false);
        }
    }

    if (baseDir.isEmpty())
        return QString();
    if (!baseDir.endsWith('/'))
        baseDir += '/';

    QStringList possibleCommands;
    switch (binary) {
    case Designer:
        if (HostOsInfo::isMacHost())
            possibleCommands << "Designer.app/Contents/MacOS/Designer";
        else
            possibleCommands << HostOsInfo::withExecutableSuffix("designer");
        break;
    case Linguist:
        if (HostOsInfo::isMacHost())
            possibleCommands << "Linguist.app/Contents/MacOS/Linguist";
        else
            possibleCommands << HostOsInfo::withExecutableSuffix("linguist");
        break;
    case Uic:
        if (HostOsInfo::isWindowsHost()) {
            possibleCommands << "uic.exe";
        } else {
            possibleCommands << "uic-qt4" << "uic4" << "uic";
        }
        break;
    case QScxmlc:
        possibleCommands << HostOsInfo::withExecutableSuffix("qscxmlc");
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
    m_uicCommand = findHostBinary(Uic);
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
        FileName sourceMkSpecPath = sourcePath().appendPath("mkspecs");
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
    QMakeGlobals option;
    applyProperties(&option);
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
    m_configValues = evaluator->values("CONFIG");
    m_qtConfigValues = evaluator->values("QT_CONFIG");
    m_defaultConfigIsDebugAndRelease = false;
    m_frameworkBuild = false;
    foreach (const QString &value, m_configValues) {
        if (value == "debug")
            m_defaultConfigIsDebug = true;
        else if (value == "release")
            m_defaultConfigIsDebug = false;
        else if (value == "build_all")
            m_defaultConfigIsDebugAndRelease = true;
        else if (value == "qt_framework")
            m_frameworkBuild = true;
    }
    const QString designerBins = "QT.designer.bins";
    const QString qmlBins = "QT.qml.bins";
    const QString declarativeBins = "QT.declarative.bins";
    const QString libinfix = MKSPEC_VALUE_LIBINFIX;
    const QString ns = MKSPEC_VALUE_NAMESPACE;
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
    if (spec.isEmpty())
        return true; // default spec of a Qt version

    QDir mkspecDir = QDir(QDir::fromNativeSeparators(qmakeProperty("QT_HOST_DATA"))
                          + "/mkspecs/");
    const QString absSpec = mkspecDir.absoluteFilePath(spec.toString());
    if (QFileInfo(absSpec).isDir() && QFileInfo(absSpec + "/qmake.conf").isFile())
        return true;
    mkspecDir.setPath(sourcePath().toString() + "/mkspecs/");
    const QString absSrcSpec = mkspecDir.absoluteFilePath(spec.toString());
    return absSrcSpec != absSpec
            && QFileInfo(absSrcSpec).isDir()
            && QFileInfo(absSrcSpec + "/qmake.conf").isFile();
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


    const QString qtInstallBins = qmakeProperty(m_versionInfo, "QT_INSTALL_BINS");
    const QString qtHeaderData = qmakeProperty(m_versionInfo, "QT_INSTALL_HEADERS");

    if (!qtInstallBins.isNull()) {
        if (!qtInstallBins.isEmpty()) {
            m_hasQmlDump
                    = !QmlDumpTool::toolForQtPaths(qtInstallBins, false).isEmpty()
                    || !QmlDumpTool::toolForQtPaths(qtInstallBins, true).isEmpty();
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

QHash<ProKey,ProString> BaseQtVersion::versionInfo() const
{
    updateVersionInfo();
    return m_versionInfo;
}

QString BaseQtVersion::qmakeProperty(const QHash<ProKey,ProString> &versionInfo, const QByteArray &name,
                                     PropertyVariant variant)
{
    QString val = versionInfo.value(ProKey(QString::fromLatin1(
            name + (variant == PropertyVariantDev ? "/dev" :
                    variant == PropertyVariantGet ? "/get" : "/src")))).toQString();
    if (!val.isNull())
        return val;
    return versionInfo.value(ProKey(name)).toQString();
}

QString BaseQtVersion::qmakeProperty(const QByteArray &name, PropertyVariant variant) const
{
    updateVersionInfo();
    return qmakeProperty(m_versionInfo, name, variant);
}

void BaseQtVersion::applyProperties(QMakeGlobals *qmakeGlobals) const
{
    qmakeGlobals->setProperties(versionInfo());
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
    return m_expander.macroExpander(this);
}

std::unique_ptr<MacroExpander>
BaseQtVersion::createMacroExpander(const std::function<const BaseQtVersion *()> &qtVersion)
{
    const auto versionProperty =
        [qtVersion](const std::function<QString(const BaseQtVersion *)> &property) {
            return [property, qtVersion]() -> QString {
                const BaseQtVersion *version = qtVersion();
                return version ? property(version) : QString();
            };
        };
    std::unique_ptr<Utils::MacroExpander> expander(new Utils::MacroExpander);
    expander->setDisplayName(QtKitInformation::tr("Qt version"));

    expander->registerVariable(
        "Qt:Version",
        QtKitInformation::tr("The version string of the current Qt version."),
        versionProperty([](const BaseQtVersion *version) {
            return version->qtVersionString();
        }));

    expander->registerVariable(
        "Qt:Type",
        QtKitInformation::tr("The type of the current Qt version."),
        versionProperty([](const BaseQtVersion *version) {
            return version->type();
        }));

    expander->registerVariable(
        "Qt:Mkspec",
        QtKitInformation::tr("The mkspec of the current Qt version."),
        versionProperty([](const BaseQtVersion *version) {
            return version->mkspec().toUserOutput();
        }));

    expander->registerVariable(
        "Qt:QT_INSTALL_PREFIX",
        QtKitInformation::tr("The installation prefix of the current Qt version."),
        versionProperty([](const BaseQtVersion *version) {
            return version->qmakeProperty(version->m_versionInfo, "QT_INSTALL_PREFIX");
        }));

    expander->registerVariable(
        "Qt:QT_INSTALL_DATA",
        QtKitInformation::tr("The installation location of the current Qt version's data."),
        versionProperty([](const BaseQtVersion *version) {
            return version->qmakeProperty(version->m_versionInfo, "QT_INSTALL_DATA");
        }));

    expander->registerVariable(
        "Qt:QT_INSTALL_HEADERS",
        QtKitInformation::tr("The installation location of the current Qt version's header files."),
        versionProperty([](const BaseQtVersion *version) {
            return version->qmakeProperty(version->m_versionInfo, "QT_INSTALL_HEADERS");
        }));

    expander->registerVariable(
        "Qt:QT_INSTALL_LIBS",
        QtKitInformation::tr("The installation location of the current Qt version's library files."),
        versionProperty([](const BaseQtVersion *version) {
            return version->qmakeProperty(version->m_versionInfo, "QT_INSTALL_LIBS");
        }));

    expander->registerVariable(
        "Qt:QT_INSTALL_DOCS",
        QtKitInformation::tr("The installation location of the current Qt version's documentation files."),
        versionProperty([](const BaseQtVersion *version) {
            return version->qmakeProperty(version->m_versionInfo, "QT_INSTALL_DOCS");
        }));

    expander->registerVariable(
        "Qt:QT_INSTALL_BINS",
        QtKitInformation::tr("The installation location of the current Qt version's executable files."),
        versionProperty([](const BaseQtVersion *version) {
            return version->qmakeProperty(version->m_versionInfo, "QT_INSTALL_BINS");
        }));

    expander->registerVariable(
        "Qt:QT_INSTALL_PLUGINS",
        QtKitInformation::tr("The installation location of the current Qt version's plugins."),
        versionProperty([](const BaseQtVersion *version) {
            return version->qmakeProperty(version->m_versionInfo, "QT_INSTALL_PLUGINS");
        }));

    expander->registerVariable(
        "Qt:QT_INSTALL_QML",
        QtKitInformation::tr("The installation location of the current Qt version's QML files."),
        versionProperty([](const BaseQtVersion *version) {
            return version->qmakeProperty(version->m_versionInfo, "QT_INSTALL_QML");
        }));

    expander->registerVariable(
        "Qt:QT_INSTALL_IMPORTS",
        QtKitInformation::tr("The installation location of the current Qt version's imports."),
        versionProperty([](const BaseQtVersion *version) {
            return version->qmakeProperty(version->m_versionInfo, "QT_INSTALL_IMPORTS");
        }));

    expander->registerVariable(
        "Qt:QT_INSTALL_TRANSLATIONS",
        QtKitInformation::tr("The installation location of the current Qt version's translation files."),
        versionProperty([](const BaseQtVersion *version) {
            return version->qmakeProperty(version->m_versionInfo, "QT_INSTALL_TRANSLATIONS");
        }));

    expander->registerVariable(
        "Qt:QT_INSTALL_CONFIGURATION",
        QtKitInformation::tr("The installation location of the current Qt version's translation files."),
        versionProperty([](const BaseQtVersion *version) {
            return version->qmakeProperty(version->m_versionInfo, "QT_INSTALL_CONFIGURATION");
        }));

    expander->registerVariable(
        "Qt:QT_INSTALL_EXAMPLES",
        QtKitInformation::tr("The installation location of the current Qt version's examples."),
        versionProperty([](const BaseQtVersion *version) {
            return version->qmakeProperty(version->m_versionInfo, "QT_INSTALL_EXAMPLES");
        }));

    expander->registerVariable(
        "Qt:QT_INSTALL_DEMOS",
        QtKitInformation::tr("The installation location of the current Qt version's demos."),
        versionProperty([](const BaseQtVersion *version) {
            return version->qmakeProperty(version->m_versionInfo, "QT_INSTALL_DEMOS");
        }));

    expander->registerVariable(
        "Qt:QMAKE_MKSPECS",
        QtKitInformation::tr("The current Qt version's default mkspecs (Qt 4)."),
        versionProperty([](const BaseQtVersion *version) {
            return version->qmakeProperty(version->m_versionInfo, "QMAKE_MKSPECS");
        }));

    expander->registerVariable(
        "Qt:QMAKE_SPEC",
        QtKitInformation::tr("The current Qt version's default mkspec (Qt 5; host system)."),
        versionProperty([](const BaseQtVersion *version) {
            return version->qmakeProperty(version->m_versionInfo, "QMAKE_SPEC");
        }));

    expander->registerVariable(
        "Qt:QMAKE_XSPEC",
        QtKitInformation::tr("The current Qt version's default mkspec (Qt 5; target system)."),
        versionProperty([](const BaseQtVersion *version) {
            return version->qmakeProperty(version->m_versionInfo, "QMAKE_XSPEC");
        }));

    expander->registerVariable(
        "Qt:QMAKE_VERSION",
        QtKitInformation::tr("The current Qt's qmake version."),
        versionProperty([](const BaseQtVersion *version) {
            return version->qmakeProperty(version->m_versionInfo, "QMAKE_VERSION");
        }));

    //    FIXME: Re-enable once we can detect expansion loops.
    //    expander->registerVariable("Qt:Name",
    //        QtKitInformation::tr("The display name of the current Qt version."),
    //        versionProperty([](BaseQtVersion *version) {
    //            return version->displayName();
    //        }));

    return expander;
}

void BaseQtVersion::populateQmlFileFinder(FileInProjectFinder *finder, const Target *target)
{
    // If target given, then use the project associated with that ...
    const ProjectExplorer::Project *startupProject = target ? target->project() : nullptr;

    // ... else try the session manager's global startup project ...
    if (!startupProject)
        startupProject = ProjectExplorer::SessionManager::startupProject();

    // ... and if that is null, use the first project available.
    const QList<ProjectExplorer::Project *> projects = ProjectExplorer::SessionManager::projects();
    QTC_CHECK(projects.isEmpty() || startupProject);

    Utils::FileName projectDirectory;
    Utils::FileNameList sourceFiles;

    // Sort files from startupProject to the front of the list ...
    if (startupProject) {
        projectDirectory = startupProject->projectDirectory();
        sourceFiles.append(startupProject->files(ProjectExplorer::Project::SourceFiles));
    }

    // ... then add all the other projects' files.
    for (const ProjectExplorer::Project *project : projects) {
        if (project != startupProject)
            sourceFiles.append(project->files(ProjectExplorer::Project::SourceFiles));
    }

    // If no target was given, but we've found a startupProject, then try to deduce a
    // target from that.
    if (!target && startupProject)
        target = startupProject->activeTarget();

    // ... and find the sysroot and qml directory if we have any target at all.
    const ProjectExplorer::Kit *kit = target ? target->kit() : nullptr;
    const Utils::FileName activeSysroot = ProjectExplorer::SysRootKitInformation::sysRoot(kit);
    const QtSupport::BaseQtVersion *qtVersion = QtVersionManager::isLoaded()
            ? QtSupport::QtKitInformation::qtVersion(kit) : nullptr;
    Utils::FileNameList additionalSearchDirectories = qtVersion
            ? Utils::FileNameList({qtVersion->qmlPath()}) : Utils::FileNameList();

    if (target) {
        for (const ProjectExplorer::DeployableFile &file : target->deploymentData().allFiles())
            finder->addMappedPath(file.localFilePath(), file.remoteFilePath());
    }

    // Add resource paths to the mapping
    if (startupProject) {
        if (ProjectExplorer::ProjectNode *rootNode = startupProject->rootProjectNode()) {
            rootNode->forEachNode([&](ProjectExplorer::FileNode *node) {
                if (auto resourceNode = dynamic_cast<ResourceEditor::ResourceFileNode *>(node))
                    finder->addMappedPath(node->filePath(), ":" + resourceNode->qrcPath());
            });
        } else {
            // Can there be projects without root node?
        }
    }

    // Finally, do populate m_projectFinder
    finder->setProjectDirectory(projectDirectory);
    finder->setProjectFiles(sourceFiles);
    finder->setSysroot(activeSysroot);
    finder->setAdditionalSearchDirectories(additionalSearchDirectories);
}

QSet<Id> BaseQtVersion::features() const
{
    if (m_overrideFeatures.isEmpty())
        return availableFeatures();
    return m_overrideFeatures;
}

void BaseQtVersion::addToEnvironment(const Kit *k, Environment &env) const
{
    Q_UNUSED(k);
    env.set("QTDIR", QDir::toNativeSeparators(qmakeProperty("QT_HOST_DATA")));
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

QString BaseQtVersion::qmlDumpTool(bool debugVersion) const
{
    const QString qtInstallBins = qmakeProperty("QT_INSTALL_BINS");
    return QmlDumpTool::toolForQtPaths(qtInstallBins, debugVersion);
}

void BaseQtVersion::recheckDumper()
{
    m_versionInfoUpToDate = false;
}

QList<Task> BaseQtVersion::reportIssuesImpl(const QString &proFile, const QString &buildDir) const
{
    Q_UNUSED(proFile);
    Q_UNUSED(buildDir);
    QList<Task> results;

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
    return nullptr;
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
    process.start(binary.toString(), QStringList("-query"), QIODevice::ReadOnly);

    if (!process.waitForStarted()) {
        *error = QCoreApplication::translate("QtVersion", "Cannot start \"%1\": %2").arg(binary.toUserOutput()).arg(process.errorString());
        return QByteArray();
    }
    if (!process.waitForFinished(timeOutMS) && process.state() == QProcess::Running) {
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
                                        QHash<ProKey, ProString> *versionInfo, QString *error)
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
        const QList<ToolChain *> tcList
                = ToolChainManager::toolChains([&abiList](const ToolChain *t) { return abiList.contains(t->targetAbi()); });
        for (ToolChain *tc : tcList) {
            Environment realEnv = env;
            tc->addToEnvironment(realEnv);
            output = runQmakeQuery(binary, realEnv, error);
            if (error->isEmpty())
                break;
        }
    }

    if (output.isNull())
        return false;

    QMakeGlobals::parseProperties(output, *versionInfo);

    return true;
}

FileName BaseQtVersion::mkspecDirectoryFromVersionInfo(const QHash<ProKey, ProString> &versionInfo)
{
    QString dataDir = qmakeProperty(versionInfo, "QT_HOST_DATA", PropertyVariantSrc);
    if (dataDir.isEmpty())
        return FileName();
    return FileName::fromUserInput(dataDir + "/mkspecs");
}

FileName BaseQtVersion::mkspecFromVersionInfo(const QHash<ProKey, ProString> &versionInfo)
{
    FileName baseMkspecDir = mkspecDirectoryFromVersionInfo(versionInfo);
    if (baseMkspecDir.isEmpty())
        return FileName();

    bool qt5 = false;
    QString theSpec = qmakeProperty(versionInfo, "QMAKE_XSPEC");
    if (theSpec.isEmpty())
        theSpec = "default";
    else
        qt5 = true;

    FileName mkspecFullPath = baseMkspecDir;
    mkspecFullPath.appendPath(theSpec);

    // qDebug() << "default mkspec is located at" << mkspecFullPath;

    if (HostOsInfo::isWindowsHost()) {
        if (!qt5) {
            QFile f2(mkspecFullPath.toString() + "/qmake.conf");
            if (f2.exists() && f2.open(QIODevice::ReadOnly)) {
                while (!f2.atEnd()) {
                    QByteArray line = f2.readLine();
                    if (line.startsWith("QMAKESPEC_ORIGINAL")) {
                        const QList<QByteArray> &temp = line.split('=');
                        if (temp.size() == 2) {
                            QString possibleFullPath = QString::fromLocal8Bit(temp.at(1).trimmed().constData());
                            if (possibleFullPath.contains('$')) { // QTBUG-28792
                                const QRegularExpression rex("\\binclude\\(([^)]+)/qmake\\.conf\\)");
                                const QRegularExpressionMatch match = rex.match(QString::fromLocal8Bit(f2.readAll()));
                                if (match.hasMatch()) {
                                    possibleFullPath = mkspecFullPath.toString() + '/'
                                            + match.captured(1);
                                }
                            }
                            // We sometimes get a mix of different slash styles here...
                            possibleFullPath = possibleFullPath.replace('\\', '/');
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
            QFile f2(mkspecFullPath.toString() + "/qmake.conf");
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
                                return baseMkspecDir.appendPath("macx-g++");
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
            QString rspec = mkspecFullPath.toFileInfo().symLinkTarget();
            if (!rspec.isEmpty())
                mkspecFullPath = FileName::fromUserInput(
                            QDir(baseMkspecDir.toString()).absoluteFilePath(rspec));
        }
    }
    return mkspecFullPath;
}

FileName BaseQtVersion::sourcePath(const QHash<ProKey, ProString> &versionInfo)
{
    const QString qt5Source = qmakeProperty(versionInfo, "QT_INSTALL_PREFIX/src");
    if (!qt5Source.isEmpty())
        return Utils::FileName::fromString(QFileInfo(qt5Source).canonicalFilePath());

    const QString installData = qmakeProperty(versionInfo, "QT_INSTALL_PREFIX");
    QString sourcePath = installData;
    QFile qmakeCache(installData + "/.qmake.cache");
    if (qmakeCache.exists() && qmakeCache.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&qmakeCache);
        while (!stream.atEnd()) {
            QString line = stream.readLine().trimmed();
            if (line.startsWith("QT_SOURCE_TREE")) {
                sourcePath = line.split('=').at(1).trimmed();
                if (sourcePath.startsWith("$$quote(")) {
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
    if (dir.dirName() == "qtbase")
        dir.cdUp();
    return filePath.isChildOf(dir);
}

bool BaseQtVersion::isSubProject(const Utils::FileName &filePath) const
{
    const Utils::FileName &source = sourcePath();
    if (!source.isEmpty()) {
        QDir dir = QDir(source.toString());
        if (dir.dirName() == "qtbase")
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

    const QString qtQuickCompilerPrf = mkspecsPath().toString() + "/features/qtquickcompiler.prf";
    if (!QFileInfo::exists(qtQuickCompilerPrf)) {
        if (reason)
            *reason = QCoreApplication::translate("BaseQtVersion", "This Qt Version does not contain Qt Quick Compiler.");
        return false;
    }

    return true;
}

FileNameList BaseQtVersion::qtCorePaths() const
{
    updateVersionInfo();
    const QString &versionString = m_qtVersionString;

    const QString &installLibsDir = qmakeProperty(m_versionInfo, "QT_INSTALL_LIBS");
    const QString &installBinDir = qmakeProperty(m_versionInfo, "QT_INSTALL_BINS");

    const QDir::Filters filters = QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot;

    const QFileInfoList entryInfoList = [&]() {
         QFileInfoList result;
         if (!installLibsDir.isEmpty())
             result += QDir(installLibsDir).entryInfoList(filters);
        if (!installBinDir.isEmpty())
            result += QDir(installBinDir).entryInfoList(filters);
        return result;
    }();
    FileNameList staticLibs;
    FileNameList dynamicLibs;
    for (const QFileInfo &info : entryInfoList) {
        const QString file = info.fileName();
        if (info.isDir()
                && file.startsWith("QtCore")
                && file.endsWith(".framework")) {
            // handle Framework
            FileName lib(info);
            dynamicLibs.append(lib.appendPath(file.left(file.lastIndexOf('.'))));
        } else if (info.isReadable()) {
            if (file.startsWith("libQtCore")
                    || file.startsWith("libQt5Core")
                    || file.startsWith("QtCore")
                    || file.startsWith("Qt5Core")) {
                if (file.endsWith(".a") || file.endsWith(".lib"))
                    staticLibs.append(FileName(info));
                else if (file.endsWith(".dll")
                         || file.endsWith(QString::fromLatin1(".so.") + versionString)
                         || file.endsWith(".so")
                         || file.endsWith(QLatin1Char('.') + versionString + ".dylib"))
                    dynamicLibs.append(FileName(info));
            }
        }
    }
    // Only handle static libs if we cannot find dynamic ones:
    if (dynamicLibs.isEmpty())
        return staticLibs;
    return dynamicLibs;
}

static QByteArray scanQtBinaryForBuildString(const FileName &library)
{
    QFile lib(library.toString());
    QByteArray buildString;

    if (lib.open(QIODevice::ReadOnly)) {
        const QByteArray startNeedle = "Qt ";
        const QByteArray buildNeedle = " build; by ";
        const size_t oneMiB = 1024 * 1024;
        const size_t keepSpace = 4096;
        const size_t bufferSize = oneMiB + keepSpace;
        QByteArray buffer(bufferSize, Qt::Uninitialized);

        char *const readStart = buffer.data() + keepSpace;
        auto readStartIt = buffer.begin() + keepSpace;
        const auto copyStartIt = readStartIt + (oneMiB - keepSpace);

        while (!lib.atEnd()) {
            const int read = lib.read(readStart, static_cast<int>(oneMiB));
            const auto readEndIt = readStart + read;
            auto currentIt = readStartIt;

            forever {
                const auto qtFoundIt = std::search(currentIt, readEndIt,
                                                   startNeedle.begin(), startNeedle.end());
                if (qtFoundIt == readEndIt)
                    break;

                currentIt = qtFoundIt + 1;

                // Found "Qt ", now find the next '\0'.
                const auto nullFoundIt = std::find(qtFoundIt, readEndIt, '\0');
                if (nullFoundIt == readEndIt)
                    break;

                // String much too long?
                const size_t len = std::distance(qtFoundIt, nullFoundIt);
                if (len > keepSpace)
                    continue;

                // Does it contain " build; by "?
                const auto buildByFoundIt = std::search(qtFoundIt, nullFoundIt,
                                                        buildNeedle.begin(), buildNeedle.end());
                if (buildByFoundIt == nullFoundIt)
                    continue;

                buildString = QByteArray(qtFoundIt, static_cast<int>(len));
                break;
            }

            if (!buildString.isEmpty() || readEndIt != buffer.constEnd())
                break;

            std::move(copyStartIt, readEndIt, buffer.begin()); // Copy last section to front.
        }
    }
    return buildString;
}

static QStringList extractFieldsFromBuildString(const QByteArray &buildString)
{
    if (buildString.isEmpty()
            || buildString.count() > 4096)
        return QStringList();

    const QRegularExpression buildStringMatcher("^Qt "
                                                "([\\d\\.a-zA-Z]*) " // Qt version
                                                "\\("
                                                "([\\w_-]+) "       // Abi information
                                                "(shared|static) (?:\\(dynamic\\) )?"
                                                "(debug|release)"
                                                " build; by "
                                                "(.*)"               // compiler with extra info
                                                "\\)$");

    QTC_ASSERT(buildStringMatcher.isValid(), qWarning() << buildStringMatcher.errorString());

    const QRegularExpressionMatch match = buildStringMatcher.match(QString::fromUtf8(buildString));
    if (!match.hasMatch())
        return QStringList();

    QStringList result;
    result.append(match.captured(1)); // qtVersion

    // Abi info string:
    QStringList abiInfo = match.captured(2).split('-', QString::SkipEmptyParts);

    result.append(abiInfo.takeFirst()); // cpu

    const QString endian = abiInfo.takeFirst();
    QTC_ASSERT(endian.endsWith("_endian"), return QStringList());
    result.append(endian.left(endian.count() - 7)); // without the "_endian"

    result.append(abiInfo.takeFirst()); // pointer

    if (abiInfo.isEmpty()) {
        // no extra info whatsoever:
        result.append(""); // qreal is unset
        result.append(""); // extra info is unset
    } else {
        const QString next = abiInfo.at(0);
        if (next.startsWith("qreal_")) {
            abiInfo.takeFirst();
            result.append(next.mid(6)); // qreal: without the "qreal_" part;
        } else {
            result.append(""); // qreal is unset!
        }

        result.append(abiInfo.join('-')); // extra abi strings
    }

    result.append(match.captured(3)); // linkage
    result.append(match.captured(4)); // buildType
    result.append(match.captured(5)); // compiler

    return result;
}

static Abi refineAbiFromBuildString(const QByteArray &buildString, const Abi &probableAbi)
{
    QStringList buildStringData = extractFieldsFromBuildString(buildString);
    if (buildStringData.count() != 9)
        return probableAbi;

    const QString compiler = buildStringData.at(8);

    Abi::Architecture arch = probableAbi.architecture();
    Abi::OS os = probableAbi.os();
    Abi::OSFlavor flavor = probableAbi.osFlavor();
    Abi::BinaryFormat format = probableAbi.binaryFormat();
    unsigned char wordWidth = probableAbi.wordWidth();

    if (compiler.startsWith("GCC ") && os == Abi::WindowsOS) {
        flavor = Abi::WindowsMSysFlavor;
    } else if (compiler.startsWith("MSVC 2005")  && os == Abi::WindowsOS) {
        flavor = Abi::WindowsMsvc2005Flavor;
    } else if (compiler.startsWith("MSVC 2008") && os == Abi::WindowsOS) {
        flavor = Abi::WindowsMsvc2008Flavor;
    } else if (compiler.startsWith("MSVC 2010") && os == Abi::WindowsOS) {
        flavor = Abi::WindowsMsvc2010Flavor;
    } else if (compiler.startsWith("MSVC 2012") && os == Abi::WindowsOS) {
        flavor = Abi::WindowsMsvc2012Flavor;
    } else if (compiler.startsWith("MSVC 2015") && os == Abi::WindowsOS) {
        flavor = Abi::WindowsMsvc2015Flavor;
    } else if (compiler.startsWith("MSVC 2017") && os == Abi::WindowsOS) {
        flavor = Abi::WindowsMsvc2017Flavor;
    } else if (compiler.startsWith("MSVC 2019") && os == Abi::WindowsOS) {
        flavor = Abi::WindowsMsvc2019Flavor;
    }

    return Abi(arch, os, flavor, format, wordWidth);
}

static Abi scanQtBinaryForBuildStringAndRefineAbi(const FileName &library,
                                                   const Abi &probableAbi)
{
    static QHash<Utils::FileName, Abi> results;

    if (!results.contains(library)) {
        const QByteArray buildString = scanQtBinaryForBuildString(library);
        results.insert(library, refineAbiFromBuildString(buildString, probableAbi));
    }
    return results.value(library);
}

QList<Abi> BaseQtVersion::qtAbisFromLibrary(const FileNameList &coreLibraries)
{
    QList<Abi> res;
    foreach (const FileName &library, coreLibraries) {
        for (Abi abi : Abi::abisOfBinary(library)) {
            Abi tmp = abi;
            if (abi.osFlavor() == Abi::UnknownFlavor)
                tmp = scanQtBinaryForBuildStringAndRefineAbi(library, abi);
            if (!res.contains(tmp))
                res.append(tmp);
        }
    }
    return res;
}

#if defined(WITH_TESTS)

#include <QTest>

#include "qtsupportplugin.h"

void QtSupportPlugin::testQtBuildStringParsing_data()
{
    QTest::addColumn<QByteArray>("buildString");
    QTest::addColumn<QString>("expected");

    QTest::newRow("invalid build string")
            << QByteArray("Qt with invalid buildstring") << QString();
    QTest::newRow("empty build string")
            << QByteArray("") << QString();
    QTest::newRow("huge build string")
            << QByteArray(8192, 'x') << QString();

    QTest::newRow("valid build string")
            << QByteArray("Qt 5.7.1 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 6.2.1 20160830)")
            << "5.7.1;x86_64;little;lp64;;;shared;release;GCC 6.2.1 20160830";

    QTest::newRow("with qreal")
            << QByteArray("Qt 5.7.1 (x86_64-little_endian-lp64-qreal___fp16 shared (dynamic) release build; by GCC 6.2.1 20160830)")
            << "5.7.1;x86_64;little;lp64;__fp16;;shared;release;GCC 6.2.1 20160830";
    QTest::newRow("with qreal and abi")
            << QByteArray("Qt 5.7.1 (x86_64-little_endian-lp64-qreal___fp16-eabi shared (dynamic) release build; by GCC 6.2.1 20160830)")
            << "5.7.1;x86_64;little;lp64;__fp16;eabi;shared;release;GCC 6.2.1 20160830";
    QTest::newRow("with qreal, eabi and softfloat")
            << QByteArray("Qt 5.7.1 (x86_64-little_endian-lp64-qreal___fp16-eabi-softfloat shared (dynamic) release build; by GCC 6.2.1 20160830)")
            << "5.7.1;x86_64;little;lp64;__fp16;eabi-softfloat;shared;release;GCC 6.2.1 20160830";
    QTest::newRow("with eabi")
            << QByteArray("Qt 5.7.1 (x86_64-little_endian-lp64-eabi shared (dynamic) release build; by GCC 6.2.1 20160830)")
            << "5.7.1;x86_64;little;lp64;;eabi;shared;release;GCC 6.2.1 20160830";
    QTest::newRow("with eabi and softfloat")
            << QByteArray("Qt 5.7.1 (x86_64-little_endian-lp64-eabi-softfloat shared (dynamic) release build; by GCC 6.2.1 20160830)")
            << "5.7.1;x86_64;little;lp64;;eabi-softfloat;shared;release;GCC 6.2.1 20160830";
}

void QtSupportPlugin::testQtBuildStringParsing()
{
    QFETCH(QByteArray, buildString);
    QFETCH(QString, expected);

    QStringList expectedList;
    if (!expected.isEmpty())
        expectedList = expected.split(';');

    QStringList actual = extractFieldsFromBuildString(buildString);
    QCOMPARE(expectedList, actual);
}

#endif // WITH_TESTS
