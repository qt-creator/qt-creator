/**************************************************************************
**
** Copyright (C) 2015 BogDan Vatra <bog_dan_ro@yahoo.com>
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

QHash<Abi, QList<int> > AndroidToolChainFactory::m_newestVersionForAbi;
FileName AndroidToolChainFactory::m_ndkLocation;

AndroidToolChain::AndroidToolChain(const Abi &abi, const QString &ndkToolChainVersion, Detection d)
    : GccToolChain(Constants::ANDROID_TOOLCHAIN_ID, d),
      m_ndkToolChainVersion(ndkToolChainVersion), m_secondaryToolChain(false)
{
    setTargetAbi(abi);
    setDisplayName(QString::fromLatin1("Android GCC (%1-%2)")
                   .arg(AndroidConfig::displayName(targetAbi()))
                   .arg(ndkToolChainVersion));
}

// for fromMap
AndroidToolChain::AndroidToolChain()
    : GccToolChain(Constants::ANDROID_TOOLCHAIN_ID, ToolChain::ManualDetection),
      m_secondaryToolChain(false)
{
}

AndroidToolChain::AndroidToolChain(const AndroidToolChain &tc) :
    GccToolChain(tc), m_ndkToolChainVersion(tc.m_ndkToolChainVersion),
    m_secondaryToolChain(tc.m_secondaryToolChain)
{ }

AndroidToolChain::~AndroidToolChain()
{ }

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
    env.set(QLatin1String("ANDROID_NDK_TOOLCHAIN_PREFIX"), AndroidConfig::toolchainPrefix(targetAbi()));
    env.set(QLatin1String("ANDROID_NDK_TOOLS_PREFIX"), AndroidConfig::toolsPrefix(targetAbi()));
    env.set(QLatin1String("ANDROID_NDK_TOOLCHAIN_VERSION"), m_ndkToolChainVersion);
    QString javaHome = AndroidConfigurations::currentConfig().openJDKLocation().toString();
    if (!javaHome.isEmpty() && QFileInfo::exists(javaHome))
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
    return AndroidConfigurations::currentConfig().gdbPath(targetAbi(), m_ndkToolChainVersion);
}

FileName AndroidToolChain::suggestedGdbServer() const
{
    FileName path = AndroidConfigurations::currentConfig().ndkLocation();
    path.appendPath(QString::fromLatin1("prebuilt/android-%1/gdbserver/gdbserver")
                    .arg(Abi::toString(targetAbi().architecture())));
    if (path.exists())
        return path;
    path = AndroidConfigurations::currentConfig().ndkLocation();
    path.appendPath(QString::fromLatin1("toolchains/%1-%2/prebuilt/gdbserver")
                               .arg(AndroidConfig::toolchainPrefix(targetAbi()))
                               .arg(m_ndkToolChainVersion));
    if (path.exists())
        return path;

    return FileName();
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
        setTargetAbi(AndroidConfig::abiForToolChainPrefix(platform));
    } else {
        m_ndkToolChainVersion = data.value(QLatin1String(ANDROID_NDK_TC_VERION)).toString();
    }

    Abi abi = targetAbi();
    m_secondaryToolChain = AndroidToolChainFactory::versionCompareLess(AndroidToolChainFactory::versionNumberFromString(m_ndkToolChainVersion),
                                                                       AndroidToolChainFactory::newestToolChainVersionForArch(abi));
    return isValid();
}

QList<FileName> AndroidToolChain::suggestedMkspecList() const
{
    return QList<FileName>()<< FileName::fromLatin1("android-g++");
}

QString AndroidToolChain::makeCommand(const Environment &env) const
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
    setTypeId(Constants::ANDROID_TOOLCHAIN_ID);
    setDisplayName(tr("Android GCC"));
}

QList<ToolChain *> AndroidToolChainFactory::autoDetect()
{
    return createToolChainsForNdk(AndroidConfigurations::currentConfig().ndkLocation());
}

bool AndroidToolChainFactory::canRestore(const QVariantMap &data)
{
    return typeIdFromMap(data) == Constants::ANDROID_TOOLCHAIN_ID;
}

ToolChain *AndroidToolChainFactory::restore(const QVariantMap &data)
{
    AndroidToolChain *tc = new AndroidToolChain();
    if (tc->fromMap(data))
        return tc;

    delete tc;
    return 0;
}

QList<AndroidToolChainFactory::AndroidToolChainInformation> AndroidToolChainFactory::toolchainPathsForNdk(const FileName &ndkPath)
{
    QList<AndroidToolChainInformation> result;
    if (ndkPath.isEmpty())
        return result;
    QRegExp versionRegExp(NDKGccVersionRegExp);
    FileName path = ndkPath;
    QDirIterator it(path.appendPath(QLatin1String("toolchains")).toString(),
                    QStringList() << QLatin1String("*"), QDir::Dirs);
    while (it.hasNext()) {
        const QString &fileName = FileName::fromString(it.next()).fileName();
        int idx = versionRegExp.indexIn(fileName);
        if (idx == -1)
            continue;
        AndroidToolChainInformation ati;
        ati.version = fileName.mid(idx + 1);
        QString platform = fileName.left(idx);
        ati.abi = AndroidConfig::abiForToolChainPrefix(platform);
        if (ati.abi.architecture() == Abi::UnknownArchitecture) // e.g. mipsel which is not yet supported
            continue;
        // AndroidToolChain *tc = new AndroidToolChain(arch, version, true);
        ati.compilerCommand = AndroidConfigurations::currentConfig().gccPath(ati.abi, ati.version);
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

QList<ToolChain *> AndroidToolChainFactory::createToolChainsForNdk(const FileName &ndkPath)
{
    QList<ToolChain *> result;
    if (ndkPath.isEmpty())
        return result;
    QRegExp versionRegExp(NDKGccVersionRegExp);
    FileName path = ndkPath;
    QDirIterator it(path.appendPath(QLatin1String("toolchains")).toString(),
                    QStringList() << QLatin1String("*"), QDir::Dirs);
    QHash<Abi, AndroidToolChain *> newestToolChainForArch;

    while (it.hasNext()) {
        const QString &fileName = FileName::fromString(it.next()).fileName();
        int idx = versionRegExp.indexIn(fileName);
        if (idx == -1)
            continue;
        QString version = fileName.mid(idx + 1);
        QString platform = fileName.left(idx);
        Abi abi = AndroidConfig::abiForToolChainPrefix(platform);
        if (abi.architecture() == Abi::UnknownArchitecture) // e.g. mipsel which is not yet supported
            continue;
        AndroidToolChain *tc = new AndroidToolChain(abi, version, ToolChain::AutoDetection);
        FileName compilerPath = AndroidConfigurations::currentConfig().gccPath(abi, version);
        tc->resetToolChain(compilerPath);
        result.append(tc);

        QHash<Abi, AndroidToolChain *>::const_iterator it
                = newestToolChainForArch.constFind(abi);
        if (it == newestToolChainForArch.constEnd())
            newestToolChainForArch.insert(abi, tc);
        else if (versionCompareLess(it.value(), tc))
            newestToolChainForArch[abi] = tc;
    }

    foreach (ToolChain *tc, result) {
        AndroidToolChain *atc = static_cast<AndroidToolChain *>(tc);
        if (newestToolChainForArch.value(atc->targetAbi()) != atc)
            atc->setSecondaryToolChain(true);
    }

    return result;
}

QList<int> AndroidToolChainFactory::newestToolChainVersionForArch(const Abi &abi)
{
    if (m_newestVersionForAbi.isEmpty()
            || m_ndkLocation != AndroidConfigurations::currentConfig().ndkLocation()) {
        QRegExp versionRegExp(NDKGccVersionRegExp);
        m_ndkLocation = AndroidConfigurations::currentConfig().ndkLocation();
        FileName path = m_ndkLocation;
        QDirIterator it(path.appendPath(QLatin1String("toolchains")).toString(),
                        QStringList() << QLatin1String("*"), QDir::Dirs);
        while (it.hasNext()) {
            const QString &fileName = FileName::fromString(it.next()).fileName();
            int idx = versionRegExp.indexIn(fileName);
            if (idx == -1)
                continue;
            QList<int> version = versionNumberFromString(fileName.mid(idx + 1));
            QString platform = fileName.left(idx);
            Abi abi = AndroidConfig::abiForToolChainPrefix(platform);
            if (abi.architecture() == Abi::UnknownArchitecture) // e.g. mipsel which is not yet supported
                continue;
            QHash<Abi, QList<int> >::const_iterator it
                    = m_newestVersionForAbi.constFind(abi);
            if (it == m_newestVersionForAbi.constEnd())
                m_newestVersionForAbi.insert(abi, version);
            else if (versionCompareLess(it.value(), version))
                m_newestVersionForAbi[abi] = version;
        }
    }
    return m_newestVersionForAbi.value(abi);
}

} // namespace Internal
} // namespace Android
