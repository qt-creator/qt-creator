/**************************************************************************
**
** Copyright (c) 2013 BogDan Vatra <bog_dan_ro@yahoo.com>
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

AndroidToolChain::AndroidToolChain(Abi::Architecture arch, const QString &ndkToolChainVersion, bool autodetected)
    : GccToolChain(QLatin1String(Constants::ANDROID_TOOLCHAIN_ID), autodetected),
      m_ndkToolChainVersion(ndkToolChainVersion)
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
    : GccToolChain(QLatin1String(Constants::ANDROID_TOOLCHAIN_ID), false)
{
}

AndroidToolChain::AndroidToolChain(const AndroidToolChain &tc) :
    GccToolChain(tc), m_ndkToolChainVersion(tc.m_ndkToolChainVersion)
{ }

AndroidToolChain::~AndroidToolChain()
{ }

QString AndroidToolChain::type() const
{
    return QLatin1String(Android::Constants::ANDROID_TOOLCHAIN_TYPE);
}

QString AndroidToolChain::typeDisplayName() const
{
    return AndroidToolChainFactory::tr("Android GCC");
}

bool AndroidToolChain::isValid() const
{
    return GccToolChain::isValid() && targetAbi().isValid() && !m_ndkToolChainVersion.isEmpty()
            && compilerCommand().isChildOf(AndroidConfigurations::instance().config().ndkLocation);
}

void AndroidToolChain::addToEnvironment(Environment &env) const
{

// TODO this vars should be configurable in projects -> build tab
// TODO invalidate all .pro files !!!

    env.set(QLatin1String("ANDROID_NDK_HOST"), AndroidConfigurations::instance().config().toolchainHost);
    env.set(QLatin1String("ANDROID_NDK_TOOLCHAIN_PREFIX"), AndroidConfigurations::toolchainPrefix(targetAbi().architecture()));
    env.set(QLatin1String("ANDROID_NDK_TOOLS_PREFIX"), AndroidConfigurations::toolsPrefix(targetAbi().architecture()));
    env.set(QLatin1String("ANDROID_NDK_TOOLCHAIN_VERSION"), m_ndkToolChainVersion);
    QString javaHome = AndroidConfigurations::instance().openJDKPath().toString();
    if (!javaHome.isEmpty() && QFileInfo(javaHome).exists())
        env.set(QLatin1String("JAVA_HOME"), javaHome);
    env.set(QLatin1String("ANDROID_HOME"), AndroidConfigurations::instance().config().sdkLocation.toString());
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
    return AndroidConfigurations::instance().gdbPath(targetAbi().architecture(), m_ndkToolChainVersion);
}

FileName AndroidToolChain::suggestedGdbServer() const
{
    Utils::FileName path = AndroidConfigurations::instance().config().ndkLocation;
    path.appendPath(QString::fromLatin1("prebuilt/android-%1/gdbserver/gdbserver")
                    .arg(Abi::toString(targetAbi().architecture())));
    if (path.toFileInfo().exists())
        return path;
    path = AndroidConfigurations::instance().config().ndkLocation;
    path.appendPath(QString::fromLatin1("toolchains/%1-%2/prebuilt/gdbserver")
                               .arg(AndroidConfigurations::toolchainPrefix(targetAbi().architecture()))
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
        QString ndkPath = AndroidConfigurations::instance().config().ndkLocation.toString();
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
        Abi::Architecture arch = AndroidConfigurations::architectureForToolChainPrefix(platform);
        ProjectExplorer::Abi abi = ProjectExplorer::Abi(arch, ProjectExplorer::Abi::LinuxOS,
                                                        ProjectExplorer::Abi::AndroidLinuxFlavor, ProjectExplorer::Abi::ElfFormat,
                                                        32);
        setTargetAbi(abi);
    } else {
        m_ndkToolChainVersion = data.value(QLatin1String(ANDROID_NDK_TC_VERION)).toString();
    }

    return isValid();
}

QList<FileName> AndroidToolChain::suggestedMkspecList() const
{
    return QList<FileName>()<< FileName::fromString(QLatin1String("android-g++"));
}

QString AndroidToolChain::makeCommand(const Utils::Environment &env) const
{
    QStringList extraDirectories = AndroidConfigurations::instance().makeExtraSearchDirectories();
    if (HostOsInfo::isWindowsHost()) {
        QString tmp = env.searchInPath(QLatin1String("ma-make.exe"), extraDirectories);
        if (!tmp.isEmpty())
            return tmp;
        tmp = env.searchInPath(QLatin1String("mingw32-make"), extraDirectories);
        return tmp.isEmpty() ? QLatin1String("mingw32-make") : tmp;
    }

    QString make = QLatin1String("make");
    QString tmp = env.searchInPath(make, extraDirectories);
    return tmp.isEmpty() ? make : tmp;
}

QString AndroidToolChain::ndkToolChainVersion()
{
    return m_ndkToolChainVersion;
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
    QLabel *label = new QLabel(AndroidConfigurations::instance().config().ndkLocation.toUserOutput());
    m_mainLayout->addRow(tr("NDK Root:"), label);
}

// --------------------------------------------------------------------------
// ToolChainFactory
// --------------------------------------------------------------------------

AndroidToolChainFactory::AndroidToolChainFactory() :
    ToolChainFactory()
{ }

QString AndroidToolChainFactory::displayName() const
{
    return tr("Android GCC");
}

QString AndroidToolChainFactory::id() const
{
    return QLatin1String(Constants::ANDROID_TOOLCHAIN_ID);
}

QList<ToolChain *> AndroidToolChainFactory::autoDetect()
{
    return createToolChainsForNdk(AndroidConfigurations::instance().config().ndkLocation);
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
        ati.architecture = AndroidConfigurations::architectureForToolChainPrefix(platform);
        if (ati.architecture == Abi::UnknownArchitecture) // e.g. mipsel which is not yet supported
            continue;
        // AndroidToolChain *tc = new AndroidToolChain(arch, version, true);
        ati.compilerCommand = AndroidConfigurations::instance().gccPath(ati.architecture, ati.version);
        // tc->setCompilerCommand(compilerPath);
        result.append(ati);
    }
    return result;
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
    while (it.hasNext()) {
        const QString &fileName = QFileInfo(it.next()).fileName();
        int idx = versionRegExp.indexIn(fileName);
        if (idx == -1)
            continue;
        QString version = fileName.mid(idx + 1);
        QString platform = fileName.left(idx);
        Abi::Architecture arch = AndroidConfigurations::architectureForToolChainPrefix(platform);
        if (arch == Abi::UnknownArchitecture) // e.g. mipsel which is not yet supported
            continue;
        AndroidToolChain *tc = new AndroidToolChain(arch, version, true);
        FileName compilerPath = AndroidConfigurations::instance().gccPath(arch, version);
        tc->setCompilerCommand(compilerPath);
        result.append(tc);
    }
    return result;
}

} // namespace Internal
} // namespace Android
