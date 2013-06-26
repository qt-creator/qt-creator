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

#include "qbsprojectmanager.h"

#include "qbslogsink.h"
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"
#include "qbsprojectmanagerplugin.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <qmljstools/qmljstoolsconstants.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/qtcassert.h>

#include <QVariantMap>

#include <qbs.h>

// qbs settings structure:
const char PROFILE_LIST[] = "preferences.qtcreator.kit.";
const char PROFILES_PREFIX[] = "profiles.";

// Qt related settings:
const char QTCORE_BINPATH[] = ".Qt.core.binPath";
const char QTCORE_BUILDVARIANT[] = ".Qt.core.buildVariant";
const char QTCORE_DOCPATH[] = ".Qt.core.docPath";
const char QTCORE_INCPATH[] = ".Qt.core.incPath";
const char QTCORE_LIBPATH[] = ".Qt.core.libPath";
const char QTCORE_VERSION[] = ".Qt.core.version";
const char QTCORE_NAMESPACE[] = ".Qt.core.namespace";
const char QTCORE_LIBINFIX[] = ".Qt.core.libInfix";
const char QTCORE_MKSPEC[] = ".Qt.core.mkspecPath";
const char QTCORE_FRAMEWORKBUILD[] = ".Qt.core.frameworkBuild";


// Toolchain related settings:
const char QBS_TARGETOS[] = ".qbs.targetOS";
const char QBS_SYSROOT[] = ".qbs.sysroot";
const char QBS_ARCHITECTURE[] = ".qbs.architecture";
const char QBS_ENDIANNESS[] = ".qbs.endianness";
const char QBS_TOOLCHAIN[] = ".qbs.toolchain";
const char CPP_TOOLCHAINPATH[] = ".cpp.toolchainInstallPath";
const char CPP_COMPILERNAME[] = ".cpp.compilerName";

const QChar sep = QChar(QLatin1Char('.'));

namespace QbsProjectManager {

qbs::Settings *QbsManager::m_settings = 0;
qbs::Preferences *QbsManager::m_preferences = 0;

QbsManager::QbsManager(Internal::QbsProjectManagerPlugin *plugin) :
    m_plugin(plugin)
{
    if (!m_settings)
        m_settings = new qbs::Settings(QLatin1String("QtProject"), QLatin1String("qbs"));
    if (!m_preferences)
        m_preferences = new qbs::Preferences(m_settings);

    setObjectName(QLatin1String("QbsProjectManager"));
    connect(ProjectExplorer::KitManager::instance(), SIGNAL(kitsChanged()), this, SLOT(pushKitsToQbs()));

    m_logSink = new Internal::QbsLogSink(this);
    int level = qbs::LoggerWarning;
    const QString levelEnv = QString::fromLocal8Bit(qgetenv("QBS_LOG_LEVEL"));
    if (!levelEnv.isEmpty()) {
        bool ok = false;
        int tmp = levelEnv.toInt(&ok);
        if (ok) {
            if (tmp < static_cast<int>(qbs::LoggerMinLevel))
                tmp = static_cast<int>(qbs::LoggerMinLevel);
            if (tmp > static_cast<int>(qbs::LoggerMaxLevel))
                tmp = static_cast<int>(qbs::LoggerMaxLevel);
            level = tmp;
        }
    }
    m_logSink->setLogLevel(static_cast<qbs::LoggerLevel>(level));
}

QbsManager::~QbsManager()
{
    delete m_settings;
}

QString QbsManager::mimeType() const
{
    return QLatin1String(QmlJSTools::Constants::QBS_MIMETYPE);
}

ProjectExplorer::Project *QbsManager::openProject(const QString &fileName, QString *errorString)
{
    if (!QFileInfo(fileName).isFile()) {
        if (errorString)
            *errorString = tr("Failed opening project '%1': Project is not a file")
                .arg(fileName);
        return 0;
    }

    return new Internal::QbsProject(this, fileName);
}

QString QbsManager::profileForKit(const ProjectExplorer::Kit *k) const
{
    if (!k)
        return QString();
    return m_settings->value(QString::fromLatin1(PROFILE_LIST) + k->id().toString()).toString();
}

void QbsManager::setProfileForKit(const QString &name, const ProjectExplorer::Kit *k)
{
    m_settings->setValue(QString::fromLatin1(PROFILE_LIST) + k->id().toString(), name);
}

QStringList QbsManager::profileNames() const
{
    QStringList keyList = m_settings->allKeys();

    QStringList result;
    foreach (const QString &key, keyList) {
        if (!key.startsWith(QString::fromLatin1(PROFILES_PREFIX)))
            continue;
        QString profile = key;
        profile.remove(0, QString::fromLatin1(PROFILES_PREFIX).count());
        profile = profile.left(profile.indexOf(sep));
        if (!result.contains(profile))
            result << profile;
    }
    return result;
}

qbs::Settings *QbsManager::settings()
{
    return m_settings;
}

qbs::Preferences *QbsManager::preferences()
{
    return m_preferences;
}

void QbsManager::addProfile(const QString &name, const QVariantMap &data)
{
    const QString base = QLatin1String(PROFILES_PREFIX) + name;
    const QVariantMap::ConstIterator cend = data.constEnd();
    for (QVariantMap::ConstIterator it = data.constBegin(); it != cend; ++it)
        m_settings->setValue(base + it.key(), it.value());
}

void QbsManager::removeCreatorProfiles()
{
    QStringList keyList = m_settings->allKeys();
    QStringList profilesToDelete;

    // Find profiles to remove:
    foreach (const QString &key, keyList) {
        if (!key.startsWith(QLatin1String(PROFILE_LIST)))
            continue;
        profilesToDelete.append(m_settings->value(key).toString());
        m_settings->remove(key);
    }
    // Remove profiles:
    foreach (const QString &key, keyList) {
        if (!key.startsWith(QLatin1String(PROFILES_PREFIX)))
            continue;
        const QString kitname = key.mid(QString::fromLatin1(PROFILES_PREFIX).size());
        foreach (const QString &i, profilesToDelete) {
            if (kitname.startsWith(i + sep))
                m_settings->remove(key);
        }
    }
}

void QbsManager::addProfileFromKit(const ProjectExplorer::Kit *k)
{
    QStringList usedProfileNames = profileNames();
    const QString name = ProjectExplorer::Project::makeUnique(
                QString::fromLatin1("qtc_") + k->fileSystemFriendlyName(), usedProfileNames);
    setProfileForKit(name, k);

    QVariantMap data;
    QtSupport::BaseQtVersion *qt = QtSupport::QtKitInformation::qtVersion(k);
    if (qt) {
        data.insert(QLatin1String(QTCORE_BINPATH), qt->binPath().toUserOutput());
        QStringList builds;
        if (qt->hasDebugBuild())
            builds << QLatin1String("debug");
        if (qt->hasReleaseBuild())
            builds << QLatin1String("release");
        data.insert(QLatin1String(QTCORE_BUILDVARIANT), builds);
        data.insert(QLatin1String(QTCORE_DOCPATH), qt->docsPath().toUserOutput());
        data.insert(QLatin1String(QTCORE_INCPATH), qt->headerPath().toUserOutput());
        data.insert(QLatin1String(QTCORE_LIBPATH), qt->libraryPath().toUserOutput());
        Utils::FileName mkspecPath = qt->mkspecsPath();
        mkspecPath.appendPath(qt->mkspec().toString());
        data.insert(QLatin1String(QTCORE_MKSPEC), mkspecPath.toUserOutput());
        data.insert(QLatin1String(QTCORE_NAMESPACE), qt->qtNamespace());
        data.insert(QLatin1String(QTCORE_LIBINFIX), qt->qtLibInfix());
        data.insert(QLatin1String(QTCORE_VERSION), qt->qtVersionString());
        if (qt->isFrameworkBuild())
            data.insert(QLatin1String(QTCORE_FRAMEWORKBUILD), true);
    }

    if (ProjectExplorer::SysRootKitInformation::hasSysRoot(k))
        data.insert(QLatin1String(QBS_SYSROOT), ProjectExplorer::SysRootKitInformation::sysRoot(k).toUserOutput());

    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(k);
    if (tc) {
        // FIXME/CLARIFY: How to pass the sysroot?
        ProjectExplorer::Abi targetAbi = tc->targetAbi();
        QString architecture = ProjectExplorer::Abi::toString(targetAbi.architecture());
        if (targetAbi.wordWidth() == 64)
            architecture.append(QLatin1String("_64"));
        data.insert(QLatin1String(QBS_ARCHITECTURE), architecture);

        if (targetAbi.endianness() == ProjectExplorer::Abi::BigEndian)
            data.insert(QLatin1String(QBS_ENDIANNESS), QLatin1String("big"));
        else
            data.insert(QLatin1String(QBS_ENDIANNESS), QLatin1String("little"));

        if (targetAbi.os() == ProjectExplorer::Abi::WindowsOS) {
            data.insert(QLatin1String(QBS_TARGETOS), QLatin1String("windows"));
            data.insert(QLatin1String(QBS_TOOLCHAIN),
                               targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMSysFlavor
                                   ? QStringList() << QLatin1String("mingw") << QLatin1String("gcc")
                                   : QStringList() << QLatin1String("msvc"));
        } else if (targetAbi.os() == ProjectExplorer::Abi::MacOS) {
            data.insert(QLatin1String(QBS_TARGETOS), QStringList() << QLatin1String("osx")
                        << QLatin1String("darwin") << QLatin1String("unix"));
            if (tc->type() != QLatin1String("clang")) {
                data.insert(QLatin1String(QBS_TOOLCHAIN), QLatin1String("gcc"));
            } else {
                data.insert(QLatin1String(QBS_TOOLCHAIN),
                            QStringList() << QLatin1String("clang")
                            << QLatin1String("llvm")
                            << QLatin1String("gcc"));
            }
        } else if (targetAbi.os() == ProjectExplorer::Abi::LinuxOS) {
            data.insert(QLatin1String(QBS_TARGETOS), QStringList() << QLatin1String("linux")
                        << QLatin1String("unix"));
            if (tc->type() != QLatin1String("clang")) {
                data.insert(QLatin1String(QBS_TOOLCHAIN), QLatin1String("gcc"));
            } else {
                data.insert(QLatin1String(QBS_TOOLCHAIN),
                            QStringList() << QLatin1String("clang")
                            << QLatin1String("llvm")
                            << QLatin1String("gcc"));
            }
        }
        Utils::FileName cxx = tc->compilerCommand();
        data.insert(QLatin1String(CPP_TOOLCHAINPATH), cxx.toFileInfo().absolutePath());
        data.insert(QLatin1String(CPP_COMPILERNAME), cxx.toFileInfo().fileName());
    }
    addProfile(name, data);
}

void QbsManager::pushKitsToQbs()
{
    // Get all keys
    removeCreatorProfiles();

    // add definitions from our kits
    foreach (const ProjectExplorer::Kit *k, ProjectExplorer::KitManager::instance()->kits())
        addProfileFromKit(k);
}

} // namespace QbsProjectManager
