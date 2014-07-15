/**************************************************************************
**
** Copyright (c) 2014 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidtoolchain.h"
#include "androidconstants.h"
#include "androidconfigurations.h"
#include "androidqtversion.h"

#include <extensionsystem/pluginmanager.h>

#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>

#include <projectexplorer/target.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/projectexplorer.h>

#include <utils/environment.h>
#include <utils/hostosinfo.h>

#include <QDir>
#include <QDirIterator>
#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace {
    const QLatin1String NDKGccVersionRegExp("-\\d[\\.\\d]+");
}

namespace Android {
namespace Internal {

using namespace ProjectExplorer;
using namespace Utils;

static const char ANDROID_QT_VERSION_KEY[] = "Qt4ProjectManager.Android.QtVersion";
static const char ANDROID_NDK_TC_VERION[] = "Qt4ProjectManager.Android.NDK_TC_VERION";

QMap<ProjectExplorer::Abi::Architecture, QList<int> > AndroidToolChainFactory::m_newestVersionForArch;
Utils::FileName AndroidToolChainFactory::m_ndkLocation;

AndroidToolChain::AndroidToolChain(Abi::Architecture arch, const QString &ndkToolChainVersion, Detection d)
    : GccToolChain(QLatin1String(Constants::ANDROID_TOOLCHAIN_ID), d),
      m_ndkToolChainVersion(ndkToolChainVersion), m_secondaryToolChain(false)
{
    ProjectExplorer::Abi abi = ProjectExplorer::Abi(arch, ProjectExplorer::Abi::LinuxOS,
                                                    ProjectExplorer::Abi::AndroidLinuxFlavor, ProjectExplorer::Abi::ElfFormat,
                                                    32);
    setTargetAbi(abi);
    setDisplayName(QString::fromLatin1("Android GCC (%1-%2)")
                   .arg(Abi::toString(targetAbi().architecture()))
                   .arg(ndkToolChainVersion));
}

// for fromMap
AndroidToolChain::AndroidToolChain()
    : GccToolChain(QLatin1String(Constants::ANDROID_TOOLCHAIN_ID), ToolChain::ManualDetection),
      m_secondaryToolChain(false)
{
}

AndroidToolChain::AndroidToolChain(const AndroidToolChain &tc) :
    GccToolChain(tc), m_ndkToolChainVersion(tc.m_ndkToolChainVersion),
    m_secondaryToolChain(tc.m_secondaryToolChain)
{ }

AndroidToolChain::~AndroidToolChain()
{ }

QString AndroidToolChain::type() const
{
    return QLatin1String(Constants::ANDROID_TOOLCHAIN_TYPE);
}

QString AndroidToolChain::typeDisplayName() const
{
    return AndroidToolChainFactory::tr("Android GCC");
}

bool AndroidToolChain::isValid() const
{
    return GccToolChain::isValid() && targetAbi().isValid() && !m_ndkToolChainVersion.isEmpty()
            && compilerCommand().isChildOf(AndroidConfigurations::currentConfig().ndkLocation());
}

void AndroidToolChain::addToEnvironment(Environment &env) const
{

// TODO this vars should be configurable in projects -> build tab
// TODO invalidate all .pro files !!!

    env.set(QLatin1String("ANDROID_NDK_HOST"), AndroidConfigurations::currentConfig().toolchainHost());
    env.set(QLatin1String("ANDROID_NDK_TOOLCHAIN_PREFIX"), AndroidConfig::toolchainPrefix(targetAbi().architecture()));
    env.set(QLatin1String("ANDROID_NDK_TOOLS_PREFIX"), AndroidConfig::toolsPrefix(targetAbi().architecture()));
    env.set(QLatin1String("ANDROID_NDK_TOOLCHAIN_VERSION"), m_ndkToolChainVersion);
    QString javaHome = AndroidConfigurations::currentConfig().openJDKLocation().toString();
    if (!javaHome.isEmpty() && QFileInfo(javaHome).exists())
        env.set(QLatin1String("JAVA_HOME"), javaHome);
    env.set(QLatin1String("ANDROID_HOME"), AndroidConfigurations::currentConfig().sdkLocation().toString());
    env.set(QLatin1String("ANDROID_SDK_ROOT"), AndroidConfigurations::currentConfig().sdkLocation().toString());
}

bool AndroidToolChain::operator ==(const ToolChain &tc) const
{
    if (!GccToolChain::operator ==(tc))
        return false;

    return m_ndkToolChainVersion == static_cast<const AndroidToolChain &>(tc).m_ndkToolChainVersion;
}

ToolChainConfigWidget *AndroidToolChain::configurationWidget()
{
    return new AndroidToolChainConfigWidget(this);
}

FileName AndroidToolChain::suggestedDebugger() const
{
    return AndroidConfigurations::currentConfig().gdbPath(targetAbi().architecture(), m_ndkToolChainVersion);
}

FileName AndroidToolChain::suggestedGdbServer() const
{
    Utils::FileName path = AndroidConfigurations::currentConfig().ndkLocation();
    path.appendPath(QString::fromLatin1("prebuilt/android-%1/gdbserver/gdbserver")
                    .arg(Abi::toString(targetAbi().architecture())));
    if (path.toFileInfo().exists())
        return path;
    path = AndroidConfigurations::currentConfig().ndkLocation();
    path.appendPath(QString::fromLatin1("toolchains/%1-%2/prebuilt/gdbserver")
                               .arg(AndroidConfig::toolchainPrefix(targetAbi().architecture()))
                               .arg(m_ndkToolChainVersion));
    if (path.toFileInfo().exists())
        return path;

    return Utils::FileName();
}

QVariantMap AndroidToolChain::toMap() const
{
    QVariantMap result = GccToolChain::toMap();
    result.insert(QLatin1String(ANDROID_NDK_TC_VERION), m_ndkToolChainVersion);
    return result;
}

bool AndroidToolChain::fromMap(const QVariantMap &data)
{
    if (!GccToolChain::fromMap(data))
        return false;

    if (data.contains(QLatin1String(ANDROID_QT_VERSION_KEY))) {
        QString command = compilerCommand().toString();
        QString ndkPath = AndroidConfigurations::currentConfig().ndkLocation().toString();
        if (!command.startsWith(ndkPath))
            return false;
        command = command.mid(ndkPath.length());
        if (!command.startsWith(QLatin1String("/toolchains/")))
            return false;
        command = command.mid(12);
        int index = command.indexOf(QLatin1Char('/'));
        if (index == -1)
            return false;
        command = command.left(index);
        QRegExp versionRegExp(NDKGccVersionRegExp);
        index = versionRegExp.indexIn(command);
        if (index == -1)
            return false;
        m_ndkToolChainVersion = command.mid(index + 1);
        QString platform = command.left(index);
        Abi::Architecture arch = AndroidConfig::architectureForToolChainPrefix(platform);
        ProjectExplorer::Abi abi = ProjectExplorer::Abi(arch, ProjectExplorer::Abi::LinuxOS,
                                                        ProjectExplorer::Abi::AndroidLinuxFlavor, ProjectExplorer::Abi::ElfFormat,
                                                        32);
        setTargetAbi(abi);
    } else {
        m_ndkToolChainVersion = data.value(QLatin1String(ANDROID_NDK_TC_VERION)).toString();
    }

    ProjectExplorer::Abi::Architecture arch = targetAbi().architecture();
    m_secondaryToolChain = AndroidToolChainFactory::versionCompareLess(AndroidToolChainFactory::versionNumberFromString(m_ndkToolChainVersion),
                                                                       AndroidToolChainFactory::newestToolChainVersionForArch(arch));
    return isValid();
}

QList<FileName> AndroidToolChain::suggestedMkspecList() const
{
    return QList<FileName>()<< FileName::fromLatin1("android-g++");
}

QString AndroidToolChain::makeCommand(const Utils::Environment &env) const
{
    QStringList extraDirectories = AndroidConfigurations::currentConfig().makeExtraSearchDirectories();
    if (HostOsInfo::isWindowsHost()) {
        FileName tmp = env.searchInPath(QLatin1String("ma-make.exe"), extraDirectories);
        if (!tmp.isEmpty())
            return QString();
        tmp = env.searchInPath(QLatin1String("mingw32-make"), extraDirectories);
        return tmp.isEmpty() ? QLatin1String("mingw32-make") : tmp.toString();
    }

    QString make = QLatin1String("make");
    FileName tmp = env.searchInPath(make, extraDirectories);
    return tmp.isEmpty() ? make : tmp.toString();
}

QString AndroidToolChain::ndkToolChainVersion() const
{
    return m_ndkToolChainVersion;
}

bool AndroidToolChain::isSecondaryToolChain() const
{
    return m_secondaryToolChain;
}

void AndroidToolChain::setSecondaryToolChain(bool b)
{
    m_secondaryToolChain = b;
}

QList<Abi> AndroidToolChain::detectSupportedAbis() const
{
    return QList<Abi>() << targetAbi();
}

// --------------------------------------------------------------------------
// ToolChainConfigWidget
// --------------------------------------------------------------------------

AndroidToolChainConfigWidget::AndroidToolChainConfigWidget(AndroidToolChain *tc) :
   ToolChainConfigWidget(tc)
{
    QLabel *label = new QLabel(AndroidConfigurations::currentConfig().ndkLocation().toUserOutput());
    m_mainLayout->addRow(tr("NDK Root:"), label);
}

// --------------------------------------------------------------------------
// ToolChainFactory
// --------------------------------------------------------------------------

AndroidToolChainFactory::AndroidToolChainFactory()
{
    setId(Constants::ANDROID_TOOLCHAIN_ID);
    setDisplayName(tr("Android GCC"));
}

QList<ToolChain *> AndroidToolChainFactory::autoDetect()
{
    return createToolChainsForNdk(AndroidConfigurations::currentConfig().ndkLocation());
}

bool AndroidToolChainFactory::canRestore(const QVariantMap &data)
{
    return idFromMap(data).startsWith(QLatin1String(Constants::ANDROID_TOOLCHAIN_ID) + QLatin1Char(':'));
}

ToolChain *AndroidToolChainFactory::restore(const QVariantMap &data)
{
    AndroidToolChain *tc = new AndroidToolChain();
    if (tc->fromMap(data))
        return tc;

    delete tc;
    return 0;
}

QList<AndroidToolChainFactory::AndroidToolChainInformation> AndroidToolChainFactory::toolchainPathsForNdk(const Utils::FileName &ndkPath)
{
    QList<AndroidToolChainInformation> result;
    if (ndkPath.isEmpty())
        return result;
    QRegExp versionRegExp(NDKGccVersionRegExp);
    FileName path = ndkPath;
    QDirIterator it(path.appendPath(QLatin1String("toolchains")).toString(),
                    QStringList() << QLatin1String("*"), QDir::Dirs);
    while (it.hasNext()) {
        const QString &fileName = QFileInfo(it.next()).fileName();
        int idx = versionRegExp.indexIn(fileName);
        if (idx == -1)
            continue;
        AndroidToolChainInformation ati;
        ati.version = fileName.mid(idx + 1);
        QString platform = fileName.left(idx);
        ati.architecture = AndroidConfig::architectureForToolChainPrefix(platform);
        if (ati.architecture == Abi::UnknownArchitecture) // e.g. mipsel which is not yet supported
            continue;
        // AndroidToolChain *tc = new AndroidToolChain(arch, version, true);
        ati.compilerCommand = AndroidConfigurations::currentConfig().gccPath(ati.architecture, ati.version);
        // tc->setCompilerCommand(compilerPath);
        result.append(ati);
    }
    return result;
}

QList<int> AndroidToolChainFactory::versionNumberFromString(const QString &version)
{
    QList<int> result;
    int start = 0;
    int end = version.length();
    while (start <= end) {
        int index = version.indexOf(QLatin1Char('.'), start);
        if (index == -1)
            index = end;

        bool ok;
        int v = version.mid(start, index - start).toInt(&ok);
        if (!ok) // unparseable, return what we have
            return result;

        result << v;
        start = index + 1;
    }
    return result;
}

bool AndroidToolChainFactory::versionCompareLess(const QList<int> &a, const QList<int> &b)
{
    int aend = a.length();
    int bend = b.length();
    int end = qMax(aend, bend);
    for (int i = 0; i < end; ++i) {
        int an = i < aend ? a.at(i) : 0;
        int bn = i < bend ? b.at(i) : 0;
        if (an < bn)
            return true;
        if (bn < an)
            return false;
    }
    return false;
}

bool AndroidToolChainFactory::versionCompareLess(AndroidToolChain *atc, AndroidToolChain *btc)
{
    QList<int> a = versionNumberFromString(atc->ndkToolChainVersion());
    QList<int> b = versionNumberFromString(btc->ndkToolChainVersion());

    return versionCompareLess(a, b);
}

QList<ToolChain *> AndroidToolChainFactory::createToolChainsForNdk(const Utils::FileName &ndkPath)
{
    QList<ToolChain *> result;
    if (ndkPath.isEmpty())
        return result;
    QRegExp versionRegExp(NDKGccVersionRegExp);
    FileName path = ndkPath;
    QDirIterator it(path.appendPath(QLatin1String("toolchains")).toString(),
                    QStringList() << QLatin1String("*"), QDir::Dirs);
    QMap<Abi::Architecture, AndroidToolChain *> newestToolChainForArch;

    while (it.hasNext()) {
        const QString &fileName = QFileInfo(it.next()).fileName();
        int idx = versionRegExp.indexIn(fileName);
        if (idx == -1)
            continue;
        QString version = fileName.mid(idx + 1);
        QString platform = fileName.left(idx);
        Abi::Architecture arch = AndroidConfig::architectureForToolChainPrefix(platform);
        if (arch == Abi::UnknownArchitecture) // e.g. mipsel which is not yet supported
            continue;
        AndroidToolChain *tc = new AndroidToolChain(arch, version, ToolChain::AutoDetection);
        FileName compilerPath = AndroidConfigurations::currentConfig().gccPath(arch, version);
        tc->setCompilerCommand(compilerPath);
        result.append(tc);

        QMap<Abi::Architecture, AndroidToolChain *>::const_iterator it
                = newestToolChainForArch.constFind(arch);
        if (it == newestToolChainForArch.constEnd())
            newestToolChainForArch.insert(arch, tc);
        else if (versionCompareLess(it.value(), tc))
            newestToolChainForArch[arch] = tc;
    }

    foreach (ToolChain *tc, result) {
        AndroidToolChain *atc = static_cast<AndroidToolChain *>(tc);
        if (newestToolChainForArch.value(atc->targetAbi().architecture()) != atc)
            atc->setSecondaryToolChain(true);
    }

    return result;
}

QList<int> AndroidToolChainFactory::newestToolChainVersionForArch(Abi::Architecture arch)
{
    if (m_newestVersionForArch.isEmpty()
            || m_ndkLocation != AndroidConfigurations::currentConfig().ndkLocation()) {
        QRegExp versionRegExp(NDKGccVersionRegExp);
        m_ndkLocation = AndroidConfigurations::currentConfig().ndkLocation();
        FileName path = m_ndkLocation;
        QDirIterator it(path.appendPath(QLatin1String("toolchains")).toString(),
                        QStringList() << QLatin1String("*"), QDir::Dirs);
        while (it.hasNext()) {
            const QString &fileName = QFileInfo(it.next()).fileName();
            int idx = versionRegExp.indexIn(fileName);
            if (idx == -1)
                continue;
            QList<int> version = versionNumberFromString(fileName.mid(idx + 1));
            QString platform = fileName.left(idx);
            Abi::Architecture arch = AndroidConfig::architectureForToolChainPrefix(platform);
            if (arch == Abi::UnknownArchitecture) // e.g. mipsel which is not yet supported
                continue;
            QMap<Abi::Architecture, QList<int> >::const_iterator it
                    = m_newestVersionForArch.constFind(arch);
            if (it == m_newestVersionForArch.constEnd())
                m_newestVersionForArch.insert(arch, version);
            else if (versionCompareLess(it.value(), version))
                m_newestVersionForArch[arch] = version;
        }
    }
    return m_newestVersionForArch.value(arch);
}

} // namespace Internal
} // namespace Android
